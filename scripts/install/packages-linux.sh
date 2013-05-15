#!/bin/bash -x

function install() {
    if dpkg -s $1 > /dev/null 2>&1 ; then
	return
    fi

    sudo apt-get --assume-yes install $* || exit 1
}

packages="\
	g++ \
	libgsl0-dev \
	libqt4-opengl-dev \
	scons \
	gnuplot \
	zlib1g-dev \
	python2.7-dev \
"

for pkg in $packages; do
    install $pkg
done