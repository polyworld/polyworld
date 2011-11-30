#!/bin/bash


function usage()
{
    echo "usage: $0 (\"nil\"|worldfile) run_id (\"nil\"|prerun_script) (\"nil\"|postrun_script) (\"nil\"|input_zip) dst_dir"
    echo
    echo "  worldfile: path of worldfile to be executed (\"nil\" if only doing postrun analysis)."
    echo
    echo "  run_id: unique id of run, which should use '/' for a hierarchy of classification (e.g. nutrients/barriers/give). This ultimately dictates where run is stored in file system."
    echo
    echo "  prerun_script: path of (optional) script to be executed prior to starting simulation."
    echo
    echo "  postrun_script: path of (optional) script to be executed after simulation completes."
    echo
    echo "  input_zip: path of (optional) input zip archive, to be unpacked at \$POLYWORLD_PWFARM_INPUT"
    echo
    echo "  dst_dir: destination for unpacking runs on local machine"
    echo

    if [ ! -z "$1" ]; then
	err $*
    fi

    exit 1
}

function err()
{
    echo "$*">&2
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

if [ -z "$1" ]; then
    usage
fi

if [ "$1" == "--field" ]; then
    ##############################
    ###                        ###
    ### EXECUTE ON REMOTE HOST ###
    ###                        ###
    ##############################

    payload_dir=`pwd`
    export POLYWORLD_PWFARM_WORLDFILE="$payload_dir/worldfile"
    RUNID=`cat $payload_dir/runid`

    cd ~/polyworld_pwfarm || exit

    if [ -e app/run ]; then
	mkdir -p runs/orphaned
	echo "found orphan run!"
	mv app/run runs/orphaned/`date | sed -e "s/ /_/g" -e "s/:/./g"`
    fi
    
    cd app

    failed=false

    export DISPLAY=:0.0 # for Linux -- allow graphics from ssh
    export PATH=`canonpath scripts`:$PATH
    ulimit -n 4096      # for Mac -- allow enough file descriptors

    ###
    ### Unpack the input zip
    ###
    if [ -e $payload_dir/input.zip ]; then
	export POLYWORLD_PWFARM_INPUT=~/polyworld_pwfarm/input

	if [ -e $POLYWORLD_PWFARM_INPUT ]; then
	    rm -rf $POLYWORLD_PWFARM_INPUT
	fi

	if ! mkdir -p $POLYWORLD_PWFARM_INPUT; then
	    failed=true
	else
	    if ! unzip $payload_dir/input.zip -d $POLYWORLD_PWFARM_INPUT; then
		failed=true
	    fi
	fi
    fi

    ###
    ### Execute the prerun script
    ###
    if ! $failed; then
	if [ -f $payload_dir/prerun.sh ]; then
	    chmod +x $payload_dir/prerun.sh

	    if ! $payload_dir/prerun.sh; then
		failed=true
	    fi
	fi
    fi

    if ! $failed; then
	if [ -e $POLYWORLD_PWFARM_WORLDFILE ]; then
	    ###
	    ### Set the seed based on farm node
	    ###
	    if [ "$PWFARM_ID" == "0" ]; then
		seed=42
	    else
		seed=$PWFARM_ID
	    fi
	    cp $POLYWORLD_PWFARM_WORLDFILE ./worldfile
	    ./scripts/wfutil edit ./worldfile SimulationSeed=$seed || exit 1

	    ###
	    ### Run Polyworld!!!
	    ###
	    if ! ./Polyworld ./worldfile; then
		failed=true
	    fi

	    cp $POLYWORLD_PWFARM_WORLDFILE run/farm.wf
	else
 	    ###
	    ### Move the old run from good/failed archive into working directory
	    ###
	    rundir=~/polyworld_pwfarm/runs/good/$RUNID
	    
	    if ! mv $rundir ./run ; then
		echo "Cannot locate good run... attempting failed run"
		rundir=~/polyworld_pwfarm/runs/failed/$RUNID

		if ! mv $rundir ./run ; then
		    failed=true
		fi
	    fi
	fi
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
    if ! $failed; then
	if [ -f $payload_dir/postrun.sh ]; then
	    chmod +x $payload_dir/postrun.sh

	    if ! $payload_dir/postrun.sh; then
		failed=true
	    fi
	fi
    fi

    if $failed; then
	touch run/FAILED
    else
	rm -f run/FAILED
    fi

    scripts/run_history.sh src run $failed $payload_dir/postrun.sh

    if [ -e run ]; then
	cd run
	zip -qr $PWFARM_OUTPUT_FILE $( cat $PWFARM_RUNZIP )
    fi

    cd ~/polyworld_pwfarm

    if [ -e app/run ]; then
	if $failed; then
	    dst=runs/failed
	else
	    dst=runs/good
	fi

	mkdir -p $dst
	dst=$dst/$RUNID
	if [ -e $dst ]; then
	    i=0
	    while [ -e ${dst}_$i ]; do
		i=$(($i + 1))
	    done

	    dst=${dst}_$i
	fi

	mkdir -p `dirname $dst`

	mv app/run $dst
    fi

    if $failed; then
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

    PWHOSTNUMBERS=~/polyworld_pwfarm/etc/pwhostnumbers
    USER=`cat ~/polyworld_pwfarm/etc/pwuser`

    if [ $# != 6 ]; then
	usage "Invalid number of arguments"
    fi

    WORLDFILE="$1"
    RUNID="$2"
    PRERUN="$3"
    POSTRUN="$4"
    INPUT_ZIP="$5"
    DSTDIR=`canonpath "$6"`

    if [ -z "$DSTDIR" ]; then
	err "No dstdir!"
    fi

    tmp_dir=`mktemp -d /tmp/poly_run.XXXXXXXX` || exit 1

    cp $0 $tmp_dir/poly_run.sh || exit

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

    pushd .
    cd $tmp_dir

    zip -r payload.zip .

    popd

    $pwfarm_dir/pwfarm_dispatcher.sh dispatch $PWHOSTNUMBERS $USER $tmp_dir/payload.zip './poly_run.sh --field' run $DSTDIR/$RUNID

    rm -rf $tmp_dir
fi