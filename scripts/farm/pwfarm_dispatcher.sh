#!/bin/bash

function err()
{
    echo "$0: $*" >&2
    exit 1
}

function try_screen()
{
    if screen -ls | grep "pwfarm_dispatcher"; then
	err pwfarm_dispatcher GNU Screen session already present!
    fi
}

function init_screen()
{
    try_screen

    screen -d -m -S pwfarm_dispatcher
}

pwfarm_dir="`dirname $0`"

if [ "$1" == "--password" ]; then
    shift
    password="true"
else
    password="false"
fi

mode="$1"

case "$mode" in
    "dispatch")
	pwhostnumbersfile="$2"
	user="$3"
	payload="$4"
	command="$5"
	output_basename="$6"
	output_dir="$7"

	if [ -e "pwfarm_dispatcher.dispatch" ]; then
	    err "./pwfarm_dispatcher.dispatch exists! Do you need to run recover?"
	fi
	try_screen

	#
	# Broadcast field script to fields
	#
	$pwfarm_dir/pwfarm_broadcast.sh "$pwhostnumbersfile" "$user" "$pwfarm_dir/pwfarm_field.sh" "~/pwfarm_field.sh"

	#
	# Broadcast payload to fields
	#
	$pwfarm_dir/pwfarm_broadcast.sh "$pwhostnumbersfile" "$user" "$payload" "~/pwfarm_payload.zip"

	init_screen

	(
	    echo $password
	    echo $pwhostnumbersfile
	    echo $user
	    echo $payload
	    echo $command
	    echo $output_basename
	    echo $output_dir
	) > "pwfarm_dispatcher.dispatch"

	;;
    "recover")
	if [ ! -e "pwfarm_dispatcher.dispatch" ]; then
	    err "Cannot locate ./pwfarm_dispatcher.dispatch, needed for recover. Are you in the right dir?"
	fi

	function field()
	{
	    head pwfarm_dispatcher.dispatch -n $(( $1 + 1 )) | tail -n 1
	}

	password=$( field 0 )
	pwhostnumbersfile=$( field 1 )
	user=$( field 2 )
	payload=$( field 3 )
	command=$( field 4 )
	output_basename=$( field 5 )
	output_dir=$( field 6 )

	init_screen
	;;
    "clear")
	echo "Clearing pwfarm state. This could take a while."
	pwhostnumbersfile="$2"
	user="$3"

	if screen -ls | grep "pwfarm_dispatcher" > /dev/null; then
	    screen -S pwfarm_dispatcher -X quit
	fi

	rm -f "pwfarm_dispatcher.dispatch"
	;;
    "exit")
	pwhostnumbersfile="$2"
	user="$3"
	exitval=0
	;;
    *)
	err "Invalid mode: $mode"
	;;
esac

if [ "$password" == "true" ]; then
    echo -n "Please enter password of administrator on farm machines (for sudo): " >&2
    # turn off echo for reading password
    stty -echo
    read PWFARM_PASSWORD
    # turn echo back on
    stty echo
else
    PWFARM_PASSWORD="nil"
fi 

pwhostnumbers=`cat $pwhostnumbersfile`

for x in $pwhostnumbers; do
    id=`printf "%02d" $x`
    pwhostname=pw$id
    eval pwhost=\$$pwhostname

    status=pwfarm_status$id
    completion=pwfarm_completion$id

    function spawn_farmer()
    {
	title="$pwhostname - $command"

	screen -S pwfarm_dispatcher -X screen -t "$title" \
	    $pwfarm_dir/pwfarm_farmer.sh $status $pwhost $x $user "$payload" "$PWFARM_PASSWORD" "$command" $output_basename $output_dir $completion
    }
	    
    case "$mode" in 
	"dispatch")
	    spawn_farmer
	    ;;

	"recover")
	    spawn_farmer
	    ;;

	"clear")
	    rm -f $status
	    rm -f ${status}_*
	    rm -f $completion
	    
	    ssh -l $user $pwhost "rm -f ~/pwfarm/$completion; if screen -ls | grep pwfarm > /dev/null; then echo Forcing quit of $pwhostname...; screen -S pwfarm -X quit; fi"
	    ;;

	"exit")
	    if [ ! "`cat $completion`" == "yay" ]; then
		exitval=1
	    fi
	    ;;
	*)
	    err "Invalid mode: $mode"
	    ;;
    esac
done

case "$mode" in
    "dispatch")
	# kill window 0... it just has bash
	screen -S pwfarm_dispatcher -p 0 -X kill
	# bring up screen, starting at the windowlist
	screen -S pwfarm_dispatcher -p = -r

	$0 "exit" $pwhostnumbersfile $user
	;;
    "recover")
	# kill window 0... it just has bash
	screen -S pwfarm_dispatcher -p 0 -X kill
	# bring up screen, starting at the windowlist
	screen -S pwfarm_dispatcher -p = -r

	$0 "exit" $pwhostnumbersfile $user
	;;
    "clear")
	;;
    "exit")
	$0 "clear" $pwhostnumbersfile $user
	echo -n "pwfarm task complete. SUCCESS="
	if [ "$exitval" == "0" ]; then
	    echo "TRUE"
	else
	    echo "FALSE"
	fi

	exit $exitval
	;;
esac