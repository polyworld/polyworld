#!/bin/bash

if [ -z "$PWFARM_SCRIPTS_DIR" ]; then
    source $( dirname $BASH_SOURCE )/__pwfarm_runutil.sh || exit 1
else
    source $PWFARM_SCRIPTS_DIR/__pwfarm_runutil.sh || exit 1
fi

if [ "$1" == "--field" ]; then
    field=true
    shift
else
    field=false
fi

TMPDIR=$( create_tmpdir ) || exit 1

if ! $field; then
    ########################################
    ###
    ### LOCAL MACHINE
    ###
    ########################################
    validate_farm_env

    __pwfarm_script.sh --output "$TMPDIR/result" $0 --field || exit 1

    for num in $( pwenv fieldnumbers ); do
	echo ----- $( fieldhostname_from_num $num ) -----

	cat "$TMPDIR/result_$num/out"
    done

else
    OUT=$TMPDIR/out
    ########################################
    ###
    ### REMOTE MACHINE
    ###
    ########################################
    if [ ! -e "$POLYWORLD_PWFARM_APP_DIR" ]; then
	echo "No app directory!" >> $OUT
    elif ! lock_app; then
	echo "Failed locking app dir!" >> $OUT
    elif [ ! -e "$POLYWORLD_PWFARM_APP_DIR/run" ]; then
	echo "No orphan run." >> $OUT
	unlock_app
    else
	cd $POLYWORLD_PWFARM_APP_DIR
	if is_good_run ./run; then
	    echo -n "Found GOOD orphan." >> $OUT
	else
	    echo -n "Found FAILED orphan." >> $OUT
	fi

	echo " runid=$(cat ./runid) nid=$(cat ./nid)" >> $OUT

	if store_orphan_run ./run; then
	    echo "orphan stored." >> $OUT
	else
	    echo "FAILED STORING ORPHAN!" >> $OUT
	fi
	unlock_app
    fi

    cd $TMPDIR
    archive pack $PWFARM_OUTPUT_ARCHIVE out
fi

rm -rf $TMPDIR