#!/bin/bash

PWFARM_FARMER_STATUS="$1"
PWFARM_FIELD_HOST="$2"
PWFARM_FIELD_ID="$3"
PWFARM_FIELD_USER="$4"
PWFARM_PAYLOAD="$5"
PWFARM_PASSWORD="$6"
PWFARM_COMMAND="$7"
PWFARM_OUTPUT_BASENAME="$8"
PWFARM_OUTPUT_DIR="$9"
PWFARM_FIELD_COMPLETION="${10}"


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

# ssh macro that specifies user and a server timeout
ssh="ssh -l $PWFARM_FIELD_USER -o ServerAliveInterval=60"

PWFARM_FIELD_COMPLETION__REMOTE="~/pwfarm/`basename $PWFARM_FIELD_COMPLETION`"
PWFARM_FIELD_OUTPUT_EXISTS__REMOTE="~/pwfarm/output_exists"
PWFARM_FIELD_OUTPUT_EXISTS__LOCAL="${PWFARM_FARMER_STATUS}_output_exists"
PWFARM_FIELD_OUTPUT__REMOTE="~/pwfarm/${PWFARM_OUTPUT_BASENAME}.zip"
PWFARM_FIELD_OUTPUT__LOCAL="${PWFARM_OUTPUT_DIR}/${PWFARM_OUTPUT_BASENAME}_${PWFARM_FIELD_ID}.zip"

#
# Util functions for maintaining farmer status file
#
function step_begin()
{
    step=$1

    #echo "Begin step $step"
    #echo "press enter..."; read

    ! grep "^$step$" "$PWFARM_FARMER_STATUS" 1>/dev/null
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
	sleep 5
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
# Create home
#
if step_begin "create_home"; then
    try $ssh $PWFARM_FIELD_HOST "~/pwfarm_field.sh" create_home

    step_done
fi


#
# Unpack payload & move run script
#
if step_begin "unpack_payload"; then
    try $ssh $PWFARM_FIELD_HOST "~/pwfarm_field.sh" unpack_payload

    step_done
fi

#
# Run script
#
if step_begin "run_script"; then
    try $ssh -t $PWFARM_FIELD_HOST "~/pwfarm/pwfarm_field.sh launch $PWFARM_FIELD_ID ~/pwfarm $PWFARM_FIELD_COMPLETION__REMOTE $PWFARM_FIELD_OUTPUT_EXISTS__REMOTE $PWFARM_FIELD_OUTPUT__REMOTE $PWFARM_PASSWORD \"$PWFARM_COMMAND\""
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

	try $ssh $PWFARM_FIELD_HOST "rm -f $src"
    fi
}

#
# Fetch completion file
#
if step_begin "fetch_completion"; then
    pwfarm_mv $PWFARM_FIELD_USER@$PWFARM_FIELD_HOST:$PWFARM_FIELD_COMPLETION__REMOTE $PWFARM_FIELD_COMPLETION

    step_done
fi

#
# Fetch output_exists file
#
if step_begin "fetch_output_exists"; then
    pwfarm_mv $PWFARM_FIELD_USER@$PWFARM_FIELD_HOST:$PWFARM_FIELD_OUTPUT_EXISTS__REMOTE $PWFARM_FIELD_OUTPUT_EXISTS__LOCAL

    step_done
fi

#
# Fetch output file
#
if step_begin "fetch_output"; then

    cat $PWFARM_FIELD_OUTPUT_EXISTS__LOCAL

    if [ "$( cat $PWFARM_FIELD_OUTPUT_EXISTS__LOCAL )" == "True" ]; then
	unzipdir="${PWFARM_OUTPUT_DIR}/${PWFARM_OUTPUT_BASENAME}_${PWFARM_FIELD_ID}"
	mkdir -p $unzipdir

	pwfarm_mv $PWFARM_FIELD_USER@$PWFARM_FIELD_HOST:$PWFARM_FIELD_OUTPUT__REMOTE $PWFARM_FIELD_OUTPUT__LOCAL

	unzip -o -d $unzipdir $PWFARM_FIELD_OUTPUT__LOCAL
	rm $PWFARM_FIELD_OUTPUT__LOCAL
    fi

    step_done
fi

if [ ! "`cat $PWFARM_FIELD_COMPLETION`" == "yay" ] ; then
    exit 1
else
    exit 0
fi

