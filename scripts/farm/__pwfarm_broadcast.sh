#!/bin/bash

source $( dirname $BASH_SOURCE )/__lib.sh || exit 1

payload="$1"
require "$payload" "payload arg"
dest="$2"
require "$dest" "dest arg"

FIELDNUMBERS=( $( pwenv fieldnumbers ) ) || exit 1
OSUSER=$( pwenv osuser )
hostname_master=$( fieldhostname_from_num ${FIELDNUMBERS[0]} )
host_master=$( fieldhost_from_name $hostname_master )

echo Transfering $payload to $hostname_master...
repeat_til_success scp "$payload" "$OSUSER@$host_master:$dest"


for (( index=1; index<${#FIELDNUMBERS[@]}; index++ )); do
    hostname=$( fieldhostname_from_num ${FIELDNUMBERS[$index]} )
    host=$( fieldhost_from_name $hostname )

    echo Transfering $payload from $hostname_master to $hostname...
    repeat_til_success scp "$OSUSER@$host_master:$dest" "$OSUSER@$host:$dest"
done
