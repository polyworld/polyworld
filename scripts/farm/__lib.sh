if [ -z "$BASH_SOURCE" ]; then
    echo "polyworld/scripts/farm/__lib.sh: Require \$BASH_SOURCE." >&2
    echo "Please use a version of bash >= 3" >&2
    exit 1
fi

function canonpath()
{
    local path="${1-stdin}"

    if [ "$path" == "stdin" ]; then
	while read path; do
	    canonpath "$path"
	done
    else
	python -c "import os.path; print os.path.realpath('$path')"
    fi
}

function canondirname()
{
    dirname `canonpath "$1"`
}

__LIB_DIR=$( canondirname $BASH_SOURCE )

source $( dirname $BASH_SOURCE )/__pwfarm_config.sh
source $( dirname $BASH_SOURCE )/__pwfarm_taskmeta.sh

function create_tmpdir()
{
    mkdir -p /tmp/pwfarm
    mktemp -d /tmp/pwfarm/$(basename $0).XXXXXXXX
}

function relpath()
{
    python -c "import os.path; print os.path.relpath('$1', '$2')"
}

export PWFARM_SCRIPTS_DIR=$( canondirname $BASH_SOURCE )
export POLYWORLD_DIR=$( canonpath $PWFARM_SCRIPTS_DIR/../.. )
export POLYWORLD_SCRIPTS_DIR=$POLYWORLD_DIR/scripts

function pushd_quiet()
{
    pushd $* > /dev/null
}

function popd_quiet()
{
    popd $* > /dev/null
}

function err()
{
    echo "$( basename $0 ):" "$*">&2
    exit 1
}

function warn()
{
    echo "$( basename $0 ): WARNING!" "$*">&2
}

function require()
{
    if [ -z "$1" ]; then
	shift
	err "Missing required parameter: $*"
    fi
}

function assert()
{
    if ! $@; then
	err "assertion failed: $@"
    fi
}

function is_integer()
{
    [ ! -z "$1" ] || return 1
    printf "%d" "$1" > /dev/null 2>&1
}

function is_integer_list()
{
    [ ! -z "$1" ] || return 1

    for x in $*; do
	if ! is_integer $x; then
	    return 1
	fi
    done

    return 0
}

function archive()
{
    local srcdir=$__LIB_DIR

    if [ -e $srcdir/archive.sh ]; then
	$srcdir/archive.sh "$@"
    elif [ -e $srcdir/../archive.sh ]; then
	$srcdir/../archive.sh "$@"
    else
	err "Cannot locate archive.sh! srcdir=$srcdir"
    fi
}

function archive_delta()
{
    local srcdir=$__LIB_DIR

    if [ -e $srcdir/archive_delta.sh ]; then
	$srcdir/archive_delta.sh "$@"
    elif [ -e $srcdir/../archive_delta.sh ]; then
	$srcdir/../archive_delta.sh "$@"
    else
	err "Cannot locate archive_delta.sh! srcdir=$srcdir"
    fi
}

function __kill_jobs()
{
    local pids=$( jobs -p )
    if [ ! -z "$pids" ]; then
	kill $pids 2>/dev/null
    fi
    exit 1
}
function kill_jobs_on_termination()
{
    trap '__kill_jobs' SIGINT SIGTERM
}

function repeat_til_success
{
    if [ "$1" == "--display" ]; then
	local display="$2"
	shift 2
    else
	local display="$*"
    fi
    local errtime="0"
    local errmsg="/tmp/pwfarm/repeat.err.txt.$$"

    mkdir -p /tmp/pwfarm

    while ! $* 2>$errmsg; do
	local now=$( date '+%s' )

	# Show error message if more than 20 seconds has elapsed since last error
	if [ $(( $now - $errtime )) -gt 20 ]; then
	    echo >&2
	    echo "FAILED: $display" >&2
	    echo "ERR: $( cat $errmsg )" >&2
	    echo "  Repeating until success..." >&2
	fi

	errtime=$now
	# give user chance to ctrl-c
	sleep 5
    done

    rm -f $errmsg

    if [ "$errtime" != "0" ]; then
	echo "RECOVERED FROM ERROR: $display"
    fi
}


function is_process_alive()
{
    local pid_file="$1"
    require "$pid_file" "is_process_alive pid_file arg"

    if [ ! -e "$pid_file" ]; then
	return 1
    fi

    local pid=$( cat $pid_file )
    if [ -z $pid ]; then
	return 1
    fi

    ps -p $pid > /dev/null
}

function mutex_trylock()
{
    local timeout=1
    if [ "$1" == "--timeout" ]; then
	require "$2" "mutex_trylock --timeout arg"
	shift
	timeout="$1"
	shift
    fi

    local mutex_name=$1
    require "$mutex_name" "mutex_lock( mutex_name )"

    mkdir -p $( dirname $mutex_name )

    while ! mkdir $mutex_name 2>/dev/null; do
	timeout=$(( timeout - 1 ))
	if [ $timeout == 0 ]; then
	    return 1
	fi
	sleep 1
    done

    return 0
}

function mutex_lock()
{
    local mutex_name=$1
    require "$mutex_name" "mutex_lock( mutex_name )"

    mkdir -p $( dirname $mutex_name )

    local blocked="false"
    while ! mkdir $mutex_name 2>/dev/null; do
	if ! $blocked; then
	    echo "blocking on mutex $mutex_name..."
	    blocked="true"
	fi
	sleep 1
    done

    if $blocked; then
	echo "successfully locked mutex $mutex_name"
    fi
}

function mutex_unlock()
{
    local mutex_name=$1
    require "$mutex_name" "mutex_unlock( mutex_name )"

    if [ ! -e "$mutex_name" ]; then
	err "mutex_unlock invoked on non-locked mutex $mutex_name"
    fi

    rmdir "$mutex_name"
}

function pwquery()
{
    __pwfarm_config query $*
}

function pwenv()
{
    __pwfarm_config env get $*
}

function taskmeta()
{
    __pwfarm_taskmeta "$@"
}

function dispatcher()
{
    $PWFARM_SCRIPTS_DIR/__pwfarm_dispatcher.sh "$@"
}

function fieldhostname_from_num()
{
    local id=$( printf "%02d" $1 2>/dev/null ) || err "Invalid field number: $1"
    echo "pw$id"
}

function fieldhostnames_from_nums()
{
    local x
    for x in $*; do 
	fieldhostname_from_num $x
    done
}

function fieldnum_from_hostname()
{
    echo $1 | sed -e s/pw0//g -e s/pw//g
}

function fieldnums_from_hostnames()
{
    for x in $*; do
	fieldnum_from_hostname $x
    done
}

function fieldhost_from_name()
{
    eval pwhost=\$$1
    echo $pwhost
}

function validate_farm_env()
{
    local farmname=$( pwenv farmname )
    if [ -z "$farmname" ]; then
	err "You haven't set your farm context."
    fi

    local sessionname=$( pwenv sessionname )
    if [ -z  "$sessionname" ]; then
	err "You haven't set your session context."
    fi

    local sessionfieldnumbers=$(pwquery sessionfieldnumbers $farmname $sessionname)
    local farmfieldnumbers=$(pwquery fieldnumbers $farmname)
    if ! is_subset_of "$sessionfieldnumbers" "$farmfieldnumbers"; then
	err "\
Session field numbers not a subset of farm field numbers!
  session: $sessionfieldnumbers
  farm:    $farmfieldnumbers"
    fi

    local envfieldnumbers=$(pwenv fieldnumbers)
    if ! is_subset_of "$envfieldnumbers" "$sessionfieldnumbers"; then
	err "\
Environment field numbers not a subset of session field numbers!
  env:     $envfieldnumbers
  session: $sessionfieldnumbers"
    fi
}

function ls_color_opt()
{
    case $( uname ) in
	Darwin)
	    echo -G
	    ;;
	Linux)
	    echo --color
	    ;;
    esac
}

function to_lower() 
{
    echo $1 | tr "[:upper:]" "[:lower:]"
}

function to_upper()
{
    echo $1 | tr  "[:lower:]" "[:upper:]"
}

function is_empty_directory()
{
    local path="$1"

    [ -d "$path" ] && [ -z "$( ls -A "$path" )" ];
}

function prune_empty_directories()
{
    local dir=$(canonpath $1)

    while is_empty_directory $dir; do
	rmdir $dir || return 1
	dir=$( dirname $dir )
    done
}

function any_files_exist()
{
    local files="$@"

    ls -d $files >/dev/null 2>&1
}

function trim()
{
    local val="$1"
    if [ "$val" == "-" ]; then
	read  -rd '' val
    else
	read  -rd '' val <<< "$val"
    fi
    echo "$val"
}

function show_file_gui()
{
    case `uname` in
	Linux)
	    gnome-open $1 || kde-open $1
	    ;;
	Darwin)
	    open $1
	    ;;
	*)
	    err "Unknown Operating System (`uname`)"
	    ;;
    esac
}

function encode_args()
{
    while [ $# != 0 ]; do
	echo -n \'$1\'" "
	shift
    done
}

function decode_args()
{
    local result=""

    while [ $# != 0 ]; do
	local decoded=$(echo -n $1 | sed -e "s/^'//" -e "s/'$//")
	result="$result $decoded"
	shift
    done

    trim "$result"
}

# expand things like '{0..2}' to '0 1 2'
function expand_int_list()
{
    eval echo $*
}

function is_subset_of()
{
    local some=( $1 )
    local all=( $2 )

    for (( i=0; i<${#some[@]}; i++ )); do
	local found=false
	for (( j = 0 ; j < ${#all[@]}; j++ )); do
	    if [ "${some[$i]}" == "${all[$j]}" ]; then
		found=true
		break;
	    fi
	done

	if ! $found; then
	    return 1
	fi

    done

    return 0
}

function contains()
{
    local args=( "$@" )
    local search_value=${args[$(( $# - 1 ))]}

    for (( i=0; i < $(( $# - 1 )); i++ )); do
	if [ "${args[$i]}" == "$search_value" ]; then
	    return 0
	fi
    done

    return 1
}

function len()
{
    echo $#
}

function join()
{
    python - "$@" <<EOF
import sys
print sys.argv[1].join(sys.argv[2:])
EOF
}

function indent()
{
    while read x; do printf "   $x\n"; done
}
