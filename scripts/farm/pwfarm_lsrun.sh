#!/bin/bash

if [ -z "$PWFARM_SCRIPTS_DIR" ]; then
    source $( dirname $BASH_SOURCE )/__pwfarm_runutil.sh || exit 1
else
    source $PWFARM_SCRIPTS_DIR/__pwfarm_runutil.sh || exit 1
fi

function usage()
{    
################################################################################
    cat <<EOF
usage: $( basename $0 ) [-f:o:] runid|/

    Lists runs with a Run ID that starts with runid, which is to say it can
  list a hierarchy of runs. You may use wildcards, but use quotes.

OPTIONS:

    -f fields
               Specify fields on which this should run. Must be a single
            argument, so use quotes. e.g. -f "0 1" or -f "{0..3}"

    -o run_owner
               Specify run owner. When used, orphan runs aren't listed.
EOF
    exit 1
}

if [ $# == 0 ]; then
    usage
fi

if [ "$1" == "--field" ]; then
    FIELD=true
    shift
else
    FIELD=false
fi

OWNER=$( pwenv pwuser )
OWNER_OVERRIDE=false

while getopts "f:o:" opt; do
    case $opt in
	f)
	    if ! $FIELD; then
		__pwfarm_config env set fieldnumbers "$OPTARG"
		validate_farm_env
	    fi
	    ;;
	o)
	    OWNER="$OPTARG"
	    OWNER_OVERRIDE=true
	    ;;
	*)
	    exit 1
	    ;;
    esac
done

ARGS=$( encode_args "$@" )
shift $(( $OPTIND - 1 ))
if [ $# != 1 ]; then
    usage
fi

RUNID="$( normalize_runid "$1" )"

TMPDIR=$( create_tmpdir ) || exit 1

if ! $FIELD; then
    validate_farm_env

    __pwfarm_script.sh --output "$TMPDIR/result" $0 --field $ARGS || exit 1

    for num in $( pwenv fieldnumbers ); do
	echo ----- $( fieldhostname_from_num $num ) -----

	cat "$TMPDIR/result_$num/out"
    done
else
    if ! $OWNER_OVERRIDE; then
	if [ -e "$POLYWORLD_PWFARM_APP_DIR/run" ] && [ ! -L "$POLYWORLD_PWFARM_APP_DIR/run" ]; then
	    if lock_app; then
		echo WARNING! Contains $(is_good_run "$POLYWORLD_PWFARM_APP_DIR/run" && echo "good" || echo "failed") orphan run! runid=$( cat "$POLYWORLD_PWFARM_APP_DIR/runid" ) nid=$( cat "$POLYWORLD_PWFARM_APP_DIR/nid" )>> $TMPDIR/out 2>&1
		unlock_app
	    else
		echo Simulation running. runid=$( cat "$POLYWORLD_PWFARM_APP_DIR/runid" ) >> $TMPDIR/out 2>&1
	    fi		
        fi
    fi

    function dols()
    {
	local status="$1"

	# find run directories
	find_runs_field $status $OWNER "$RUNID" > $TMPDIR/runs

	local runids=$(
	    cat $TMPDIR/runs |
	    parse_stored_run_path_field --runid |
	    sort |
	    uniq
	)

	if [ "$status" == "failed" ] && [ ! -z "$runids" ]; then
	    echo "FAILED:" >> $TMPDIR/out
	fi

	for x in $runids; do
	    nids=$(
		cat $TMPDIR/runs |
		while read run; do
		    if [ "$x" == "$(parse_stored_run_path_field --runid $run)" ]; then
			parse_stored_run_path_field --nid $run
		    fi
		done |
		sort -n |
		tr "\n" " "
	    )
	    echo "$x  ;  NIDs = $nids" >> $TMPDIR/out
	done
    }

    dols "good"
    dols "failed"

    if [ ! -e $TMPDIR/out ]; then
	echo "NO SUCH RUN" > $TMPDIR/out
    fi

    cd $TMPDIR
    archive pack $PWFARM_OUTPUT_ARCHIVE out
fi

rm -rf $TMPDIR