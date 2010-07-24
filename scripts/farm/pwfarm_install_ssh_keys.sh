#!/bin/bash

mode=$1

function err()
{
    echo "$*">&2
    exit 1
}

function canonpath()
{
    python -c "import os.path; print os.path.realpath('$1')"
}

function canondirname()
{
    dirname `canonpath "$1"`
}

pwfarm_dir=`canondirname "$0"`
dispatcher="$pwfarm_dir/pwfarm_dispatcher.sh"

case "$mode" in
    "farmer")
	pwhostnumbers=$2
	if [ ! -e "$pwhostnumbers" ]; then
	    err "Cannot locate pwhostnumbers file: $pwhostnumbers"
	fi
	user=$3
	if [ -z "$3" ]; then
	    err "Must specify user."
	fi

	rsafile=~/.ssh/id_rsa.pub
	if [ ! -e "$rsafile" ]; then
	    err "Cannot locate RSA public key file: $rsafile"
	fi

	tmpdir=`mktemp -d /tmp/pwfarm_install_ssh_keys.XXXXXXXX` || exit 1

	cp $rsafile $tmpdir
	cp $0 $tmpdir
	cd $tmpdir

	zip -r payload.zip .

	$dispatcher clear $pwhostnumbers
	$dispatcher dispatch $pwhostnumbers $user $tmpdir/payload.zip './pwfarm_install_ssh_keys.sh field'

	rm -rf $tmpdir
	;;

    "field")
	keys=~/.ssh/authorized_keys
	rsafile=./id_rsa.pub

	if [ ! -e $keys ]; then
	    cp $rsafile $keys
	elif ! grep "`cat $rsafile`" $keys ; then
	    echo cat $rsafile >> $keys
	else
	    echo "Keys up to date"
	fi
	;;

    *)
	err "Invalid mode." >&2
	;;
esac