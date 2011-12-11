#!/bin/bash

if [ -z "$PWFARM_SCRIPTS_DIR" ]; then
    source $( dirname $BASH_SOURCE )/__pwfarm_runutil.sh || exit 1
else
    source $PWFARM_SCRIPTS_DIR/__pwfarm_runutil.sh || exit 1
fi

function usage()
{    
    cat <<EOF
usage: $( basename $0 ) mode

MODES:

    sessions 
             List active sessions on farm.

    top      Execute top util. (Interactive)

    df       Show output from df util -- disk free.

    screen   Show output from screen -ls

OPTIONS:

   -f fields
             Specify fields on which this should run. Must be a single argument,
          so use quotes. e.g. -f "0 1" or -f "{0..3}"
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

while getopts "f:" opt; do
    case $opt in
	f)
	    __pwfarm_config env set fieldnumbers "$OPTARG"
	    validate_farm_env
	    ;;
	*)
	    exit 1
	    ;;
    esac
done
shift $(( $OPTIND - 1 ))

if [ $# != 1 ]; then
    usage
fi

args="$*"
mode="$1"

case "$mode" in
    "sessions")
	;;
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
    validate_farm_env

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
	"sessions")
	    screen -ls | grep pwfarm_field | 
	    while read line; do
		user=$( echo $line | sed 's/\(.*pwfarm_field__user_\)\(.*\)\(__farm_.*\)/\2/g' )
		session=$( echo $line | sed 's/\(.*pwfarm_field_.*__session_\)\(.*\)\(____.*\)/\2/g' )

		if [ "$session" != $( pwenv sessionname ) ]; then
		    echo "$session (user=$user)" >> $tmpdir/out 2>&1
		fi
	    done

	    if [ ! -e $tmpdir/out ]; then
		echo "NO SESSIONS" >> $tmpdir/out 2>&1
	    fi
	    ;;
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