#!/bin/bash

function err()
{
    echo "$0: $*" >&2
    exit 1
}

pwfarm_dir="`dirname $0`"

mode="$1"
pwhostnumbersfile="$2"
user="$3"

if [ "$mode" == "dispatch" ] ; then
    if screen -ls | grep "pwfarm_dispatcher"; then
	err pwfarm_dispatcher GNU Screen session already present!
    fi

    screen -d -m -S pwfarm_dispatcher

    payload="$4"
    command="$5"
fi

pwhostnumbers=`cat $pwhostnumbersfile`

for x in $pwhostnumbers; do
    id=`printf "%02d" $x`
    pwhostname=pw$id
    eval pwhost=\$$pwhostname

    status=pwfarm_status$id
    completion=pwfarm_completion$id
	    
    case "$mode" in 
	"dispatch")
	    title="$pwhostname - $command"

	    screen -S pwfarm_dispatcher -X screen -t "$title" \
		$pwfarm_dir/pwfarm_farmer.sh $status $pwhost $x $user "$payload" "$command" $completion
	    ;;

	"clear")
	    rm -f $status
	    rm -f $completion
	    ;;

	"exit")
	    echo $completion
	    if [ ! `cat $completion` == "yay" ]; then
		echo "FAIL!"
		exit 1
	    fi
	    ;;
	*)
	    err "Invalid mode: $mode"
	    ;;
    esac
done

if [ "$mode" == "dispatch" ] ; then
    # kill window 0... it just has bash
    screen -S pwfarm_dispatcher -p 0 -X kill
    # bring up screen, starting at the windowlist
    screen -S pwfarm_dispatcher -p = -r

    $0 "exit" $2 $3
fi

if [ "$mode" == "exit" ]; then
    echo "YAY!"
    exit 0
fi