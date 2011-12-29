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
usage: $( basename $0 ) [-F:f:o:] runid|/

    Fetch runs with a Run ID that starts with runid, which is to say this can
  operate on a hierarchy of runs. You may use wildcards, but use quotes.

OPTIONS:

   -F fetch_list
                  Files to be pulled back from run. For example:

                    -F "stat/* *.wf"

                  By default, the following is fetched:

                    $DEFAULT_FETCH_LIST

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
    field=true
    shift
else
    field=false
fi

FETCH_LIST="$DEFAULT_FETCH_LIST"
OWNER=$( pwenv pwuser )

while getopts "F:f:o:" opt; do
    case $opt in
	F)
	    FETCH_LIST="$OPTARG"
	    ;;
	f)
	    if ! $field; then
		__pwfarm_config env set fieldnumbers "$OPTARG"
		validate_farm_env
	    fi
	    ;;
	o)
	    OWNER="$OPTARG"
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

if ! $field; then
    ###
    ### EXECUTE ON LOCAL
    ###
    validate_farm_env

    PAYLOAD_DIR=$TMPDIR/payload
    CHECKSUMS_DIR=$PAYLOAD_DIR/checksums

    mkdir -p $PAYLOAD_DIR
    mkdir -p $CHECKSUMS_DIR

    #
    # Create checksums for local data.
    #
    find_runs_local "$OWNER" "$RUNID" |
    while read rundir; do
	runid=$( parse_stored_run_path_local --runid $rundir )
	nid=$( parse_stored_run_path_local --nid $rundir )

	mkdir -p $CHECKSUMS_DIR/$runid/

	$POLYWORLD_SCRIPTS_DIR/archive_delta.sh checksums \
	    $CHECKSUMS_DIR/$runid/$nid \
	    $rundir \
	    "$FETCH_LIST"
    done

    #
    # Create Payload
    #
    PAYLOAD=$TMPDIR/payload.zip

    pushd_quiet .
    cd $PAYLOAD_DIR
    zip -rq $PAYLOAD .
    popd_quiet

    #
    # Execute on farm
    #
    __pwfarm_script.sh --input $PAYLOAD --output "$TMPDIR/result" $0 --field $ARGS || exit 1

    #
    # Unpack fetched data
    #
    for resultdir in $TMPDIR/result*; do
	cat $resultdir/runs |
	while read line; do
	    runid=$( echo "$line" | cut -f 1 )
	    nid=$( echo "$line" | cut -f 2 )
	    archive=$resultdir/$( echo "$line" | cut -f 3 )

	    rundir=$( stored_run_path_local $OWNER $runid $nid )
	    mkdir -p $rundir

	    unzip -oq -d $rundir $archive
	done
    done
else
    ###
    ### EXECUTE ON REMOTE
    ###
    PAYLOAD_DIR=$PWD

    touch $TMPDIR/runs

    #
    # For each run matching runid, package up files not on local.
    #
    for rundir in $(find_runs_field "good" $OWNER "$RUNID"); do
	runid=$( parse_stored_run_path_field --runid  $rundir )
	nid=$( parse_stored_run_path_field --nid  $rundir )

	mkdir -p $TMPDIR/$runid/$nid

	archive=$runid/$nid/run.zip

	$POLYWORLD_PWFARM_SCRIPTS_DIR/archive_delta.sh archive \
	    -e $PAYLOAD_DIR/checksums/$runid/$nid \
	    $TMPDIR/$archive \
	    $rundir \
	    "$FETCH_LIST .pwfarm/*"

	printf "${runid}\t${nid}\t${archive}\n" >> $TMPDIR/runs
    done

    cd $TMPDIR
    zip -qr $PWFARM_OUTPUT_FILE .
fi

rm -rf $TMPDIR