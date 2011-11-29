#!/bin/bash

pwhostnumbersfile="$1"
user="$2"
payload="$3"
dest="$4"

#
# Util function for executing command until it succeeds
#
function try
{
    while ! $*; do
	echo "FAILED: $*"
	# give user chance to ctrl-c
	sleep 5
    done
}

pwhostnumbers=(`cat $pwhostnumbersfile`)

function pwhostname_from_num()
{
    id=$( printf "%02d" $1 )
    echo pw$id
}

function pwhost_from_name()
{
    eval pwhost=\$$1
    echo $pwhost
}

pwhostname_master=$( pwhostname_from_num ${pwhostnumbers[0]} )
pwhost_master=$( pwhost_from_name $pwhostname_master )

echo Transfering $payload to $pwhostname_master...
try scp "$payload" "$user@$pwhost_master:$dest"


for (( index=1; index<${#pwhostnumbers[@]}; index++ )); do

    pwhostname=$( pwhostname_from_num ${pwhostnumbers[$index]} )
    pwhost=$( pwhost_from_name $pwhostname )

    echo Transfering $payload from $pwhostname_master to $pwhostname...
    try scp "$user@$pwhost_master:$dest" "$user@$pwhost:$dest"
done
