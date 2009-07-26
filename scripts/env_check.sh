#!/bin/bash
#
# Common Polyworld environment setup/verification
#
# 'source' this script in order to gain access to all the PW_* environment variables. If you have
# to invoke this in a sub-process (eg from python) then you can get values by using --echo and
# piping stdout.
#
# $1 = path of script invoking/sourcing this one
#
# $2..$n = ([--echo] <module>)*
#
#   --echo:  echo full module path to stdout
#
#   module:  complexity (CalcComplexity)
#            gnuplot
#

function err() {
    echo "env_check.sh:" "$1">&2
    exit 1
}

function parentdir() {
    readlink -f `dirname "$1"`/..
}

CALLER="$1"
shift

if [ -z "${CALLER}" ]; then
    err "must provide caller"
else
    if [ ! -f "${CALLER}" ]; then
	err "invalid caller (${CALLER})"
    fi
fi

. `dirname "${CALLER}"`/env_init.sh `parentdir "${CALLER}"`

if [ -z "${PW_HOME}" ]; then
    err "That's not possible! env_check.sh is confused."
fi

ECHO=0

for arg in "$@"; do
    ECHO_VAL=""

    case $arg in
	--echo)
	    ECHO=1
	    ;;
	complexity)
	    ECHO_VAL="${PW_CALC_COMPLEXITY}"
	    if [ ! -x "${ECHO_VAL}" ]; then
		err "Please build CalcComplexity -- execute 'make' in Polyworld home directory."
	    fi
	    ;;
	gnuplot)
	    ECHO_VAL="${PW_GNUPLOT}"
	    if [ ! -x "${ECHO_VAL}" ]; then
		err "Cannot locate 'gnuplot'. Please install or modify PATH."
	    fi
	    ;;
	open)
	    ECHO_VAL="${PW_OPEN}"
	    if [ ! -x "${ECHO_VAL}" ]; then
		err "Cannot determine open command."
	    fi
	    ;;
	*)
	    err "Invalid arg '$arg'"
	    ;;
    esac

    if (( ${ECHO} != 0 && "${#ECHO_VAL}" != 0 )); then
	echo "${ECHO_VAL}"
	ECHO=0
    fi
done
