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
    FIELD=true
    shift
else
    FIELD=false
fi

while getopts "f:" opt; do
    case $opt in
	f)
	    if ! $FIELD; then
		__pwfarm_config env set fieldnumbers "$OPTARG"
		validate_farm_env
	    fi
	    ;;
	*)
	    exit 1
	    ;;
    esac
done

ARGS=$( encode_args "$@" )
shift $(( $OPTIND - 1 ))

if [ $# != 1 ]; then
    usage
fi

MODE="$1"

case "$MODE" in
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

TMP_DIR=$( create_tmpdir ) || exit 1

if ! $FIELD; then
    validate_farm_env

    RESULTS_DIR=$TMP_DIR/result

    __pwfarm_script.sh --output $RESULTS_DIR $0 --field $ARGS || exit 1

    for num in $( pwenv fieldnumbers ); do
	if [ -e "${RESULTS_DIR}_$num/out" ]; then
	    echo -----
	    echo ----- $( fieldhostname_from_num $num )
	    echo -----
	    cat "${RESULTS_DIR}_$num/out"
	    echo
	fi
    done
else
    case "$MODE" in
	"sessions")
	    screen -ls |
	    grep pwfarm_field | 
	    while read line; do
		user=$( echo $line | sed 's/\(.*pwfarm_field__user_\)\(.*\)\(__farm_.*\)/\2/g' )
		session=$( echo $line | sed 's/\(.*pwfarm_field_.*__session_\)\(.*\)\(____.*\)/\2/g' )

		if [ "$user" != "$( pwenv pwuser )" ] || [ "$session" != "$( pwenv sessionname )" ]; then
		    echo "$session (user=$user)" >> $TMP_DIR/out 2>&1
		fi
	    done

	    if [ ! -e $TMP_DIR/out ]; then
		echo "NO SESSIONS" >> $TMP_DIR/out 2>&1
	    fi
	    ;;
	"top")
	    top
	    ;;
	"df")
	    df -h > $TMP_DIR/out 2>&1
	    ;;
	"screen")
	    screen -ls > $TMP_DIR/out 2>&1
	    ;;
	*)
	    err "Invalid mode ($mode)"
	    ;;
    esac

    if [ -e $TMP_DIR/out ]; then
	cd $TMP_DIR
	archive pack $PWFARM_OUTPUT_ARCHIVE out
    fi
fi

rm -rf $TMP_DIR