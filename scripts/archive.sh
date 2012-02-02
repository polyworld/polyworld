#!/bin/bash

function err()
{
    echo "$( basename $0 ):" "$*">&2
    exit 1
}

function usage()
{
################################################################################
    cat >&2 <<EOF
USAGE 

    $( basename $0 ) pack <output_archive> <input>...

        -x <pattern> Exclude pattern. Use a -x for each pattern.

    $( basename $0 ) unpack [-ed:x:] <input_archive>

        -e           Echo command to stdout -- for use with \$()
        -d <dir>     Specify dir to which files are unpacked

    $( basename $0 ) ls <input_archive>

Supported file extensions: zip, tbz
EOF
    exit 1
}

function get_type()
{
    local path="$1"
    local ext=${path##*.}

    if [ "$ext" == "zip" ] || [ "$ext" == "tbz" ]; then
	echo $ext
    else
	err "Unsupported archive file extension: $ext"
    fi
}

function TAR()
{
    local _echo=false
    if [ "$1" == "--echo" ]; then
	_echo=true
	shift
    fi

    local cmd="
	if which gnutar > /dev/null; then
	    gnutar $@;
	else
	    tar $@;
	fi
    "

    if $_echo; then
	echo $cmd
    else
	eval $cmd
    fi
}

if [ $# -lt 1 ]; then
    usage
fi

MODE=$1

case $MODE in
    pack)
	declare -a EXCLUDES

	shift # ignore MODE

	while getopts "x:" opt; do
	    case $opt in
		x)
		    EXCLUDES[${#EXCLUDES[@]}]="$OPTARG"
		    ;;
		*)
		    exit 1
		    ;;
	    esac
	done
	shift $(( $OPTIND - 1 ))

	ARCHIVE=$1
	[ ! -z $ARCHIVE ] || err "Missing archive argument"
	TYPE=$( get_type "$ARCHIVE" ) || exit 1
	shift
	[ $# -gt 0 ] || err "Missing input files"

	case $TYPE in
	    zip)
		xopt=""
		for x in "${EXCLUDES[@]}"; do
		    xopt="$xopt -x '$x'"
		done

		eval zip -rq "$ARCHIVE" "$@" $xopt
		;;
	    tbz)
		xopt=""
		for x in "${EXCLUDES[@]}"; do
		    xopt="$xopt --exclude '$x'"
		done

		TAR -cjf "$ARCHIVE" $xopt "$@"
		;;
	    *)
		err "Unhandled pack type"
		;;
	esac
	;;
    unpack)
	ECHO=false
	DEST=""

	shift # ignore MODE

	while getopts "ed:" opt; do
	    case $opt in
		e)
		    ECHO=true
		    ;;
		d)
		    DEST="$OPTARG"
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

	ARCHIVE=$1
	[ ! -z $ARCHIVE ] || err "Missing archive argument"
	TYPE=$( get_type "$ARCHIVE" ) || exit 1
	shift
	[ $# == 0 ] || err "Unexpected args"	

	case $TYPE in
	    zip)
		if [ ! -z "$DEST" ]; then
		    dopt="-d $DEST"
		fi
		CMD="unzip $dopt -qo $ARCHIVE"
		;;
	    tbz)
		if [ ! -z "$DEST" ]; then
		    dopt="-C $DEST"
		fi
		CMD=$( TAR --echo -xjf $ARCHIVE $dopt )
		;;
	    *)
		err "Unhandled pack type"
		;;
	esac

	if [ ! -z "$DEST" ]; then
	    CMD="mkdir -p $DEST && $CMD"
	fi

	if $ECHO; then
	    echo eval \"$CMD\"
	else
	    eval $CMD
	fi
	;;
    ls)
	ARCHIVE=$2
	[ ! -z $ARCHIVE ] || err "Missing archive argument"
	TYPE=$( get_type "$ARCHIVE" ) || exit 1
	shift 2
	[ $# == 0 ] || err "Unexpected args"

	case $TYPE in
	    zip)
		unzip -l "$ARCHIVE"
		;;
	    tbz)
		TAR -tf "$ARCHIVE"
		;;
	    *)
		err "Unhandled ls type"
		;;
	esac
	;;
    *)
	err "Invalid mode ($MODE)"
	;;
esac