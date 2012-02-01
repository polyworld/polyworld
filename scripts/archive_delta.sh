#!/bin/bash

function usage()
{
################################################################################
    cat >&2 <<EOF
usage: 

$( basename $0 ) "checksums" output_checksums dir input_spec...

      Creates a checksums file.

ARGS:

   output_checksums
                 Path of file to be created.

   dir           Root directory from which operation is performed. Paths in
               checksums file will be relative to this.

   input_spec    One or more paths relative to <dir> arg, which may include
               wildcards.

$( basename $0 ) "archive" [-e:] output_archive dir input_spec...

      Creates a delta archive, where items in new archive are only those that are not
    in the old archive or whose content has changed (as determined by md5 checksum).

ARGS:

   output_archive
                  Path of delta archive that is to be generated.

   dir           Root directory from which operation is performed. Paths in
               checksums file and archive will be relative to this.

   input_spec    One or more paths relative to <dir> arg, which may include
               wildcards.

OPTIONS:

   -e existing_checksum
                  Path of checksum file for already existing archived files.
EOF

    if [ ! -z "$1" ]; then
	echo >&2
	echo $* >&2
    fi

    exit 1
}

if [ -z "$1" ]; then
    usage
fi

set -e

function canonpath()
{
    python -c "import os.path; print os.path.realpath('$1')"
}

function canondirname()
{
    dirname `canonpath "$1"`
}

SRC_DIR=$( canondirname $BASH_SOURCE )

function find_input_files()
{
    local dir="$1"
    shift
    local input="$@"

    (
	set +e
	cd $dir >/dev/null #larryy tends to make noisy cd
	for i in $input; do
	    if [ -f $i ]; then
		ls $i 2>/dev/null
	    elif [ -d $i ]; then
		find $i -type f 2>/dev/null
	    fi
	done
		
	exit 0
    )
}

function compute_checksum()
{
    # convert 'MD5($path)= $checksum' to '$path\t$checksum'
    pattern='MD5(\(.*\))= '
    subst='\1'$'\t'
    openssl md5 $@ | sed "s/$pattern/$subst/"
}

function archive()
{
    $SRC_DIR/archive.sh "$@"
}

MODE=$1

if [ "$MODE" == "checksums" ]; then
    ########################################
    ###
    ### MODE checksums
    ###
    ########################################
    output_checksum="$2"
    dir=$( canonpath $3 )
    shift 3
    input=$( find_input_files $dir "$@" )

    if [ -z "$input" ]; then
	rm -f $output_checksum
	touch $output_checksum
	exit
    fi

    output_tmp=/tmp/archive_delta.$$
    rm -f $output_tmp
    (
	cd $dir

	for file in $input; do
	    compute_checksum $file >> $output_tmp
	done
    )

    mv $output_tmp $output_checksum

    exit 0
fi

if [ "$MODE" != "archive" ]; then
    usage
fi

########################################
###
### MODE archive
###
########################################

shift

existing_checksum=""

while getopts "e:" opt; do
    case $opt in
	e)
	    existing_checksum="$OPTARG"
	    ;;
	*)
	    exit 1
	    ;;
    esac
done

shift $(( $OPTIND - 1 ))
if [ $# -lt 3 ]; then
    usage "Missing arguments"
fi

output_archive="$1"
dir=$( canonpath $2 )
shift 2
input_files=$( find_input_files $dir "$@" )

if [ -z "$input_files" ]; then
    echo "No input files to archive" >&2 
    exit
fi

###
### If no old archive, then just create one
###
if [ -z "$existing_checksum" ] || [ ! -e $existing_checksum ]; then
    cd $dir
    archive pack $output_archive $input_files
    exit
fi

###
### We're doing a delta
###
tmpdir=$( mktemp -d /tmp/archive_delta.XXXXXXXX )

(
    cd $dir
    for x in $input_files; do
	compute_checksum $x >> $tmpdir/input_checksum
    done
)

###
### Figure out which files have actually changed or been added
###

archive_input_files=$(
    # concat old and new path/checksum info, with "old" or "new" prefixed to each line
    (
	cat $tmpdir/input_checksum | while read x; do printf "new\t$x\n"; done
	cat $existing_checksum | while read x; do printf "old\t$x\n"; done
    ) |
    # sort by path. Note is stable to keep new info before old info
    sort -s -k 2,2 |
    # filter out non-unique lines (ignoring new/old prefix).
    uniq -u -f 1 |
    # reorder columns to be "old/new   $checksum   $path"
    awk '{print $1"\t"$3"\t"$2}' |
    # combine repeated lines, ignoring new/old and checksum
    uniq -f 2 |
    # only use lines starting with "new", are files that are either being added
    # or have been modified since the old archive.
    grep "^new" |
    # cut the path
    cut -f 3
)

###
### CREATE DELTA ARCHIVE
###
if [ ! -z "$archive_input_files" ]; then
    (
	cd $dir
	archive pack $output_archive $archive_input_files
    )
fi

rm -rf tmpdir