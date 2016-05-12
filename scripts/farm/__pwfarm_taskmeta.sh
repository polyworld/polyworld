#!/bin/bash

function __pwfarm_taskmeta()
{
    local mode=$1
    local path=$2

    case $mode in
	"has")
	    local propname=$3
	    grep ^${propname}$'\t' $path> /dev/null
	    ;;
	"get")
	    local propname=$3
	    __pwfarm_taskmeta has $path $propname && grep ^${propname}$'\t' $path | sed s/^${propname}$'\t'//
	    ;;
	"set")
	    local propname=$3
	    local propval="$4"

	    local tmp=/tmp/__pwfarm_taskmeta.$$

	    echo "$propname"$'\t'"$propval" > $tmp

	    if [ -e $path ]; then
		cat $path |
		grep -v ^${propname}$'\t' >> $tmp
	    fi

	    mv $tmp $path
	    ;;
	"validate")
	    assert [ $(__pwfarm_taskmeta get $path "id" | wc -w) == "1" ]
	    assert __pwfarm_taskmeta has $path "command"
	    ;;
	"create_field_tasks")
	    if [ "$3" == "--noprompterr" ]; then
		local prompterr=false
		shift
	    else
		local prompterr=true
	    fi
	    if [ "$3" == "--sudo" ]; then
		local sudo=true
		shift
	    else
		local sudo=false
	    fi
	    local cmd="$3"
	    local outputdir="$4"

	    require "$cmd" "create_field_tasks cmd"
	    require "$outputdir" "create_field_tasks outputdir"

	    mkdir -p $path

	    taskid=0

	    for x in $( pwenv fieldnumbers ); do
		local path_task=$path/$x
		rm -f $path_task ; touch $path_task
		__pwfarm_taskmeta set $path_task id $taskid
		__pwfarm_taskmeta set $path_task required_field $x
		__pwfarm_taskmeta set $path_task command "$cmd"
		if [ "$outputdir" != "nil" ]; then
		    __pwfarm_taskmeta set $path_task outputdir "${outputdir}_${x}"
		fi
		__pwfarm_taskmeta set $path_task prompterr "$prompterr"
		__pwfarm_taskmeta set $path_task sudo "$sudo"
		
		taskid=$(( $taskid + 1 ))

		echo $path_task
	    done
	    ;;
	*)
	    err "Invalid mode($mode)"
	    ;;
    esac
}