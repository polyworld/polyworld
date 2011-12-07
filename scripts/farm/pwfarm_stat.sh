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
usage: $( basename $0 ) mode

MODES:

    top      Execute top util. (Interactive)

    df       Show output from df util -- disk free.

    screen   Show output from screen -ls
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

if [ $# != 1 ]; then
    usage
fi

args="$*"
mode="$1"

case "$mode" in
    "top")
	;;
    "df")
	;;
    "screen")
	;;
    *)
	err "Invalid mode ($mode)"
	;;
esac

tmpdir=$( mktemp -d /tmp/pwfarm_stat.XXXXXXXX ) || exit 1

if ! $field; then

    __pwfarm_script.sh --output result "$tmpdir" $0 --field $args || exit 1

    for num in $( pwenv fieldnumbers ); do
	if [ -e "$tmpdir/result_$num/out" ]; then
	    echo -----
	    echo ----- $( fieldhostname_from_num $num )
	    echo -----
	    cat "$tmpdir/result_$num/out"
	    echo
	fi
    done
else
    case "$mode" in
	"top")
	    top
	    ;;
	"df")
	    df -h > $tmpdir/out 2>&1
	    ;;
	"screen")
	    screen -ls > $tmpdir/out 2>&1
	    ;;
	*)
	    err "Invalid mode ($mode)"
	    ;;
    esac

    if [ -e $tmpdir/out ]; then
	cd $tmpdir
	zip -q $PWFARM_OUTPUT_FILE *
    fi
fi

rm -rf $tmpdir