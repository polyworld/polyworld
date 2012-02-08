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
usage: $( basename $0 ) [-tglf:o:] run_id|/

    Deletes runs with a Run ID that starts with runid, which is to say it can
  list a hierarchy of runs. You may use wildcards, but use quotes. By default,
  operates on failed runs.

    When deleting good runs, local data will also be deleted, unless -l is
  specified.

OPTIONS:

    -t         Don't delete anything, just show what data would be operated on.

    -g         Operate on good runs.

    -l         Don't delete local data when deleting good runs.

    -f fields
               Specify fields on which this should run. Must be a single
            argument, so use quotes. e.g. -f "0 1" or -f "{0..3}"

    -o run_owner
               Specify run owner. When used, orphan runs aren't deleted.
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

TESTING=false
GOOD=false
LOCAL=true
OWNER=$( pwenv pwuser )
OWNER_OVERRIDE=false

while getopts "tglf:o:" opt; do
    case $opt in
	t)
	    TESTING=true
	    ;;
	g)
	    GOOD=true
	    ;;
	l)
	    LOCAL=false
	    ;;
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

RUNID=$( normalize_runid "$1" )


if ! $FIELD; then
    validate_farm_env

    if [ -z "$RUNID" ] && $GOOD && ! $TESTING; then
	read -p "You are about to delete all good runs! Are you sure? [y/n]: " response
	if [ "$response" != "y" ]; then
	    exit 1
	fi
    fi

    __pwfarm_script.sh $0 --field $ARGS || exit 1

    if $LOCAL && $GOOD; then
	removed_dir=$( stored_run_path_local --subpath $OWNER "$RUNID" )

	if ! any_files_exist "$removed_dir"; then
	    if $TESTING; then
		echo "Nothing to delete locally"
	    fi
	else
	    if $TESTING; then
		echo "Operating on local data at:"
		ls -d1 $removed_dir | indent

		echo
		echo "Which will remove the following local run data:"
		find_runs_local $OWNER "$RUNID" | indent
	    else
		rm -rf $removed_dir
		prune_empty_directories $(dirname $removed_dir)
	    fi
	fi
    fi

else
    if $GOOD; then
	STATUS="good"
    else
	STATUS="failed"
    fi

    if ! $OWNER_OVERRIDE; then
	orphan="$POLYWORLD_PWFARM_APP_DIR/run"
	if [ -e $orphan ] && lock_app; then
	    store_orphan_run $orphan
	    unlock_app
	fi
    fi

    removed_dir=$( stored_run_path_field --subpath $STATUS $OWNER "$RUNID" )

    if ! any_files_exist "$removed_dir"; then
	if $TESTING; then
	    echo "No stored runs to delete"
	fi
    else
	if $TESTING; then
	    echo "Operating on:"
	    ls -d1 $removed_dir | indent

	    echo
	    echo "Which will remove the following runs:"
	    find_runs_field $STATUS $OWNER "$RUNID" | indent
	else
	    rm -rf $removed_dir

	    prune_empty_directories $(dirname $removed_dir)
	fi
    fi

    if $TESTING; then
	echo
	read -p "Press enter..."
    fi
fi