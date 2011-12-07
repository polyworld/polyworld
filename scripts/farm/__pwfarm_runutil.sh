source $( dirname $BASH_SOURCE )/__lib.sh || exit 1

export POLYWORLD_PWFARM_HOME=$HOME/polyworld_pwfarm
export POLYWORLD_PWFARM_APP_DIR=$POLYWORLD_PWFARM_HOME/$( pwenv pwuser )/$( pwenv farmname )/$( pwenv sessionname )/app
export POLYWORLD_PWFARM_RUNS_DIR=$POLYWORLD_PWFARM_HOME/runs

function build_runid()
{
    local owner="$1"
    local runid="$2"

    if [ "$owner" == "nil" ]; then
	echo $runid
    else
	echo $owner/$runid
    fi
}

function stored_run_path()
{
    local status="$1"
    local runid="$2"

    echo "$POLYWORLD_PWFARM_RUNS_DIR/$status/$runid"
}

function is_good_run()
{
    if [ "$1" == "--exit" ]; then
	local exitval="$2"
	shift 2
    else
	local exitval="0"
    fi
    local run="$1"

    if [ "$exitval" != "0" ]; then
	return 1
    fi

    if [ ! -e "$run/endStep.txt" ]; then
	return 1
    fi

    return 0
}

function good_run_exists()
{
    local runid="$1"

    if [ -e $( stored_run_path "good" "$runid" ) ]; then
	return 0
    else
	return 1
    fi
}

function store_orphan_run()
{
    local orphan="$1"

    if [ -e $orphan ]; then
	echo "Found orphan: $orphan"

	if [ -e $( dirname $orphan)/runid ]; then
	    local runid=$( cat $( dirname $orphan )/runid )
	else
	    echo "No associated run id!"
	    local runid=$( date | sed -e "s/ /_/g" -e "s/:/./g" )
	fi
	echo "  run id=$runid"

	store_run "$runid" "$orphan"
    fi
}

function store_run()
{
    local runid="$1"
    local run="$2"

    if is_good_run "$run"; then
	store_good_run "$runid" "$run"
    else
	store_failed_run "$runid" "$run"
    fi
}

function store_good_run()
{
    local runid="$1"
    local run="$2"

    local dst=$( stored_run_path "good" "$runid" )

    if [ -e "$dst" ]; then
	err "Attempting to store run of id $runid, but it already exists under good/!!!"
    fi

    echo "Storing good run at $dst" >&2
    mkdir -p $( dirname "$dst" ) || exit 1
    mv "$run" "$dst" || err "Failed storing good run $run to $dst!!!"
}

function store_failed_run()
{
    local runid="$1"
    local run="$2"

    if [ ! -e "$run" ]; then
	return
    fi

    local dst=$( stored_run_path "failed" "$runid" )

    if [ -e "$dst" ]; then
	local i=0
	while [ -e ${dst}_$i ]; do
	    i=$(($i + 1))
	done
	dst=${dst}_$i
    fi

    echo "Storing failed run at $dst" >&2
    mkdir -p $( dirname "$dst" ) || exit 1
    mv "$run" "$dst" || err "Failed storing failed run!!!"
}

function unstore_run()
{
    local runid="$1"
    local run="$2"

    if [ -e $( stored_run_path "good" "$runid" ) ]; then
	local loc=$( stored_run_path "good" "$runid" )
    else
	err "Cannot locate existing run of id $runid"
    fi

    mv "$loc" "$run" || err "Failed unstoring run of id $runid!!!"
}