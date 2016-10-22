#!/bin/bash

source $( dirname $BASH_SOURCE )/__lib.sh || exit 1


FIELD_NUMBER="$1"
require "$FIELD_NUMBER" "farmer field number"
__pwfarm_config env set fieldnumber $FIELD_NUMBER

FIELD_HOSTNAME=$( fieldhostname_from_num $FIELD_NUMBER )
FIELD_HOST=$( fieldhost_from_name $FIELD_HOSTNAME )
FIELD_SCREEN_SESSION="$( pwenv fieldscreensession )"

FARMER_STATE_DIR=$( pwenv farmerstate_dir ) || exit 1
require "$FARMER_STATE_DIR" "farmer dir cannot be empty!!!"
mkdir -p $FARMER_STATE_DIR
cd $FARMER_STATE_DIR
FARMER_TASKMETA=$FARMER_STATE_DIR/task

FARM_NAME=$( pwenv farmname )
SESSION_NAME=$( pwenv sessionname )

FIELD_STATE_DIR="$( pwenv fieldstate_dir )" || exit 1
require "$FIELD_STATE_DIR" "field dir cannot be empty!!!"

OSUSER=$( pwenv osuser )

STEPS=$FARMER_STATE_DIR/steps
PID=$FARMER_STATE_DIR/pid

# macro that specifies user, host and a server timeout
SSH="ssh -l $OSUSER -o ServerAliveInterval=30 $FIELD_HOST"

mode="$2"
require "$mode" "farmer mode"


case "$mode" in
    "dispatch")
	# no-op... fall through
	;;
    "recover")
	# no-op... fall through
	;;
    "clear")
	BLOB="$3"

	repeat_til_success $SSH "
          if [ -e $FIELD_STATE_DIR/pid ]; then
            kill \$( cat $FIELD_STATE_DIR/pid ) 2>/dev/null ;
          fi ;
          if [ -e $FIELD_STATE_DIR/pid_command ]; then
            kill \$( cat $FIELD_STATE_DIR/pid_command ) 2>/dev/null;
          fi ;
          if [ -e $FIELD_STATE_DIR/pid_status ]; then
            kill \$( cat $FIELD_STATE_DIR/pid_status ) 2>/dev/null;
          fi ;
          screen -ls | grep \"$FIELD_SCREEN_SESSION\" | grep -o '^[[:space:]]\+[0-9]\+' 2>/dev/null > $FIELD_STATE_DIR/pid_screen ;
          if [ ! -z \$( cat $FIELD_STATE_DIR/pid_screen ) ]; then
            kill \$( cat $FIELD_STATE_DIR/pid_screen ) 2>/dev/null;
          fi ;
          rm -f $BLOB ;
          rm -rf $FIELD_STATE_DIR"
	if is_process_alive $PID; then
	    kill $( cat $PID )
	fi
	rm -rf $FARMER_STATE_DIR

	exit 0
	;;
    "disconnect")
	if is_process_alive $PID; then
	    kill $( cat $PID )
	    rm $PID
	fi
	exit 0
	;;
    *)
	err "Invalid farmer mode ($mode)"
	;;
esac

################################################################################
#####
##### DISPATCH / RECOVER
#####
################################################################################

SCREEN_WINDOW="$3"
BLOB="$4"

# If a farmer is already alive, then kill it.
if is_process_alive $PID; then
    kill $( cat $PID )
fi
echo $$ > $PID

#
# Util functions for maintaining steps file
#
function step_begin()
{
    __step=$1

    ! grep "^$__step$" "$STEPS" 1>/dev/null
}

function step_done()
{
    echo "$__step" >> "$STEPS"
}

TITLE_PREFIX="$FARM_NAME/$SESSION_NAME/$FIELD_HOSTNAME"
TITLE_PREFIX_DETAILS=""

function title()
{
    screen -S "${DISPATCHER_SCREEN_SESSION}" -p $SCREEN_WINDOW -X title "${TITLE_PREFIX}${TITLE_PREFIX_DETAILS}: $*"
}


###
### Loop as long as we get a task assignment
###
while true; do
    touch $STEPS

    TITLE_PREFIX_DETAILS=""
    
    #
    # Try to get task assignment
    #
    title "Get Task Assignment"

    if step_begin "task_get"; then
	if ! $PWFARM_SCRIPTS_DIR/__pwfarm_dispatcher.sh task_get $FARMER_TASKMETA; then
	    break
	fi

	step_done
    fi

    if taskmeta has $FARMER_TASKMETA statusid; then
	TITLE_PREFIX_DETAILS="/$(taskmeta get $FARMER_TASKMETA statusid)"
    fi

    echo "--- Assigned Task ---"
    cat $FARMER_TASKMETA
    echo "---------------------"

    __pwfarm_config env set taskid $( taskmeta get $FARMER_TASKMETA id )

    #
    # Init field state dir
    #
    title "Init Field"

    if step_begin "init_statedir"; then
	repeat_til_success $SSH "
          rm -rf $FIELD_STATE_DIR ;
          mkdir -p $FIELD_STATE_DIR ;
          cd $FIELD_STATE_DIR ;
          $( archive unpack -e $BLOB ) ;
          cd payload ;
          $( archive unpack -e payload.tbz ) ;
          rm payload.tbz ;
        "
	step_done
    fi

    #
    # Launch Status Background Process
    #
    function status_background_process()
    {
	if [ "$mode" == "dispatch" ]; then
	    failstate=false
	else
	    failstate=true
	fi

	function get_status()
	{
	    local op=$1
	    status=$( $SSH "
                        cd $FIELD_STATE_DIR > /dev/null ;
                        scripts/__pwfarm_status.py status_state $op
                      " 2>/dev/null )

	    if [ $? != 0 ]; then
		failstate=true
		status="CONNECTION FAILURE"
	    else
		failstate=false
	    fi
	}

	while true; do
	    if $failstate; then
		get_status "get"
	    else
		get_status "wait"
	    fi

	    if [ "$status" == $( $PWFARM_SCRIPTS_DIR/__pwfarm_status.py quit_signature ) ]; then
		break
	    fi

	    title $status

	    if $failstate; then
		sleep 5
	    fi
	done
    }

    # in case we die abnormally, kill status background
    kill_jobs_on_termination

    status_background_process &

    #
    # Launch Field
    #
    title "Launching"

    idomain=$( pwenv domains | grep -n "\b$FIELD_NUMBER\b" | cut -d ':' -f 1 )
    eval password="\$PASSWORD_$idomain"

    if step_begin "run_script"; then	
        # set --display so we don't echo password to console
	repeat_til_success \
	    --display "ssh __pwfarm_field.sh launch" \
	    $SSH -t "
              export PASSWORD=\"$password\" ;
              $( $PWFARM_SCRIPTS_DIR/pwfarm_config.sh env export )
              cd $FIELD_STATE_DIR ;
              scripts/__pwfarm_field.sh launch
            "

	step_done
    fi
    
    # Kill status background.
    kill $! > /dev/null 2>/dev/null

    #
    # Fetch result file
    #
    title "Downloading Result"

    if step_begin "fetch_result"; then
	repeat_til_success scp $OSUSER@$FIELD_HOST:$FIELD_STATE_DIR/result.tbz $FARMER_STATE_DIR

	step_done
    fi

    #
    # Unpack result file
    #
    if step_begin "unpack_result"; then
	if [ -e result.tbz ]; then
	    rm -rf result
	    archive unpack -d result result.tbz
	    rm result.tbz
	fi

	step_done
    fi

    #
    # Save log file
    #
    if step_begin "save_log"; then
	if [ -e result/log ]; then
	    cp result/log /tmp/log_task$(pwenv taskid)_field${FIELD_NUMBER}_session$(pwenv sessionname)_farm$(pwenv farmname)
	fi

	step_done
    fi

    #
    # Unpack output file
    #
    if step_begin "unpack_output"; then
	if [ -e result/output.tbz ] && taskmeta has $FARMER_TASKMETA outputdir; then
	    outputdir=$( taskmeta get $FARMER_TASKMETA outputdir )
	    archive unpack -d $outputdir result/output.tbz
	    rm result/output.tbz
	fi

	step_done
    fi

    #
    # Inform dispatcher we're done with this task
    #
    if step_begin "task_done"; then
	cp result/taskmeta $FARMER_TASKMETA
	if ! taskmeta has $FARMER_TASKMETA exitval; then
	    taskmeta set $FARMER_TASKMETA exitval 1
	fi

	$PWFARM_SCRIPTS_DIR/__pwfarm_dispatcher.sh task_done $FARMER_TASKMETA

	step_done
    fi

    rm $STEPS
done

rm $PID

exit