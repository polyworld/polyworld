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
payload_dir=$tmpdir/payload
mkdir -p $payload_dir
payload=$tmpdir/payload.tbz
cp $script $payload_dir || exit
if [ ! -z "$input" ]; then
    archive unpack -d $payload_dir $input
fi

pushd_quiet .
cd $payload_dir
archive pack $payload .
popd_quiet

pwfarm_dir=`canondirname "$0"`

tasks=$( taskmeta create_field_tasks $tmpdir $erropt $sudoopt "$cmd" "$outputdir") || exit 1

$pwfarm_dir/__pwfarm_dispatcher.sh dispatch $payload $tasks
exitval=$?

rm -rf $tmp_dir

exit $exitval