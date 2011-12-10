#!/bin/bash

if [ -z "$PWFARM_SCRIPTS_DIR" ]; then
    source $( dirname $BASH_SOURCE )/__pwfarm_runutil.sh || exit 1
else
    source $PWFARM_SCRIPTS_DIR/__pwfarm_runutil.sh || exit 1
fi

function usage()
{
    cat <<EOF
usage: $( basename $0 ) [-tgo:] run_id|/

    Deletes run data from farm. Either specify run id, or '/' for all. By default,
operates on runs in failed directory and failed orphans.

OPTIONS:

    -n         Don't delete anything, just show what data would be operated on.

    -g         Operate on good runs and good orphans.

    -f fields
               Specify fields on which this should run. Must be a single argument,
            so use quotes. e.g. -f "0 1" or -f "{0..3}"

    -o owner
               Specify run owner, which is prepended to run ID. "nil" for no owner.
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

testing=false
good=false
owner=$( pwenv pwuser )
owner_override=false

while getopts "ngf:o:" opt; do
    case $opt in
	n)
	    testing=true
	    ;;
	g)
	    good=true
	    ;;
	f)
	    __pwfarm_config env set fieldnumbers "$OPTARG"
	    validate_farm_env
	    ;;
	o)
	    owner="$OPTARG"
	    owner_override=true
	    ;;
	*)
	    exit 1
	    ;;
    esac
done

args="$@"
shift $(( $OPTIND - 1 ))
if [ $# != 1 ]; then
    usage
fi

runid="$1"

if ! $field; then
    validate_farm_env

    if [ "$runid" == "/" ] && $good && ! $testing; then
	read -p "You are about to delete all good runs! Are you sure? [y/n]: " response
	if [ "$response" != "y" ]; then
	    exit 1
	fi
    fi

    __pwfarm_script.sh $0 --field $args
else
    function dorm()
    {
	local rmpath="$1"

	if $testing; then
	    file $rmpath
	    if [ -d $rmpath ]; then
		echo "CONTENTS:"
		ls $(ls_color_opt) -lF $rmpath
	    fi
	else
	    rm -rf $rmpath
	fi
    }

    if [ "$runid" == "/" ]; then
	runid_full=""
    else
	runid_full="$runid"
    fi

    if [ "$owner" == "nil" ]; then
	runid_full="$runid_full"
    else
	runid_full="$owner/$runid_full"
    fi

    if ! $owner_override; then
	orphan="$POLYWORLD_PWFARM_APP_DIR/run"
	orphanid="$POLYWORLD_PWFARM_APP_DIR/runid"
	if [ -e "$orphan" ] && [ -e "$orphanid" ]; then
	    if [ "$runid" == "/" ] || [ $(cat "$orphanid") == "$runid_full" ]; then
		if lock_app; then
		    if ($good && is_good_run "$orhpan") || (! $good && ! is_good_run "$orphan"); then
			dorm "$orphan"
		    fi
		    unlock_app
		fi
	    fi
	fi
    fi

    if $good; then
	dorm $( stored_run_path "good" "$runid_full" )
    else
	dorm $( stored_run_path "failed" "$runid_full" )
    fi

    if $testing; then
	read -p "press enter..."
    fi
fi