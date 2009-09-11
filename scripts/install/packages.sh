#!/bin/bash

PFM=`uname`

case "$PFM" in
    'Linux')
	. `dirname $0`/packages-linux.sh
	;;
    *)
	echo "Unsupported OS: $PFM"
esac