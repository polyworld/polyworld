#!/bin/bash

PWFARM_FARMER_STATUS="$1"
PWFARM_FIELD_HOST="$2"
PWFARM_FIELD_ID="$3"
PWFARM_FIELD_USER="$4"
PWFARM_PAYLOAD="$5"
PWFARM_COMMAND="$6"
PWFARM_FIELD_COMPLETION="$7"


function err()
{
    echo "$*" >&2
    exit 1
}

function canonpath()
{
    python -c "import os.path; print os.path.realpath('$1')"
}

function canondirname()
{
    dirname `canonpath "$1"`
}

PWFARM_FIELD_COMPLETION__REMOTE="~/pwfarm/`basename $PWFARM_FIELD_COMPLETION`"


#
# Util functions for maintaining farmer status file
#
function step_begin()
{
    step=$1

    #echo "Begin step $step"
    #echo "press enter..."; read

    ! grep "$step" "$PWFARM_FARMER_STATUS" 1>/dev/null
}

function step_done()
{
    echo "$step" >> "$PWFARM_FARMER_STATUS"
}


#
# Util function for executing command until it succeeds
#
function try
{
    while ! $*; do
	echo "$0@$PWFARM_FIELD_HOST: FAILED ($*)"
	# give user chance to ctrl-c
	sleep 2
    done
}


##
##
## BEGIN PROCEDURE
##
##

#
# Check for status file.
#
if [ ! -e "$PWFARM_FARMER_STATUS" ]; then
    # This is a new task, so delete the completion file.
    rm -f "$PWFARM_FIELD_COMPLETION"
    touch "$PWFARM_FARMER_STATUS"
fi


#
# Transfer run script to remote machine
#
if step_begin "transfer_run"; then
    try scp `canondirname "$0"`/pwfarm_field.sh "$PWFARM_FIELD_USER@$PWFARM_FIELD_HOST:~/"
    step_done
fi


#
# Transfer payload to remote machine
#
if step_begin "transfer_payload"; then
    try scp "$PWFARM_PAYLOAD" "$PWFARM_FIELD_USER@$PWFARM_FIELD_HOST:~/pwfarm_payload.zip"

    step_done
fi


#
# Create home
#
if step_begin "create_home"; then
    try ssh -l $PWFARM_FIELD_USER $PWFARM_FIELD_HOST "~/pwfarm_field.sh" create_home

    step_done
fi


#
# Unpack payload & move run script
#
if step_begin "unpack_payload"; then
    try ssh -l $PWFARM_FIELD_USER $PWFARM_FIELD_HOST "~/pwfarm_field.sh" unpack_payload

    step_done
fi


#
# Run script
#
if step_begin "run_script"; then
    try ssh -t -l $PWFARM_FIELD_USER $PWFARM_FIELD_HOST "~/pwfarm/pwfarm_field.sh launch $PWFARM_FIELD_ID ~/pwfarm $PWFARM_FIELD_COMPLETION__REMOTE \"$PWFARM_COMMAND\""
    step_done
fi


function pwfarm_mv()
{
    src=$1
    dst=$2

    # todo: will this handle partial copies (due to failure)?
    if [ ! -e $dst ]; then
	try scp $src $dst

	# strip user@host:
	src=`echo $src | sed -e "s/.*://"`

	try ssh -l $PWFARM_FIELD_USER $PWFARM_FIELD_HOST "rm -f $src"
    fi
}

#
# Fetch completion file
#
if step_begin "fetch_completion"; then
    pwfarm_mv $PWFARM_FIELD_USER@$PWFARM_FIELD_HOST:$PWFARM_FIELD_COMPLETION__REMOTE $PWFARM_FIELD_COMPLETION

    step_done
fi

if [ ! "`cat $PWFARM_FIELD_COMPLETION`" == "yay" ] ; then
    echo --- completion ---
    cat $PWFARM_FIELD_COMPLETION
    read
    exit 1
else
    exit 0
fi

