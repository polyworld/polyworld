#!/bin/bash

source $( dirname $BASH_SOURCE )/__lib.sh || exit 1

payload="$1"
require "$payload" "payload arg"
dest="$2"
require "$dest" "dest arg"

fieldnumbers=( $( pwenv fieldnumbers ) ) || exit 1

pwenv domains |
(
    kill_jobs_on_termination

    while read domain; do
	user=$( echo $domain | cut -d ":" -f 1 )
	domain_fieldnumbers=( $( echo $domain | cut -d ":" -f 2 ) )

	active_fieldnumbers=$(
	    (
		echo ${domain_fieldnumbers[@]}
		echo ${fieldnumbers[@]}
	    ) |
	    tr " " "\n" |
	    sort |
	    uniq -d
	)

	if [ ! -z "$active_fieldnumbers" ]; then
	    master_fieldnumber=${domain_fieldnumbers[0]}
	    master_hostname=$( fieldhostname_from_num $master_fieldnumber )
	    master_host=$( fieldhost_from_name $master_hostname )

	    echo "Transferring $payload to $master_hostname..."
	    (
		kill_jobs_on_termination

		repeat_til_success scp "$payload" "$user@$master_host:$dest"

		for fieldnumber in $active_fieldnumbers; do
		    if [ $fieldnumber != $master_fieldnumber ]; then
			hostname=$( fieldhostname_from_num $fieldnumber )
			echo "Transferring $payload from $master_hostname to $hostname..."
			(
			    host=$( fieldhost_from_name $hostname )
			    repeat_til_success scp "$user@$master_host:$dest" "$user@$host:$dest"
			) &
		    fi
		done

		wait
	    ) &
	fi
    done

    wait
)

exitval=$?
if [ $exitval == 0 ]; then
    echo "Transfer complete."
else
    echo "Transfer failed."
fi

exit $exitval