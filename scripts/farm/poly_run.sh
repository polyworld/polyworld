#!/bin/bash -x


if [ -z "$1" ]; then
    echo "usage: $0 worldfile run_id postrun_script dst_dir"
    echo
    echo "  worldfile: path of worldfile to be executed."
    echo
    echo "  run_id: unique id of run, which should use '/' for a hierarchy of classification."
    echo
    echo "  postrun_script: path of script to be executed after simulation completes."
    echo
    echo "  dst_dir: destination for unpacking runs on local machine"
    echo

    exit 1
fi

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

if [ "$1" == "--field" ]; then
    payload_dir=`pwd`

    cd ~/polyworld_pwfarm || exit

    if [ -e app/run ]; then
	mkdir -p runs/orphaned
	echo "found orphan run!"
	mv app/run runs/orphaned/`date | sed -e "s/ /_/g" -e "s/:/./g"`
    fi
    
    cd app

    if [ "$PWFARM_ID" == "0" ]; then
	seed=42
    else
	seed=$PWFARM_ID
    fi

    ./scripts/edit_worldfile $payload_dir/worldfile ./worldfile genomeseed_used_for_all=$seed

    export DISPLAY=:0.0

    failed=true
    if ./Polyworld; then
	if $payload_dir/postrun.sh; then
	    failed=false
	fi
    fi

    if ! $failed; then
	rundir=run_${PWFARM_ID}
	archive=${rundir}.zip
	if [ -e $archive ]; then
	    rm $archive
	fi
	mv run $rundir
	zip -r $archive $rundir/brain/Recent/*.plt $rundir/events/*.txt $rundir/stats/* $rundir/worldfile
	mv $rundir run
    fi

    cd ~/polyworld_pwfarm

    if [ -e app/run ]; then
	RUNID=`cat $payload_dir/runid`

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
    WORLDFILE=$1
    RUNID=$2
    POSTRUN=$3
    DSTDIR=$4

    if [ -z $DSTDIR ]; then
	err "No dstdir!"
    fi

    tmp_dir=`mktemp -d /tmp/poly_run.XXXXXXXX` || exit 1
    echo $tmp_dir

    cp $0 $tmp_dir/poly_run.sh || exit
    cp $WORLDFILE $tmp_dir/worldfile || exit
    cp $POSTRUN $tmp_dir/postrun.sh || exit
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
	    unzip $x
	done

	rm *.zip
    fi

    rm -rf $tmp_dir
fi