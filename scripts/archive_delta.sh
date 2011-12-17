#!/bin/bash

function usage()
{
    cat >&2 <<EOF
usage: $( basename $0 ) [-e:v] output_checksum output_archive input_files...

      Creates a delta archive, where items in new archive are only those that are not
    in the old archive or whose content has changed (as determined by md5 checksum).

ARGS:

   output_checksum 
                  Path of file to which checksums of all archived files should be
                written (includes files only in old archive).

   output_archive
                  Path of delta zip that is to be generated.

OPTIONS:

   -e existing_checksum
                  Path of checksum file for already existing archived files. In practice,
               should be the output_checksum of a previous invocation of this script.

   -v             Verbose output.
EOF

    if [ ! -z "$1" ]; then
	echo >&2
	err $*
    fi

    exit 1
}

if [ -z "$1" ]; then
    usage
fi

set -e

existing_checksum=""
isverbose=false

while getopts "e:v" opt; do
    case $opt in
	e)
	    existing_checksum="$OPTARG"
	    ;;
	v)
	    isverbose=true
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

output_checksum="$1"
output_archive="$2"
shift 2
input_files="$@"


function verbose()
{
    if $isverbose; then
	(
	    $*
	) | while read x; do printf "archive_delta: $x\n"; done
    fi
}

function compute_checksum()
{
    # convert 'MD5($path)= $checksum' to '$path\t$checksum'
    pattern='MD5(\(.*\))= '
    subst='\1'$'\t'
    openssl md5 $@ | sed "s/$pattern/$subst/"
}

###
### If no old archive, then just create one
###
if [ -z $existing_checksum ] || [ ! -e $existing_checksum ]; then
    verbose echo "=== NO EXISTING CHECKSUM ==="

    rm -f $output_archive

    for file in "$@"; do
	compute_checksum $file >> $output_checksum
    done

    verbose echo "--- checksums ---"
    verbose cat $output_checksum
    verbose echo "-----------------"

    zip -q $output_archive "$@"

    verbose echo "--- archive ---"
    verbose unzip -l $output_archive
    verbose echo "---------------"

    exit
fi

verbose echo "=== DELTA ==="
verbose echo "--- existing checksums ---"
verbose cat $existing_checksum
verbose echo "--------------------------"

###
### We're doing a delta
###
tmpdir=$( mktemp -d /tmp/archive_delta.XXXXXXXX )

for x in $input_files; do
    compute_checksum $x >> $tmpdir/input_checksum
done

verbose echo "--- input checksums ---"
verbose cat $tmpdir/input_checksum
verbose echo "--------------------------"

###
### Figure out which files have actually changed or been added
###

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
# print normal checksum format: "$path   $checksum"
awk '{print $3"\t"$2}' > $tmpdir/modified_checksum

verbose echo "--- modified checksums ---"
verbose cat $tmpdir/modified_checksum
verbose echo "--------------------------"

###
### Generate checksum file reflecting old archive and our updates
###

# concat new and old 
(
    cat $tmpdir/modified_checksum
    cat $existing_checksum
) |
# sort on path. stable to keep new on top
sort -s -k 1,1 |
# put checksum first so we can ignore it with uniq
awk '{print $2"\t"$1}' |
# combine lines with same path, ignoring checksum.
uniq -f 1 |
# restore column order
awk '{print $2"\t"$1}' > $tmpdir/output_checksum

# we wrote this to a tmp location in case the output checksum is the same
# path as the existing checksum
cp $tmpdir/output_checksum $output_checksum

verbose echo "--- output checksums ---"
verbose cat $tmpdir/output_checksum
verbose echo "--------------------------"

###
### CREATE DELTA ARCHIVE
###
zip_input_files="$(cut -f 1 $tmpdir/modified_checksum)"
if [ ! -z "$zip_input_files" ]; then
    zip -q $output_archive $(cut -f 1 $tmpdir/modified_checksum)

    verbose echo "--- archive ---"
    verbose unzip -l $output_archive
    verbose echo "---------------"
else
    verbose echo "NOTHING TO ARCHIVE"
fi

rm -rf tmpdir