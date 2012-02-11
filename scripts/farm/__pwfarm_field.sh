#!/bin/bash

source $( dirname $BASH_SOURCE )/__lib.sh || exit 1

MODE=$1

VERBOSE=false

eval FIELD_STATE_DIR="$( pwenv fieldstate_dir )"
export PWFARM_TASKMETA_PATH=$FIELD_STATE_DIR/tasks/$( pwenv taskid )
function PWFARM_TASKMETA()
{
    local mode=$1
    shift
    taskmeta $mode $PWFARM_TASKMETA_PATH "$@"
}
export -f PWFARM_TASKMETA
export PWFARM_STATUS_STATE="$FIELD_STATE_DIR/status_state"
COMMAND=$( PWFARM_TASKMETA get command )
RESULT_DIR="$FIELD_STATE_DIR/result"
RESULT_ARCHIVE="$FIELD_STATE_DIR/result.tbz"
LOG="/tmp/log_user$(pwenv pwuser)_field$(pwenv fieldnumber)_session$(pwenv sessionname)_farm$(pwenv farmname)"
MUTEX="${FIELD_STATE_DIR}/mutex"
PID="${FIELD_STATE_DIR}/pid"
PID_COMMAND="${FIELD_STATE_DIR}/pid_command"
PID_STATUS="${FIELD_STATE_DIR}/pid_status"
COMMAND_LAUNCH_BEGIN="${FIELD_STATE_DIR}/command_launch_begin"
COMMAND_LAUNCH_END="${FIELD_STATE_DIR}/command_launch_end"
COMMAND_BORN="${FIELD_STATE_DIR}/command_born"
COMMAND_DONE="${FIELD_STATE_DIR}/command_done"
FIELD_SCREEN_SESSION="$( pwenv fieldscreensession )"

function dbprompt()
{
    return

    if [ $# != 0 ]; then
	local prompt="[debug] $*"
    else
	local prompt="[debug] Press enter..."
    fi

    read -p "$prompt"
}

function log()
{
    local doecho=$VERBOSE
    if [ "$1" == "--echo" ]; then
	doecho="true"
	shift
    fi

    echo "[$$]: $*" >> $LOG

    if $doecho; then
	echo "__pwfarm_field.sh [$$]: $*"
    fi
}

function screen_active()
{
    screen -ls | grep "\\b${FIELD_SCREEN_SESSION}\\b" > /dev/null
}

function screen_attached()
{
    screen -ls | grep "\\b${FIELD_SCREEN_SESSION}\\b" | grep "Attached"
}

function get_screen_pid()
{
    screen -ls | grep "\\b${FIELD_SCREEN_SESSION}\\b" | grep -o '^[[:space:]]\+[0-9]\+'
}

function screen_detach()
{
    screen -S "${FIELD_SCREEN_SESSION}" -d
}

function screen_reattach()
{
    screen -S "${FIELD_SCREEN_SESSION}" -r
}

function screen_resume()
{
    while screen_attached; do
	log --echo "Attempting to detach screen..."
	screen_detach
	sleep 2
    done

    # If we don't issue SIGCHLD, the reattach is unreliable.
    local screen_pid=$( get_screen_pid )
    log "Sending SIGCHLD to screen (pid=$screen_pid)..."
    kill -CHLD $screen_pid

    log --echo "Reattaching..."
    screen_reattach
    log "Reattach returned."
}

function screen_command()
{
    screen -d -m -S "${FIELD_SCREEN_SESSION}" bash -c "$0 command"
}

function screen_wait_complete()
{
    log "\
ENTER screen_wait_complete.
Screen session = ${FIELD_SCREEN_SESSION}.
screen -ls:
$( screen -ls )"

    local nbornfail=0
    while [ ! -e "$COMMAND_BORN" ]; do
	log "Command not born yet... (nbornfail=$nbornfail)"
	log "$( screen -ls )"
	nbornfail=$(( $nbornfail + 1 ))
	if [ $nbornfail == "50" ]; then
	    log "Giving up on birth of screen. Please press enter..."
	    return 1
	fi
	sleep 1
    done

    while screen_active ; do
	log "Detected screen session. Attempting to resume."
	screen_resume

	if screen_active ; then
	    log "After completion of screen resume, screen still alive!
                 Will sleep a moment and retry.
                 Screen session = ${FIELD_SCREEN_SESSION}
	           screen -ls:
	           $( screen -ls )"
	    sleep 5

	    if [ -e "$COMMAND_DONE" ]; then
		if screen_active ; then
		    echo "Found COMMAND_DONE. Killing screen."
		    screen -S "${FIELD_SCREEN_SESSION}" -X quit
		    sleep 5
		fi
	    fi
	fi
    done

    return 0
}

function kill_launcher()
{
    if is_process_alive $PID; then
	local pid=$( cat $PID )
	log "Killing launcher $pid"
	kill $pid
	rm -f "$PID"
    fi
}

mkdir -p "${FIELD_STATE_DIR}"
mkdir -p "${RESULT_DIR}"

log "===== INVOKED ===== args: $*"

case "$MODE" in
    "launch")

	while ! mutex_trylock "$MUTEX" ; do
	    log "Failed locking mutex!"
	    if is_process_alive $PID; then
		log "Launcher process is still alive. Waiting..."
		sleep 5
	    else
		log "Launcher process is dead. Assuming ownership of mutex." >&2
		break;
	    fi
	done

	log "Own mutex"

	# Make sure any other launcher for this session is dead. There could be a live
	# one from a previous SSH session with a lost connection.
	kill_launcher

	echo $$ > $PID

	if [ -e "$COMMAND_DONE" ]; then
	    log "Found command_done."
	elif [ -e "$COMMAND_LAUNCH_END" ]; then
	    log "found command_launch_end."
	elif [ -e "$COMMAND_LAUNCH_BEGIN" ]; then
	    log "Found command_launch_begin, but not end."
	    sleep 10
	
	    if screen_active ; then
		log "Screen is alive! Touching launch_end"
		touch "$COMMAND_LAUNCH_END"
	    else
		log "Screen still not alive. Attempting launch."
		screen_command
		touch "$COMMAND_LAUNCH_END"
	    fi
	else
	    log "No evidence of previous command launch attempts. Launching."
	    touch "$COMMAND_LAUNCH_BEGIN"
	    screen_command
	    touch "$COMMAND_LAUNCH_END"
	fi

	mutex_unlock "$MUTEX"
	screen_wait_complete

	mutex_lock "$MUTEX"

	if [ ! -e "$RESULT_ARCHIVE" ]; then
	    log "creating $RESULT_ARCHIVE"

	    if ! PWFARM_TASKMETA has exitval; then
		log "!!! DIDN'T FIND EXITVAL! COMMAND MUST HAVE TRAPPED !!!"
		PWFARM_TASKMETA set exitval 1
	    fi

	    cd "$RESULT_DIR"

	    cp "$PWFARM_TASKMETA_PATH" ./taskmeta
	    cp "$LOG" ./log

	    result_archive_tmp="/tmp/result.$$.tbz"
	    echo "Creating temporary result archive $result_archive_tmp"
	    archive pack $result_archive_tmp .
	    mv $result_archive_tmp "$RESULT_ARCHIVE"
	    cd "$FIELD_STATE_DIR"

	    log "$RESULT_ARCHIVE generated"

	    rm "$LOG"
	fi

	rm -f $PID

	mutex_unlock "$MUTEX"

	dbprompt "end launch"

	exit 0
	;;
    "command")
	touch "$COMMAND_BORN"

	# Start status server as background task.
	$PWFARM_SCRIPTS_DIR/__pwfarm_status.py $PWFARM_STATUS_STATE server &
	echo $! > $PID_STATUS

	function PWFARM_STATUS()
	{
	    $PWFARM_SCRIPTS_DIR/__pwfarm_status.py $PWFARM_STATUS_STATE set "$*"
	}
	export -f PWFARM_STATUS

	PWFARM_STATUS "Init Task"
	
	echo $$ > $PID_COMMAND

	function PWFARM_SUDO()
	{
	    if [ "$PASSWORD" == "nil" ]; then
		echo "PWFARM_SUDO invoked, but no password provided to script! __pwfarm_dispatcher must be invoked with --password!" >&2
		exit 1
	    fi
	    printf "$PASSWORD\n" | sudo -S -E $*
	    local exitval=$?

 	    # Make sure sudo queries us for a password on next invocation. 
	    sudo -k

	    return $exitval
	}
	export -f PWFARM_SUDO

	export PWFARM_OUTPUT_ARCHIVE="${RESULT_DIR}/output.tbz"

	if [ "$( uname )" == "Darwin" ]; then
	    export PATH=$PATH:/usr/local/bin:/Library/Frameworks/Python.framework/Versions/Current/bin
	fi

	#
	# EXEC COMMAND
	#
	log "Executing command: $COMMAND"

	PWFARM_STATUS "Running"

	bash -c "cd payload && $COMMAND"
	exitval=$?

	log "Command complete. exitval=$exitval"

	PWFARM_TASKMETA set exitval $exitval

	if [ $exitval == 0 ]; then
	    PWFARM_STATUS "Success"
	else
	    PWFARM_STATUS "ERROR"
	fi

	if [ "$(PWFARM_TASKMETA get prompterr)" != "false" ] && [ $exitval != 0 ] ; then
	    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" >&2
	    echo "An error occurred. Press \"Ctrl-a Esc\" to use PgUp/PgDown to find error description." >&2
	    echo "When done looking, press Esc, then Enter..." >&2
	    read
	fi

	$PWFARM_SCRIPTS_DIR/__pwfarm_status.py $PWFARM_STATUS_STATE quit 
	log "Waiting for status server to die..."
	wait

	rm $PID_COMMAND
	rm $PID_STATUS

	touch "$COMMAND_DONE"

	log "COMMAND DONE!"

	dbprompt "end command"

	exit 0
	;;
    "*")
	read -p "Invalid field mode ($MODE). Press enter..."
	exit 1
	;;
esac


read -p "fell through field! press enter..."
exit 1
