#!/bin/bash

if [ -z "$PWFARM_SCRIPTS_DIR" ]; then
    source $( dirname $BASH_SOURCE )/__lib.sh || exit 1
else
    source $PWFARM_SCRIPTS_DIR/__lib.sh || exit 1
fi

function usage()
{    
    cat <<EOF
usage: $( basename $0 ) [-isf:] command [arg]...

    Execute a shell command on the farm.

OPTIONS:

   -i             Interactive. Don't redirect stdout/stderr to file. Prompt user on err.

   -s             Sudo. Prompt for admin password before running tasks on farm.

   -f fields
                  Specify fields on which this should run. Must be a single argument,
                so use quotes. e.g. -f "0 1" or -f "{0..3}"
EOF
    exit 1
}

if [ "$1" == "--field" ]; then
    FIELD=true
    shift
else
    FIELD=false
fi


if ! $FIELD; then
    validate_farm_env

    INTERACTIVE=false
    SUDO=false

    while getopts "isf:" opt; do
	case $opt in
	    i)
		INTERACTIVE=true
		;;
	    s)
		SUDO=true
		;;
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
    if [ $# == 0 ]; then
	usage
    fi

    TMP_DIR=$( create_tmpdir ) || exit 1
    PAYLOAD_DIR=$TMP_DIR/payload
    RESULTS_DIR=$TMP_DIR
    
    mkdir -p $PAYLOAD_DIR

    cp $0 $PAYLOAD_DIR
    echo $INTERACTIVE > $PAYLOAD_DIR/interactive
    echo "$@" > $PAYLOAD_DIR/command
    echo $SUDO > $PAYLOAD_DIR/sudo

    PAYLOAD=$TMP_DIR/payload.tbz

    pushd_quiet .
    cd $PAYLOAD_DIR
    archive pack $PAYLOAD .
    popd_quiet

    if $INTERACTIVE; then
	opt_err=""
	opt_out=""
    else
	opt_err="--noprompterr"
	opt_out="--output $RESULTS_DIR"
    fi
    if $SUDO; then
	opt_sudo="--sudo"
    else
	opt_sudo=""
    fi
    opt_in="--input $PAYLOAD"
    opts="$opt_err $opt_sudo $opt_in $opt_out"

    __pwfarm_script.sh $opts $0 --field

    if ! $INTERACTIVE; then
	for num in $( pwenv fieldnumbers ); do
	    if [ -e "${RESULTS_DIR}_$num/out" ]; then
		echo ----- $( fieldhostname_from_num $num ) -----

		cat "${RESULTS_DIR}_$num/out"
	    fi
	done
    fi

    rm -rf $TMP_DIR
else
    if [ -e interactive ] && ! $( cat interactive ); then
	# Call this script again, but redirected.
	rm ./interactive

	tmpdir=$( create_tmpdir ) || exit 1

	$0 --field > $tmpdir/out  2>&1
	exitval=$?

	cd $tmpdir
	archive pack $PWFARM_OUTPUT_ARCHIVE *
	rm -rf $tmpdir
    elif [ -e ./sudo ] && $( cat ./sudo ); then
	# Call this script again, but with root access.
	rm ./sudo

	PWFARM_SUDO $0 --field
	exitval=$?
    else
	cmd="$( cat command )"
	eval "$cmd"
	exitval=$?
    fi

    exit $exitval
fi

