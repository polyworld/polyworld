#!/bin/bash

if [ -z "$PWFARM_SCRIPTS_DIR" ]; then
    source $( dirname $BASH_SOURCE )/__pwfarm_runutil.sh || exit 1
else
    source $PWFARM_SCRIPTS_DIR/__pwfarm_runutil.sh || exit 1
fi

function usage()
{
    cat >&2 <<EOF
usage: $( basename $0 ) [-w:c:a:i:o:] run_id

ARGS:

   run_id         Unique ID for run.

OPTIONS:

   -w worldfile
                  Path of local worldfile to be executed. If not provided, it is
                assumed a run already exists on the farm of run_id.

   -c config_script
                  Path of script that is to be executed prior to running 
                simulation.

   -a analysis_script
                  Path of script that is to be executed after simulation.

   -i input_zip
                  Path of zip file that is to be sent to each machine.

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

if [ -z "$1" ]; then
    usage
fi

if [ "$1" == "--field" ]; then
    ##############################
    ###                        ###
    ### EXECUTE ON REMOTE HOST ###
    ###                        ###
    ##############################

    if [ ! -e "$POLYWORLD_PWFARM_APP_DIR" ]; then
	err "No app directory! Please do a pwfarm_build for this session."
    fi

    PAYLOAD_DIR=$( pwd )
    cd "$POLYWORLD_PWFARM_APP_DIR" || exit 1

    export POLYWORLD_PWFARM_WORLDFILE="$PAYLOAD_DIR/worldfile"
    cp "$PAYLOAD_DIR/runid" .
    RUNID=$( cat $PAYLOAD_DIR/runid )

    export DISPLAY=:0.0 # for Linux -- allow graphics from ssh
    export PATH=$( canonpath scripts ):$PATH
    ulimit -n 4096      # for Mac -- allow enough file descriptors
    ulimit -c unlimited # for Mac -- generate core dump on trap

    ###
    ### If a run is already here, store it away.
    ###
    store_orphan_run ./run

    ###
    ### If we're running Polyworld, make sure a run of this ID doesn't already exist.
    ###
    if [ -e "$POLYWORLD_PWFARM_WORLDFILE" ]; then
	if good_run_exists "$RUNID"; then
	    err "A run of id $RUNID already exists!"
	fi
    fi

    ###
    ### Unpack the input zip
    ###
    if [ -e $PAYLOAD_DIR/input.zip ]; then
	export POLYWORLD_PWFARM_INPUT="$PAYLOAD_DIR/input"

	rm -rf $POLYWORLD_PWFARM_INPUT
	mkdir -p $POLYWORLD_PWFARM_INPUT || exit 1
	unzip $PAYLOAD_DIR/input.zip -d $POLYWORLD_PWFARM_INPUT || exit 1
    fi

    ###
    ### Execute the prerun script if it exists
    ###
    if [ -f $PAYLOAD_DIR/prerun.sh ]; then
	chmod +x $PAYLOAD_DIR/prerun.sh
	$PAYLOAD_DIR/prerun.sh || exit 1
    fi


    ###
    ### If no worldfile, then relocate requested run to current directory
    ###
    if [ ! -e $POLYWORLD_PWFARM_WORLDFILE ]; then
	unstore_run "$RUNID" ./run || exit 1
    fi

    ###
    ### Execute Polyworld if worldfile exists
    ###
    if [ -e $POLYWORLD_PWFARM_WORLDFILE ]; then
	###
	### Set the seed based on farm node
	###
	if [ "$( pwenv fieldnumber )" == "0" ]; then
	    seed=42
	else
	    seed="$( pwenv fieldnumber )"
	fi
	cp $POLYWORLD_PWFARM_WORLDFILE ./worldfile || exit 1
	./scripts/wfutil edit ./worldfile SimulationSeed=$seed || exit 1

	########################
	###                  ###
	### Run Polyworld!!! ###
	###                  ###
	########################
	./Polyworld --status ./worldfile
	exitval=$?

	if ! is_good_run --exit $exitval ./run; then
	    store_failed_run "$RUNID" ./run
	    exit 1
	fi

	cp $POLYWORLD_PWFARM_WORLDFILE run/farm.wf
    fi

    ###
    ### Init Run Zip Args
    ###
    PWFARM_RUNZIP=$( pwd )/zipargs
    export PWFARM_RUNZIP
    rm -f $PWFARM_RUNZIP

    echo 'stats/*' >> $PWFARM_RUNZIP
    echo '*.wf' >> $PWFARM_RUNZIP
    echo '*.wfs' >> $PWFARM_RUNZIP
    echo 'brain/Recent/*.plt' >> $PWFARM_RUNZIP

    ###
    ### Execute the postrun script
    ###
    postrun_failed=false

    if [ -f $PAYLOAD_DIR/postrun.sh ]; then
	chmod +x $PAYLOAD_DIR/postrun.sh

	if ! $PAYLOAD_DIR/postrun.sh; then
	    postrun_failed=true
	fi
    fi

    ###
    ### Store a code snapshot
    ###
    scripts/run_history.sh src run $postrun_failed $PAYLOAD_DIR/postrun.sh

    ###
    ### Create archive pulled to workstation
    ###
    cd run || err "postrun script moved ./run!!!"
    zip -qr $PWFARM_OUTPUT_FILE $( cat $PWFARM_RUNZIP )
    cd ..

    ###
    ### Store run
    ###
    store_good_run "$RUNID" "./run"

    ###
    ### Exit
    ###
    if $postrun_failed; then
	exit 1
    else
	exit 0
    fi
else
    ########################
    ###                  ###
    ### EXECUTE ON LOCAL ###
    ###                  ###
    ########################

    pwfarm_dir=`canondirname "$0"`
    poly_dir=`canonpath "$pwfarm_dir/../.."`

    WORLDFILE="nil"
    PRERUN="nil"
    POSTRUN="nil"
    INPUT_ZIP="nil"
    OWNER=$( pwenv pwuser )
    OWNER_OVERRIDE=false

    while getopts "w:c:a:i:o:" opt; do
	case $opt in
	    w)
		WORLDFILE="$OPTARG"
		;;
	    c)
		PRERUN="$OPTARG"
		;;
	    a)
		POSTRUN="$OPTARG"
		;;
	    i)
		INPUT_ZIP="$OPTARG"
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

    shift $(( $OPTIND - 1 ))
    if [ $# -lt 1 ]; then
	usage "Missing arguments"
    elif [ $# -gt 1 ]; then
	shift
	usage "Unexpected arguments: $*"
    fi

    RUNID=$( build_runid "$OWNER" "$1" )
    if $OWNER_OVERRIDE; then
	RUNID_LOCAL="$RUNID"
    else
	RUNID_LOCAL="$1"
    fi

    DSTDIR=$( pwenv runresults_dir )

    tmp_dir=`mktemp -d /tmp/poly_run.XXXXXXXX` || exit 1

    cp $0 $tmp_dir/pwfarm_run.sh || exit

    function cpopt()
    {
	if [ "$1" != "nil" ]; then
	    cp "$1" "$tmp_dir/$2" || exit 1
	fi
    }

    cpopt "$WORLDFILE" worldfile
    cpopt "$PRERUN" prerun.sh
    cpopt "$POSTRUN" postrun.sh
    cpopt "$INPUT_ZIP" input.zip

    echo "$RUNID" > $tmp_dir/runid

    pushd_quiet .
    cd $tmp_dir

    zip -qr payload.zip .

    popd_quiet

    $pwfarm_dir/__pwfarm_dispatcher.sh dispatch $tmp_dir/payload.zip './pwfarm_run.sh --field' run $DSTDIR/$RUNID_LOCAL

    rm -rf $tmp_dir
fi