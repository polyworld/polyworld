#!/bin/bash

source $( dirname $BASH_SOURCE )/__lib.sh || exit 1

if [ "$1" == "--noprompterr" ]; then
    shift
    erropt="--noprompterr"
else
    erropt=""
fi

if [ "$1" == "--sudo" ]; then
    shift
    sudoopt="--sudo"
else
    sudoopt=""
fi

if [ "$1" == "--input" ]; then
    shift
    input="$1"
    shift
fi

if [ "$1" == "--output" ]; then
    outputdir="$2"
    shift 2
else
    outputdir="nil"
fi

script="$1"
shift
args="$*"
cmd="./$( basename $script ) $args"

if [ ! -e "$script" ]; then
    err "Cannot locate $script"
fi

tmpdir=$( create_tmpdir ) || exit 1
cp $script $tmpdir || exit
if [ ! -z "$input" ]; then
    cp $input $tmpdir || exit
fi

pushd_quiet .
cd $tmpdir
zip -qr payload.zip .
popd_quiet

pwfarm_dir=`canondirname "$0"`

tasks=$( taskmeta create_field_tasks $tmpdir $erropt $sudoopt "$cmd" "$outputdir") || exit 1

$pwfarm_dir/__pwfarm_dispatcher.sh dispatch $tmpdir/payload.zip $tasks
exitval=$?

rm -rf $tmp_dir

exit $exitval