#!/bin/bash

function __err() {
    echo "env_init.sh:" "$1">&2
    exit 1
}

case `uname` in
    Linux)
		PW_OS='linux'
		PW_OPEN=`which gnome-open`
		if [ -z "${PW_OPEN}" ]; then
			PW_OPEN=`which kde-open`
		fi
		;;
    Darwin)
		PW_OS='mac'
		PW_OPEN=`which open`
		;;
    *)
		__err "Unknown Operating System (`uname`)"
		;;
esac


if [ -z "$1" ]; then
    PW_HOME=${PWD}
else
    PW_HOME=${1}
fi

PW_SCRIPTS=${PW_HOME}/scripts
PW_TOOLS=${PW_HOME}/tools
PW_BIN=${PW_HOME}/bin

PW_CALC_COMPLEXITY=${PW_BIN}/CalcComplexity
PW_PROPUTIL=${PW_BIN}/proputil
PW_GNUPLOT=`which gnuplot`

if [ ! -e ${PW_SCRIPTS}/env_init.sh ]; then
    __err "Invalid Polyworld home directory (${PW_HOME})"
else
    export PW_HOME
    export PW_SCRIPTS
    export PW_TOOLS
    export PW_BIN
    export PW_CALC_COMPLEXITY
    export PW_PROPUTIL
    export PW_GNUPLOT

#    if [[ ${PATH} != *${PW_BIN}* ]]; then
#	PATH=${PW_BIN}:${PW_SCRIPTS}:${PATH}
#    fi
fi
