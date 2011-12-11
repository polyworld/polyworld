#!/bin/bash

source $( dirname $BASH_SOURCE )/__lib.sh || exit 1

payload="$1"
require "$payload" "payload arg"
dest="$2"
require "$dest" "dest arg"

farm_fieldnumbers=( $(pwquery fieldnumbers $(pwenv farmname)) )
fieldnumber_master=${farm_fieldnumbers[0]}
hostname_master=$( fieldhostname_from_num $fieldnumber_master )
host_master=$( fieldhost_from_name $hostname_master )

fieldnumbers=( $( pwenv fieldnumbers ) ) || exit 1
osuser=$( pwenv osuser )

echo Transfering $payload to $hostname_master...
repeat_til_success scp "$payload" "$osuser@$host_master:$dest"


for (( index=0; index<${#fieldnumbers[@]}; index++ )); do
    hostname=$( fieldhostname_from_num ${fieldnumbers[$index]} )
    if [ "$hostname" != "$hostname_master" ]; then
	host=$( fieldhost_from_name $hostname )

	echo Transfering $payload from $hostname_master to $hostname...
	( repeat_til_success scp "$osuser@$host_master:$dest" "$osuser@$host:$dest" ) &
    fi
done

wait
