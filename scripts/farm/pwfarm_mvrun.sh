#!/bin/bash

if [ -z "$PWFARM_SCRIPTS_DIR" ]; then
    source $( dirname $BASH_SOURCE )/__pwfarm_runutil.sh || exit 1
else
    source $PWFARM_SCRIPTS_DIR/__pwfarm_runutil.sh || exit 1
fi

function usage()
{    
    cat <<EOF
usage: $( basename $0 ) [-o:] runid_src runid_dst

    Move good runs on farm. The semantics are different than posix 'mv'; this
performs essentially a string substitution. For example, if you had a directory
a/b/c and executed 'mv a/b/c x/y' the result would be 'x/y/c'. If you were to
execute 'fmvrun a/b/c x/y' the result would be 'x/y'.

    Note: You may operate on a hierarchy of runs. For example, if you have runs
'foo/x' and 'foo/y', you may move both in one operation via 'foo'.

OPTIONS:

    -f fields
               Specify fields on which this should run. Must be a single argument,
            so use quotes. e.g. -f "0 1" or -f "{0..3}"

    -o run_owner
               Specify owner of run.
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
if [ $# != 2 ]; then
    usage
fi

RUNID_SRC="$( normalize_runid "$1" )"
validate_runid --ancestor "$RUNID_SRC"

RUNID_DST="$( normalize_runid "$2" )"
validate_runid --ancestor "$RUNID_DST"

TMP_DIR=$( create_tmpdir ) || exit 1

if ! $FIELD; then
    validate_farm_env

    __pwfarm_script.sh --output "$TMP_DIR/result" $0 --field $ARGS || exit 1

    field_moved=false
    for num in $( pwenv fieldnumbers ); do
	if [ -e "$TMP_DIR/result_$num/moved" ]; then
	    field_moved=true
	    break
	fi
    done

    if ! $field_moved; then
	warn "No runs matching '$RUNID_SRC' found on field machines!"
    fi

    function runids()
    {
	local runid_ancestor="$1"

	find_runs_local $OWNER "$runid_ancestor" |
	parse_stored_run_path_local --runid |
	sort |
	uniq
    }

    runids $RUNID_SRC > $TMP_DIR/runids_src
    if [ -z "$(cat $TMP_DIR/runids_src)" ]; then
	err "No local run data found for $RUNID_SRC"
    fi

    runids $RUNID_DST > $TMP_DIR/runids_dst

    cat $TMP_DIR/runids_src |
    sed "s|^$RUNID_SRC|$RUNID_DST|"  > $TMP_DIR/runids_moved


    sort $TMP_DIR/runids_moved $TMP_DIR/runids_dst |
    uniq -d > $TMP_DIR/runids_conflicting

    if [ ! -z "$(cat $TMP_DIR/runids_conflicting)" ]; then
	echo "Found conflicting Run IDs between source and destination:" 1>&2
	cat $TMP_DIR/runids_conflicting 1>&2
	exit 1
    fi

    paste $TMP_DIR/runids_src $TMP_DIR/runids_moved |
    while read line; do
	runid_src=$(echo "$line" | cut -f 1)
	runid_dst=$(echo "$line" | cut -f 2)

	paths_src="$( stored_run_path_local $OWNER $runid_src "*")"
	path_dst=$( stored_run_path_local --subpath $OWNER $runid_dst )

	mkdir -p $path_dst || exit 1
	mv $paths_src $path_dst || exit 1
    done

    find_results_local $OWNER $RUNID_SRC |
    while read results_dir_src; do
	results_dir_dst=$( echo $results_dir_src | sed "s|$RUNID_SRC|$RUNID_DST|" )
	if [ -e $results_dir_dst ]; then
	    echo "WARNING! Results dir conflict:"
	    echo "  $results_dir_src"
	    echo "  $results_dir_dst"
	else
	    mkdir -p $( dirname $results_dir_dst )
	    mv $results_dir_src $results_dir_dst
	fi
    done

    cat $TMP_DIR/runids_src |
    while read runid_src; do
	prune_empty_directories $( stored_run_path_local --subpath $OWNER $runid_src )
    done
else
    function runids()
    {
	local runid_ancestor="$1"

	find_runs_field "good" $OWNER "$runid_ancestor" |
	parse_stored_run_path_field --runid |
	sort |
	uniq
    }

    runids $RUNID_SRC > ./runids_src
    if [ -z "$(cat ./runids_src)" ]; then
	echo "No matching runs"
	exit 0
    fi
    runids $RUNID_DST > ./runids_dst

    cat ./runids_src |
    sed "s|^$RUNID_SRC|$RUNID_DST|"  > ./runids_moved


    sort ./runids_moved ./runids_dst |
    uniq -d > ./runids_conflicting

    if [ ! -z "$(cat ./runids_conflicting)" ]; then
	echo "Found conflicting Run IDs between source and destination:" 1>&2
	cat ./runids_conflicting 1>&2
	exit 1
    fi

    paste ./runids_src ./runids_moved |
    while read line; do
	runid_src=$(echo "$line" | cut -f 1)
	runid_dst=$(echo "$line" | cut -f 2)

	paths_src="$( stored_run_path_field "good" $OWNER $runid_src "*")"
	path_dst=$( stored_run_path_field --subpath "good" $OWNER $runid_dst )

	mkdir -p $path_dst || exit 1
	mv $paths_src $path_dst || exit 1

	prune_empty_directories $( stored_run_path_field --subpath "good" $OWNER $runid_src )
    done

    cd $TMP_DIR
    touch moved
    archive pack $PWFARM_OUTPUT_ARCHIVE moved
    archive ls $PWFARM_OUTPUT_ARCHIVE
fi

rm -rf $TMP_DIR
