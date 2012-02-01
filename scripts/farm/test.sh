#!/bin/bash

if [ "$1" != "--field" ]; then
    source $( dirname $BASH_SOURCE )/__lib.sh || exit 1

    fieldnumbers=( $( pwenv fieldnumbers ) ) || exit 1
    pwenv domains |
    while read domain; do
	user=$( echo $domain | cut -d ":" -f 1 )
	domain_fieldnumbers=( $( echo $domain | cut -d ":" -f 2 ) )

	echo $user
	echo $domain_fieldnumbers
    done

    rm -rf /tmp/test_output
    __pwfarm_script.sh --sudo --output /tmp/test_output/foo $0 --field $* || exit 1
    #__pwfarm_script.sh --output /tmp/test_output/foo $0 --field $* || exit 1

    find /tmp/test_output
else
    source $PWFARM_SCRIPTS_DIR/__lib.sh

    echo -n "ARGS: "
    echo $*
    echo -n "PWD: "
    pwd
    echo -n "TASKID: "
    PWFARM_TASKMETA get id
    echo -n "COMMAND: "
    PWFARM_TASKMETA get command
    echo -n "DATE: "
    date

    echo hi > a;
    mkdir bye
    echo foo > bye/b
    archive pack $PWFARM_OUTPUT_ARCHIVE .

    PWFARM_STATUS "TEST (Press Enter)"

    PWFARM_SUDO echo hi sudo

    read -p "[test] press enter (f for fail exitval)..." input
    if [ "$input" == "f" ]; then
	exit 1
    fi
fi