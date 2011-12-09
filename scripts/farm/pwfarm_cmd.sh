#!/bin/bash

if [ -z "$PWFARM_SCRIPTS_DIR" ]; then
    source $( dirname $BASH_SOURCE )/__lib.sh || exit 1
else
    source $PWFARM_SCRIPTS_DIR/__lib.sh || exit 1
fi

ensure_farm_session

function usage()
{    
    cat <<EOF
usage: $( basename $0 ) command [arg]...

    Execute a shell command on the farm.
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

args=$( encode_args "$@" )
if [ $# == 0 ]; then
    usage
fi

tmpdir=$( mktemp -d /tmp/pwfarm_lsrun.XXXXXXXX ) || exit 1

if ! $field; then

    __pwfarm_script.sh --output result "$tmpdir" $0 --field "$args" || exit 1

    for num in $( pwenv fieldnumbers ); do
	echo ----- $( fieldhostname_from_num $num ) -----

	cat "$tmpdir/result_$num/out"
    done
else
    touch $tmpdir/out

    echo $args

    cd ~/
    cmd="$( decode_args $args )"
    #echo "Executing '$cmd'..."
    #echo "Executing '$cmd'..." >> $tmpdir/out 2>&1
    eval "$cmd" >> $tmpdir/out  2>&1

    if [ ! -e $tmpdir/out ]; then
	echo "NO SUCH RUN" > $tmpdir/out
    fi

    cd $tmpdir
    zip -q $PWFARM_OUTPUT_FILE *
fi

rm -rf $tmpdir