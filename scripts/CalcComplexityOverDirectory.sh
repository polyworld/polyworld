#!/bin/bash

#
# make sure environment is ok and fetch PW_* environment variables
#
. `dirname $0`/env_check.sh $0 complexity

# Make sure we're taking an argument
if [ -z "$1" ]
then
	echo "you must specify a directory containing brainFunction files to calculate the Complexity of";
	exit 1;
fi

# if there is a trailing slash, get rid of it.
directory=$(echo "${1}" | sed -e 's/\/*$//')

shift
args=$*

files=$(ls -1 "$directory" | awk '$1 ~ /brainFunction/ && $1 !~ /incomplete_/ { print $0 }' | sort -n -t '_' -k2,4 | awk "{print \"${directory}/\"\$0}")


${PW_CALC_COMPLEXITY} brainfunction --list $files -- $args
