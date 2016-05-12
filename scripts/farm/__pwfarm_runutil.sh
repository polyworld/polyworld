source $( dirname $BASH_SOURCE )/__lib.sh || exit 1

export POLYWORLD_PWFARM_HOME=$HOME/polyworld_pwfarm

export POLYWORLD_PWFARM_APP_DIR=$POLYWORLD_PWFARM_HOME/$( pwenv pwuser )/$( pwenv farmname )/app
export POLYWORLD_PWFARM_APP_MUTEX=$( dirname $POLYWORLD_PWFARM_APP_DIR )/mutex
export POLYWORLD_PWFARM_APP_PID=$( dirname $POLYWORLD_PWFARM_APP_DIR )/pid

export POLYWORLD_PWFARM_RUNS_DIR=$POLYWORLD_PWFARM_HOME/runs

export POLYWORLD_PWFARM_SCRIPTS_DIR=$POLYWORLD_PWFARM_APP_DIR/scripts

DEFAULT_FETCH_LIST=""
IMPLICIT_FETCH_LIST="stats *.wfs *.wf endStep.txt .pwfarm [*.wfo]"

function lock_app()
{
    while ! mutex_trylock $POLYWORLD_PWFARM_APP_MUTEX; do
	echo "Failed locking app mutex"
	if [ ! -e $POLYWORLD_PWFARM_APP_PID ]; then
	    echo "  No app mutex pid, assuming someone else is locking"
	    return 1
	else
	    if is_process_alive $POLYWORLD_PWFARM_APP_PID; then
		echo "  Mutex is currently owned."
		ps -p $(cat $POLYWORLD_PWFARM_APP_PID )
		return 1
	    else
		echo "  Mutex owner is dead."
		mutex_unlock $POLYWORLD_PWFARM_APP_MUTEX
	    fi
	fi
    done

    echo $$ > $POLYWORLD_PWFARM_APP_PID

    return 0
}

function unlock_app()
{
    mutex_unlock $POLYWORLD_PWFARM_APP_MUTEX
}

function validate_runid()
{
    if [ "$1" == "--ancestor" ]; then
	local ancestor=true
	shift
    else
	local ancestor=false
    fi
    if [ "$1" == "--wildcards" ]; then
	local wildcards=true
	shift
    else
	local wildcards=false
    fi
    local runid="$1"

    if echo "$runid" | grep [[:space:]] > /dev/null; then
	err "A Run ID cannot contain whitespace"
    fi

    if ! $wildcards && echo "$runid" | grep "[\*\?]" > /dev/null; then
	err "A Run ID cannot contain wildcards"
    fi

    if ! $ancestor && [ -z "$runid" ]; then
	err "A Run ID cannot be empty"
    fi

    while [ ! -z "$runid" ]; do
	local node=$(basename $runid)

	if echo $node | grep "^run_[0-9]*$" > /dev/null; then
	    err "A Run ID cannot contain 'run_[0-9]*'"
	elif [ "$node" == "results" ]; then
	    err "A Run ID cannot contain /results/"
	elif [ "$node" == ".." ]; then
	    err "A Run ID cannot contain '..'"
	fi

	prev=$runid
	runid=$(dirname $runid)

	if [ "$prev" == "$runid" ]; then
	    break;
	fi
    done
}

function normalize_runid()
{
    local runid="$1"

    echo "$runid" |
    sed  -e "s|/\{1,\}|/|g" -e "s|^/||" -e "s|/$||"
}

function is_runid_ancestor_match()
{
    local runid="/$1/"
    local ancestor="^/$2/"


    ancestor=$(
	echo "$ancestor" |
	sed -e "s|\.|\\\\.|g" -e "s|\*|.*|g" -e "s|//|/|g"
    )

    echo "$runid" | grep "$ancestor" > /dev/null
}

function stored_run_path_field()
{
    if [ "$1" == "--subpath" ]; then
	shift
	local status="$1"
	local owner="$2"
	local runid="$3"

	echo "$POLYWORLD_PWFARM_RUNS_DIR/$status/$owner/$runid"
    else
	local status="$1"
	local owner="$2"
	local runid="$3"
	local nid="$4"

	echo "$POLYWORLD_PWFARM_RUNS_DIR/$status/$owner/$runid/run_$nid"
    fi
}

function parse_stored_run_path_field()
{
    local mode="$1"
    local path="${2-stdin}"

    function __parse_path()
    {
	case $mode in
	    "--runid")
		echo $path |
		sed "s|^$POLYWORLD_PWFARM_RUNS_DIR/[^/]*/[^/]*/\(.*\)/run_[0-9]*$|\1|"
		;;
	    "--nid")
		echo $path |
		sed "s|^.*/run_\([0-9]*\)$|\1|"
		;;
	    *)
		err "Invalid parse_stored_run_path_field mode ($mode)"
		;;
	esac
    }

    if [ "$path" != "stdin" ]; then
	__parse_path
    else
	while read path; do
	    __parse_path
	done
    fi
}

function stored_run_path_local()
{
    if [ "$1" == "--subpath" ]; then
	shift
	local owner="$1"
	local runid="$2"

	echo "$(pwenv runresults_dir)/$owner/$runid"
    else
	local owner="$1"
	local runid="$2"
	local nid="$3"

	echo "$(pwenv runresults_dir)/$owner/$runid/run_$nid"
    fi
}

function parse_stored_run_path_local()
{
    local mode="$1"
    local path="${2-stdin}"

    function __parse_path()
    {
	case $mode in
	    "--runid")
		local resultsdir=$(pwenv runresults_dir)
		echo $path |
		sed "s|^$resultsdir/[^/]*/\(.*\)/run_[0-9]*$|\1|"
		;;
	    "--nid")
		echo $path |
		sed "s|^.*/run_\([0-9]*\)$|\1|"
		;;
	    *)
		err "Invalid parse_stored_run_path_field mode ($mode)"
		;;
	esac
    }

    if [ "$path" != "stdin" ]; then
	__parse_path
    else
	while read path; do
	    __parse_path
	done
    fi
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

    if ! is_run $run; then
	return 1
    fi

    if [ "$exitval" != "0" ]; then
	return 1
    fi

    if [ ! -e "$run/endStep.txt" ]; then
	return 1
    fi

    return 0
}

function is_run()
{
    dir="$1"

    [ -d $dir/stats ] || return 1
    [ -f $dir/normalized.wf ] || return 1
    [ -f $dir/original.wf ] || return 1

    return 0
}

function conflicting_run_exists()
{
    if [ "$1" == "--ignorebatch" ]; then
	local ignorebatch=true
	shift
    else
	local ignorebatch=false
    fi
    local owner="$1"
    local runid="$2"
    local nid="$3"
    local batchid="$4"

    local runs=$(ls_runs_field "good" $owner $runid "*")

    for run in $runs; do
	if ! $ignorebatch && [ "$(cat $run/.pwfarm/batchid)" != "$batchid" ]; then
	    echo "Found existing run with same Run ID, but different Batch ID at $run" >&2
	    return 0
	fi
	if [ "$(cat $run/.pwfarm/nid)" == "$nid" ]; then
	    echo "Conflicting nid at $run" >&2
	    return 0
	fi
    done

    return 1
}

function ls_runs_field()
{
    local status="$1"
    local owner="$2"
    local runid="$3"
    local nid="$4"

    local dir=$( stored_run_path_field "$status" "$owner" "$runid" "$nid" )

    ls -d $dir 2>/dev/null
}

function find_runs_field()
{
    local status=$1
    local owner=$2
    local runid=$3
    
    (
	export -f is_run

	for runfile in $(stored_run_path_field --subpath "$status" "$owner" "$runid"); do
	    if [ -e $runfile ]; then
		find $runfile -exec bash -c "is_run {}" \; -print -prune
	    fi
	done |
	canonpath
    )
}

function ls_runs_local()
{
    local owner="$1"
    local runid="$2"
    local nid="$3"

    local dir=$( stored_run_path_local "$owner" "$runid" "$nid" )

    ls -d $dir 2>/dev/null
}

function find_runs_local()
{
    local owner="$1"
    local runid="$2"

    (
	export -f is_run

	for runfile in $(stored_run_path_local --subpath "$owner" "$runid"); do
	    if [ -e $runfile ]; then
		find $runfile -exec bash -c "is_run {}" \; -print -prune
	    fi
	done |
	canonpath
    )
}

function find_results_local()
{
    local owner="$1"
    local runid="$2"

    for dir in $(stored_run_path_local --subpath "$owner" "$runid"); do
	if [ -e $dir ]; then
	    find $dir -type d -name "results" -prune
	fi
    done |
    canonpath
}

function store_orphan_run()
{
    local orphan="$1"

    if [ -e $orphan ]; then
	echo "Found orphan: $orphan"
	
	if [ -L $orphan ]; then
	    echo "Orphan is just a symlink. Removing link."
	    rm $orphan
	    return
	fi


	local orphan_dir=$( dirname $orphan )
	local owner=$( cat $orphan_dir/owner )
	local runid=$( cat $orphan_dir/runid )
	local nid=$( cat $orphan_dir/nid )

	[ ! -z "$owner" ] || err "Can't find orphan owner!"
	[ ! -z "$runid" ] || err "Can't find orphan runid!"
	[ ! -z "$nid" ] || err "Can't find orphan nid!"

	echo "  owner=$owner"
	echo "  runid=$runid"
	echo "  nid=$nid"

	store_run $owner $runid $nid $orphan
    fi
}

function store_run()
{
    local owner="$1"
    local runid="$2"
    local nid="$3"
    local run="$4"

    if is_good_run $run; then
	store_good_run $owner $runid $nid $run
    else
	store_failed_run $owner $runid $nid $run
    fi
}

function store_good_run()
{
    local owner="$1"
    local runid="$2"
    local nid="$3"
    local run="$4"

    local dst=$( stored_run_path_field "good" $owner $runid $nid )

    if [ -e "$dst" ]; then
	err "Attempting to store run of id $runid, but it already exists under good/!!!"
    fi

    echo "Storing good run at $dst" >&2
    mkdir -p $( dirname "$dst" ) || exit 1
    mv "$run" "$dst" || err "Failed storing good run $run to $dst!!!"
}

function store_failed_run()
{
    local owner="$1"
    local runid="$2"
    local nid="$3"
    local run="$4"

    if [ ! -e "$run" ]; then
	return
    fi

    local dst=$( stored_run_path_field "failed" $owner $runid $nid )

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

function link_run()
{
    local owner="$1"
    local runid="$2"
    local nid="$3"
    local run="$4"

    local src=$( stored_run_path_field "good" $owner $runid $nid )

    if [ ! -e $src ]; then
	err "Cannot locate existing run of id $runid"
    fi

    echo "Linking $run --> $src"

    ln -s "$src" "$run" || err "Failed linking run of id $runid!!!"
}

function unlink_run()
{
    local run="$1"

    if [ ! -L $run ]; then
	err "Not a symlink: $run"
    fi

    echo "Unlinking $run --> $(readlink $run)"

    rm $run
}