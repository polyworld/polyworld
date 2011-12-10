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
    field=true
    shift
else
    field=false
fi


if ! $field; then
    validate_farm_env

    interactive=false
    password=false
    testing=false

    while getopts "isf:" opt; do
	case $opt in
	    i)
		interactive=true
		;;
	    s)
		password=true
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

    payloaddir=$( mktemp -d /tmp/pwfarm_lsrun.XXXXXXXX ) || exit 1

    cp $0 $payloaddir
    echo $interactive > $payloaddir/interactive
    echo "$@" > $payloaddir/command
    echo $password > $payloaddir/password

    pushd_quiet .
    cd $payloaddir
    zip -rq payload.zip .
    popd_quiet

    farmcmd="./$( basename $0 ) --field"

    opts=""
    if $password; then
	opts="--password $opts"
    fi

    if ! $interactive; then
	opts="$opts --noprompterr"
	resultdir=$( mktemp -d /tmp/pwfarm_lsrun.XXXXXXXX ) || exit 1
	output_basename="result"
	output_dir="$resultdir"
    fi

    $PWFARM_SCRIPTS_DIR/__pwfarm_dispatcher.sh $opts dispatch $payloaddir/payload.zip "$farmcmd" "$output_basename" "$output_dir"
    
    if ! $interactive; then
	for num in $( pwenv fieldnumbers ); do
	    if [ -e "$resultdir/result_$num/out" ]; then
		echo ----- $( fieldhostname_from_num $num ) -----

		cat "$resultdir/result_$num/out"
	    fi
	done

	rm -rf $resultdir
    fi

    rm -rf $payloaddir
else
    if [ -e interactive ] && ! $( cat interactive ); then
	# Call this script again, but redirected.
	rm ./interactive

	tmpdir=$( mktemp -d /tmp/pwfarm_lsrun.XXXXXXXX ) || exit 1

	$0 --field > $tmpdir/out  2>&1
	exitval=$?

	cd $tmpdir
	zip -q $PWFARM_OUTPUT_FILE *
	rm -rf $tmpdir
    elif [ -e password ] && $( cat password ); then
	# Call this script again, but with root access.
	rm password

	PWFARM_SUDO $0 --field
	exitval=$?
    else
	cmd="$( cat command )"
	eval "$cmd"
	exitval=$?
    fi

    exit $exitval
fi

