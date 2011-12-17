#!/bin/bash

source $( dirname $BASH_SOURCE )/__lib.sh || exit 1

validate_farm_env

DISPATCHERSTATE_DIR=$( pwenv dispatcherstate_dir ) || exit 1
require "$DISPATCHERSTATE_DIR" "dispatcher dir cannot be empty!!!"
mkdir -p "$DISPATCHERSTATE_DIR" || exit 1

export DISPATCHER_SCREEN_SESSION="____pwfarm_dispatcher__farm_$( pwenv farmname )__session_$( pwenv sessionname )____"
MUTEX=$DISPATCHERSTATE_DIR/mutex
PARMS=$DISPATCHERSTATE_DIR/parms
PID=$DISPATCHERSTATE_DIR/pid
FIELDNUMBERS=$DISPATCHERSTATE_DIR/fieldnumbers

BLOB_DIR=${DISPATCHERSTATE_DIR}/blob
BLOB_LOCAL=${BLOB_DIR}/blob.zip
BLOB_REMOTE="~/__pwfarm_blob__user_$( pwenv pwuser )__farm_$( pwenv farmname )__session_$( pwenv sessionname ).zip"
BROADCAST_COMPLETE=$DISPATCHERSTATE_DIR/broadcast_complete
FIELD_BORN=$DISPATCHERSTATE_DIR/field_born

PWUSER=$( pwenv pwuser )
OSUSER=$( pwenv osuser )

function screen_active()
{
    screen -ls | grep "\\b${DISPATCHER_SCREEN_SESSION}\\b" > /dev/null
}

function init_screen()
{
    screen -d -m -S "${DISPATCHER_SCREEN_SESSION}"
}

function resume_screen()
{
    screen -r -S "${DISPATCHER_SCREEN_SESSION}"
}

function kill_screen()
{
    while screen_active; do
	screen -S "${DISPATCHER_SCREEN_SESSION}" -X quit
	sleep 1
    done
}

if [ "$1" == "--password" ]; then
    shift
    prompt_password="true"
else
    prompt_password="false"
fi

if [ "$1" == "--noprompterr" ]; then
    shift
    prompt_err="false"
else
    prompt_err="true"
fi

broadcast="false"

mode="$1"

###
### Mode-specific logic prior to interfacing with field nodes
###
case "$mode" in
    "dispatch")
	payload=$( canonpath "$2" )
	command="$3"
	output_basename="$4"
	output_dir=$( canonpath "$5" )

	mutex_lock $MUTEX

	if [ -e "$PARMS" ]; then
	    mutex_unlock $MUTEX
	    err "You're either already running a task in this session or need to 'clear/recover'. (Exists: $PARMS)"
	fi
	if screen_active ; then
	    mutex_unlock $MUTEX
	    err "You must be running a task in this session. (Dispatcher screen already active!)"
	fi
	if ! init_screen; then
	    mutex_unlock $MUTEX
	    err "Failed initing dispatcher screen!"
	fi

	rm -f $BROADCAST_COMPLETE

	(
	    echo $prompt_password
	    echo $prompt_err
	    echo $payload
	    echo $command
	    echo $output_basename
	    echo $output_dir
	) > $PARMS

	echo $$ > $PID
	echo $( pwenv fieldnumbers ) > $FIELDNUMBERS

	broadcast="true"

	mutex_unlock $MUTEX
	;;
    "recover")
	mutex_lock $MUTEX

	if [ ! -e "$PARMS" ]; then
	    mutex_unlock $MUTEX
	    err "Can't find task to recover. Is this the right session? (Does not exist: $PARMS)"
	fi
	if [ ! -e "$FIELDNUMBERS" ]; then
	    mutex_unlock $MUTEX
	    err "Can't find field numbers to recover. (Does not exist: $FIELDNUMBERS)"
	fi
	if screen_active ; then
	    mutex_unlock $MUTEX
	    err "Dispatcher screen already active! You must be running a task in this session."
	fi
	if [ -e "$PID" ]; then
	    if ps -e | grep "\\b$( cat $PID )\\b"; then
		mutex_unlock $MUTEX
		err "Dispatcher already alive!"
	    fi
	fi
	if ! init_screen; then
	    mutex_unlock $MUTEX
	    err "Failed initing dispatcher screen!"
	fi

	function parm()
	{
	    head -n $(( $1 + 1 )) "$PARMS" | tail -n 1
	}

	prompt_password=$( parm 0 )
	prompt_err=$( parm 1 )
	payload=$( parm 2 )
	command=$( parm 3 )
	output_basename=$( parm 4 )
	output_dir=$( parm 5 )

	__pwfarm_config env set fieldnumbers $( fieldnums_from_hostnames $(cat $FIELDNUMBERS) )

	echo $$ > $PID

	broadcast="true"

	mutex_unlock $MUTEX
	;;
    "clear")
	if [ "$2" == "--exit" ]; then
	    # We're being invoked as part of the exit procedure
	    echo "Performing healthy pwfarm state clear."
	    shift

	    mutex_lock $MUTEX;
	else
	    # This is an abort/force-clear operation.
	    echo "Forcing pwfarm state clear. This could take a while."

	    # If we can't get the mutex, we still need to keep going.
	    mutex_trylock $MUTEX;

	    if [ -e $PID ]; then
		pid=$( cat $PID )
		kill $pid  2>/dev/null
	    fi

	    kill_screen

	    # Force clear of all machines on farm.
	    pwquery fieldnumbers $(pwenv farmname) > $FIELDNUMBERS
	fi
	rm -f "$PARMS"
	rm -f "$PID"
	rm -f "$BROADCAST_COMPLETE"
	rm -rf "$BLOB_DIR"
	;;
    "disconnect")
	mutex_lock $MUTEX;

	if [ "$2" != "--exit" ]; then
	    if is_process_alive $PID; then
		echo "Killing dispatcher process..." >&2
		pid=$( cat $PID )
		kill $pid  2>/dev/null
	    else
		echo "No dispatcher process detected" >&2
	    fi
	fi

	if screen_active ; then
	    echo "Killing dispatcher screen..." >&2
	    kill_screen
	else
	    echo "No dispatcher screen detected" >&2
	fi
	;;
    "exit")
	succeeded=""
	failed=""
	exitval=0
	;;
    *)
	err "Invalid mode: $mode"
	;;
esac

###
### Prompt user for sudo password if needed
###
if $prompt_password; then
    PASSWORD=""
    
    while [ -z "$PASSWORD" ]; do
        # turn off echo for reading password
	stty -echo
	read -p "Please enter password of administrator on farm machines (for sudo): " PASSWORD
	echo
        # turn echo back on
	stty echo
    done
else
    PASSWORD="nil"
fi

###
### Broadcast blob to field nodes if needed
###
if $broadcast; then
    if [ ! -e "$BROADCAST_COMPLETE" ]; then
	require "$BLOB_DIR" "blob dir can't be null!"
	rm -rf "$BLOB_DIR"

	set -e
	mkdir -p "$BLOB_DIR"
	pushd_quiet .
	cd "$BLOB_DIR"
        mkdir payload
	cp "$payload" payload/payload.zip
	mkdir scripts
	cp "$PWFARM_SCRIPTS_DIR/__lib.sh" scripts
	cp "$PWFARM_SCRIPTS_DIR/__pwfarm_field.sh" scripts
	cp "$PWFARM_SCRIPTS_DIR/__pwfarm_runutil.sh" scripts
	cp "$PWFARM_SCRIPTS_DIR/__pwfarm_config.sh" scripts
	cp "$PWFARM_SCRIPTS_DIR/__pwfarm_status.py" scripts
	zip -qr "$BLOB_LOCAL" *
	popd_quiet
	set +e

	#
	# Broadcast blob to fields
	#
	$PWFARM_SCRIPTS_DIR/__pwfarm_broadcast.sh "$BLOB_LOCAL" "$BLOB_REMOTE"

	touch "$BROADCAST_COMPLETE"
    fi
fi

###
### Perform task on all field nodes
###
screen_window=1

if [ -e $FIELDNUMBERS ]; then
    for fieldnumber in $( cat $FIELDNUMBERS ); do

	fieldhostname=$( fieldhostname_from_num $fieldnumber )

	FARMER_SH="$PWFARM_SCRIPTS_DIR/__pwfarm_farmer.sh"

	if [ "$mode" == "dispatch" ] || [ "$mode" == "recover" ]; then
	    rm -f $FIELD_BORN

	    title="$fieldhostname - $command"
	    screen -S "${DISPATCHER_SCREEN_SESSION}" -X screen -t "$title" \
		"$FARMER_SH" \
		$fieldnumber \
                $mode \
		$screen_window \
		$FIELD_BORN \
                "${BLOB_REMOTE}" \
                "$PASSWORD" \
		"$prompt_err" \
		"$command" \
                "$output_basename" \
                "$output_dir"

	    while [ ! -e $FIELD_BORN ]; do
		sleep 0.1
	    done

	    screen_window=$(( $screen_window + 1 ))
	else
	    case "$mode" in 
		"clear")
		    echo "   [ Clearing $fieldnumber (farm=$(pwenv farmname), session=$(pwenv sessionname)) ]"
		    ( $FARMER_SH $fieldnumber clear )&
		    ;;

		"disconnect")
		    echo "   [ Disconnecting $fieldnumber... ]"
		    $FARMER_SH $fieldnumber disconnect
		    ;;

		"exit")
		    if ! $FARMER_SH $fieldnumber exit; then
			failed="${failed}$( fieldhostname_from_num $fieldnumber ) "
			exitval=1
		    else
			succeeded="${succeeded}$( fieldhostname_from_num $fieldnumber ) "
		    fi
		    ;;
		*)
		    err "Invalid mode: $mode"
		    ;;
	    esac
	fi
    done
fi

###
### Mode-specific logic after interfacing with field nodes
###
if [ "$mode" == "dispatch" ] || [ "$mode" == "recover" ]; then
    # kill window 0... it just has bash
    screen -S "${DISPATCHER_SCREEN_SESSION}" -p 0 -X kill
    # bring up screen, starting at the windowlist
    screen -S "${DISPATCHER_SCREEN_SESSION}" -p = -r
    while screen_active; do
	echo
	echo "You have detached the dispatcher screen."
	echo
        echo "  (r)esume"
	echo "  (c)lear"
	echo "  (d)isconnect"
	echo
	if read -p "Enter command: " -t 10 choice; then
	    case "$choice" in
		c)
		    $0 clear --exit
		    reset
		    exit 1
		    ;;
		d)
		    $0 disconnect --exit
		    reset
		    exit 1
		    ;;
	    esac
	fi

	resume_screen
    done

    $0 "exit"
else
    case "$mode" in
	"clear")
	    wait
	    rm -f "$FIELDNUMBERS"
	    mutex_unlock $MUTEX
	    ;;

	"disconnect")
	    mutex_unlock $MUTEX
	    echo "dispatcher disconnect complete." >&2
	    ;;

	"exit")
	    $0 "clear" --exit
	    echo -n "pwfarm task complete. SUCCESS="
	    if [ "$exitval" == "0" ]; then
		echo "TRUE"
	    else
		echo "FALSE"

		if [ ! -z "$succeeded" ]; then
		    echo "succeeded=$succeeded"
		fi
		if [ ! -z "$failed" ]; then
		    echo "failed=$failed"
		fi
	    fi

	    exit $exitval
	    ;;
    esac
fi