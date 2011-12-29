#!/bin/bash

if [ -z "$PWFARM_SCRIPTS_DIR" ]; then
    source $( dirname $BASH_SOURCE )/__pwfarm_runutil.sh || exit 1
else
    source $PWFARM_SCRIPTS_DIR/__pwfarm_runutil.sh || exit 1
fi

function usage()
{
    cat >&2 <<EOF
usage: $( basename $0 ) [a:o:] run_id_driven run_id_passive

ARGS:

   run_id_driven   Run ID of existing driven run.

   run_id_passive  Run ID of passive run to be executed.

OPTIONS:

   -a analysis_script
                  Path of script that is to be executed after simulation.

   -o run_owner
                  Specify owner of run, which is ultimately prepended to run IDs.
                A value of "nil" indicates that no owner will be prepended.
EOF

    if [ ! -z "$1" ]; then
	echo >&2
	err $*
    fi

    exit 1
}

self=$( basename $0 )

if [ "$self" == "prerun.sh" ]; then
    # we're running on the field

    run_id_driven=$( cat $POLYWORLD_PWFARM_INPUT/run_id_driven )
    run_dir=$( stored_run_path_field "good" "$run_id_driven" )

    if [ ! -d $run_dir ]; then
	err "Cannot locate run at $run_dir"
    fi

    cp $run_dir/BirthsDeaths.log ./LOCKSTEP-BirthsDeaths.log || exit 1
    cp $run_dir/normalized.wf $POLYWORLD_PWFARM_WORLDFILE || exit 1
    ./scripts/wfutil edit $POLYWORLD_PWFARM_WORLDFILE PassiveLockstep=True || err "Failed setting PassiveLockstep property"
else
    if [ $# == 0 ]; then
	usage
    fi

    postrun_script="nil"
    owner=$( pwenv pwuser )
    owner_override=false

    while getopts "a:o:" opt; do
	case $opt in
	    a)
		postrun_script="$OPTARG"
		;;
	    o)
		owner="$OPTARG"
		owner_override=true
		;;
	    *)
		exit 1
		;;
	esac
    done

    shift $(( $OPTIND - 1 ))

    if [ $# != 2 ]; then
	usage "Invalid number of parms"
    fi

    run_id_driven="$1"
    run_id_passive="$2"

    tmp_dir=`mktemp -d /tmp/poly_passive.XXXXXXXX` || exit 1
    build_runid "$owner" "$run_id_driven" > $tmp_dir/run_id_driven

    pushd_quiet .
    cd $tmp_dir
    zip -qr input.zip .
    popd_quiet

    opts=""
    if [ "$postrun_script" != "nil" ]; then
	opts="$opts -a $postrun_script"
    fi
    if $owner_override; then
	opts="$opts -o $owner"
    fi

    $PWFARM_SCRIPTS_DIR/pwfarm_run.sh $opts -c "$0" -z "$tmp_dir/input.zip" "$run_id_passive"
fi