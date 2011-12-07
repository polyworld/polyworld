#!/bin/bash

if [ -z "$PWFARM_SCRIPTS_DIR" ]; then
    source $( dirname $BASH_SOURCE )/__pwfarm_runutil.sh || exit 1
else
    source $PWFARM_SCRIPTS_DIR/__pwfarm_runutil.sh || exit 1
fi

ensure_farm_session

function usage()
{    
    cat <<EOF
usage: $( basename $0 ) [-o:] runid|/

    Checks if runid exists in either good or failed directories.

    Note: It isn't strictly necessary that you specify a valid run ID. It can be any
subpath below the runs directories. For example, if you wanted to check the size of
a movie file for run 'foo/x', you could invoke:

    $(basename $0) -u foo/x/movie.pmv

OPTIONS:

    -o    Specify run owner, which is prepended to run ID. "nil" for no owner.
        When used, orphan runs aren't listed.

    -u    Show disk usage. May take a while for directories.

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

owner=$( pwenv pwuser )
owner_override=false
diskusage=false

while getopts "o:u" opt; do
    case $opt in
	o)
	    owner="$OPTARG"
	    owner_override=true
	    ;;
	u)
	    diskusage=true
	    ;;
	*)
	    exit 1
	    ;;
    esac
done

args="$@"
shift $(( $OPTIND - 1 ))
if [ $# != 1 ]; then
    usage
fi

runid="$1"

tmpdir=$( mktemp -d /tmp/pwfarm_lsrun.XXXXXXXX ) || exit 1

if ! $field; then

    __pwfarm_script.sh --output result "$tmpdir" $0 --field $args || exit 1

    for num in $( pwenv fieldnumbers ); do
	echo ----- $( fieldhostname_from_num $num ) -----

	cat "$tmpdir/result_$num/out"
    done
else
    if ! $owner_override; then
	if [ -e "$POLYWORLD_PWFARM_APP_DIR/run" ] ; then
            echo WARNING! Contains orphan run! runid=$( cat "$POLYWORLD_PWFARM_APP_DIR/runid" ) >> $tmpdir/out 2>&1
        fi
    fi

    if [ "$runid" == "/" ]; then
	runid=""
    fi
    runid=$( build_runid "$owner" "$runid" )

    function dols()
    {
	local status="$1"
	local path=$( stored_run_path $status $runid )

	if [ -e "$path" ]; then
	    if is_empty_directory "$path"; then
		return 0
	    fi

	    echo "$( to_upper $status ):" >> $tmpdir/out
	    echo "  path=$path" >> $tmpdir/out  2>&1
	    if $diskusage; then
		echo "Computing disk usage -- might take a while..."
		echo "  disk usage=$( du -hs $path | cut -f 1 | trim - )" >> $tmpdir/out
	    fi
	    if [ -d "$path" ]; then
		echo "  details:" >> $tmpdir/out
		ls -ld "$path" | while read x; do echo "      $x"; done >> $tmpdir/out  2>&1
		echo "  content:" >> $tmpdir/out
		ls -F "$path" | while read x; do echo "      $x"; done >> $tmpdir/out  2>&1
	    else
		echo "  details:" >> $tmpdir/out
		ls -l "$path" | while read x; do echo "      $x"; done >> $tmpdir/out  2>&1
	    fi
	fi
    }

    dols "good"
    dols "failed"

    if [ ! -e $tmpdir/out ]; then
	echo "NO SUCH RUN" > $tmpdir/out
    fi

    cd $tmpdir
    zip -q $PWFARM_OUTPUT_FILE *
fi

rm -rf $tmpdir