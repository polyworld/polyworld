#!/bin/bash


function usage()
{
    echo "usage: $0 (\"nil\"|worldfile) run_id (\"nil\"|prerun_script) (\"nil\"|postrun_script) (\"nil\"|input_zip) dst_dir"
    echo
    echo "  worldfile: path of worldfile to be executed (\"nil\" if only doing postrun analysis)."
    echo
    echo "  run_id: unique id of run, which should use '/' for a hierarchy of classification."
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
    ulimit -n 1024      # for Mac -- allow enough file descriptors

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
	    ./scripts/edit_worldfile.py $POLYWORLD_PWFARM_WORLDFILE ./worldfile genomeSeed_also_used_for_position=$seed || exit 1

	    ###
	    ### Run Polyworld!!!
	    ###
	    if ! ./Polyworld; then
		failed=true
	    fi
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

    rundir=run_${PWFARM_ID}
    archive=${rundir}.zip
    if [ -e $archive ]; then
	rm $archive
    fi
    mv run $rundir
    zip -r $archive $rundir/brain/Recent/*.plt $rundir/events/*.txt $rundir/stats/* $rundir/worldfile $rundir/*.eps
    mv $rundir run

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

    cd $tmp_dir

    zip -r payload.zip .

    $pwfarm_dir/pwfarm_dispatcher.sh clear $PWHOSTNUMBERS
    if $pwfarm_dir/pwfarm_dispatcher.sh dispatch $PWHOSTNUMBERS $USER payload.zip './poly_run.sh --field'
    then
	mkdir -p $DSTDIR/$RUNID

	pwhostnumbers=`cat $PWHOSTNUMBERS`

	for x in $pwhostnumbers; do
	    number=$x
	    id=`printf "%02d" $number`
	    pwhostname=pw$id
	    eval pwhost=\$$pwhostname

	    while ! scp $USER@$pwhost:~/polyworld_pwfarm/app/run_$number.zip $DSTDIR/$RUNID
	    do
		echo "Failed copying run.zip from $pwhostname"
		sleep 2
	    done
	done

	cd $DSTDIR/$RUNID
	for x in *.zip; do
	    unzip -o $x
	done

	rm *.zip
    fi

    rm -rf $tmp_dir
fi