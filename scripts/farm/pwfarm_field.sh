#!/bin/bash

MODE="$1"

function msg()
{
    echo "pwfarm_field.sh: $*"
}

function sighandler()
{
    if [ -z "$PWFARM_COMPLETION_FILE" ] ; then
	echo 'SIGINT'>$PWFARM_COMPLETION_FILE
    fi
}


case "$MODE" in
    "create_home")
	cd
	if [ -e pwfarm ]; then
	    rm -rf pwfarm
	fi
	mkdir pwfarm
	;;

    "unpack_payload")
	cd
	mv pwfarm_payload.zip pwfarm
	mv pwfarm_field.sh pwfarm
	cd pwfarm
	mkdir -p payload
	unzip -o pwfarm_payload.zip -d payload
	;;

    "launch")
	trap sighandler 2

	export PWFARM_ID="$2"
	export PWFARM_HOME="$3"
	export PWFARM_COMPLETION_FILE="$4"
	export PWFARM_COMMAND="$5"
	
	#
	# Try resuming from a disconnect
	#
	if screen -S pwfarm -r; then
	    msg RESUMED SCREEN
	    if [ ! -e $PWFARM_COMPLETION_FILE ] ; then
		echo 'Screen crashed' > $PWFARM_COMPLETION_FILE
	    fi
	    exit 0
	fi
    
	#
	# Maybe screen already finished, and completion file exists?
	#
	if [ -e $PWFARM_COMPLETION_FILE ]; then
	    msg "Completion file exists!"
	    exit 0
	fi

	msg "Creating Screen session"

	screen -S pwfarm bash -c "$0 command"

	if [ ! -e $PWFARM_COMPLETION_FILE ] ; then
	    # This is a serious problem because if the completion file
	    # isn't created in the screen session then we've got a race
	    # condition.
	    #
	    # We should be capturing any errors within the "command" mode
	    # that executes within screen.
	    msg "Fatal error! Creating completion file outside of screen!"
	    echo "Screen Crash" > $PWFARM_COMPLETION_FILE
	fi

	exit 0
	;;

    "command")
	trap sighandler 2

	export PATH=$PATH:/usr/local/bin:/Library/Frameworks/Python.framework/Versions/Current/bin

	msg invoking _"$PWFARM_COMMAND"_

	cd "$PWFARM_HOME/payload"
	pwd
	ls

	bash -c "$PWFARM_COMMAND"
	exitval=$?

	if [ $exitval == 0 ] ; then
	    echo 'yay'>$PWFARM_COMPLETION_FILE
	else
	    echo $?>$PWFARM_COMPLETION_FILE
	    echo "press enter..." ; read
	fi

	exit 0
	;;

    *)
	msg "Invalid mode: $MODE"
	exit 1
	;;
esac
