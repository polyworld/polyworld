#!/bin/bash

function install() {
    if dpkg -s $1 > /dev/null 2>&1 ; then
	return
    fi

    sudo apt-get --assume-yes install $*
    if test $?; then
	exit 1
    fi
}

packages="\
	g++ \
	libgsl0-dev \
	libqt4-opengl-dev \
	scons \
"

for pkg in $packages; do
    install $pkg
done