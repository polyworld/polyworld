#!/bin/bash

if [ $# -lt 2 ]; then
    echo "\
usage: $(basename $0) username machine_id...

example: $(basename $0) larryy 0 1 2 3 4 5 6 7 8 9
         $(basename $0) larryy {0..9}
"
exit 1
fi

function err()
{
    echo "$*">&2
    exit 1
}

function pwhostnames_from_nums()
{
    for x in $*; do 
	id=$( printf "%02d" $x 2>/dev/null ) || err "Invalid host number: $x"
	pwhostname=pw$id
	echo $pwhostname
    done
}

function pwhost_from_name()
{
    eval pwhost=\$$1
    echo $pwhost
}

function step()
{
    echo ......
    echo ...... $*
    echo ......
}

function substep()
{
    echo ..
    echo .. $*
    echo ..
}

user="$1"
shift
pwnums="$*"
pwhostnames=( $(pwhostnames_from_nums $pwnums) ) || exit 1

#
# Try pinging all the hosts
#
step "Pinging all hosts"

for hostname in ${pwhostnames[*]}; do
    substep "Pinging $hostname..."

    host=$( pwhost_from_name $hostname )
    ping -c 1 $host || err Cannot ping $hostname
done

pwhostname_master=${pwhostnames[0]}
pwhost_master=$( pwhost_from_name $pwhostname_master )

#
# Make sure we have public key
#
path_rsa=~/.ssh/id_rsa
path_rsa_pub=${path_rsa}.pub

if [ ! -e $path_rsa_pub ]; then
    echo "!!!"
    echo "!!! Enter a blank password. Just press enter when prompted."
    echo "!!!"
    ssh-keygen -f $path_rsa -P ""
fi
if [ ! -e $path_rsa_pub ]; then
    err "No public key at $path_rsa_pub"
fi

key_data=$( cat $path_rsa_pub )

#
# Install key on master
#

keys="~/.ssh/authorized_keys"

key_install="
  if [ ! -e $keys ]; then echo \"$key_data\" > $keys;
  elif ! grep -q \"$key_data\" $keys; then echo \"$key_data\" >> $keys;
  else echo Public key already installed
  fi
"

step "Installing key on $pwhostname_master"

ssh -l $user $pwhost_master "$key_install"  || err Failed installing public key

#
# Ensure master can ping other hosts
#
step "Pinging all hosts from $pwhostname_master"

ssh -l $user $pwhost_master "
  for hostname in ${pwhostnames[*]}; do
    echo ..
    echo .. \$hostname
    echo ..
    eval host=\\\$\$hostname
    if [ -z \"\$host\" ]; then
      echo Cannot resolve \\\$\$hostname>&2
      exit 1
    fi
    ping -c 1 \$host || exit 1
  done
" || err "Failed pinging hosts"

#
# Install key on all hosts
#
step "Installing key on all hosts"

for pwhostname in ${pwhostnames[*]}; do
    if [ "$pwhostname" == "$pwhostname_master" ]; then continue; fi

    substep $pwhostname

    ssh -l $user $pwhost_master "
      ssh \$$pwhostname \"
        if [ ! -e $keys ]; then echo \\\"$key_data\\\" > $keys;
        elif ! grep -q \\\"$key_data\\\" $keys; then echo \\\"$key_data\\\" >> $keys;
        else echo Public key already installed
        fi
      \"
    "
done

#
# Update known_hosts
#
step "Updating known_hosts"

for pwhostname in ${pwhostnames[*]}; do
    substep $pwhostname
    pwhost=$( pwhost_from_name $pwhostname )
    # remove any previous entry
    ssh-keygen -R $pwhost
    # add entry
    ssh-keyscan -H $pwhost >> ~/.ssh/known_hosts 
done

#
# Verify ssh into all hosts 
#
step "Verifying ssh into all hosts"

for pwhostname in ${pwhostnames[*]}; do
    substep $pwhostname
    pwhost=$( pwhost_from_name $pwhostname )
    
    ssh -l $user $pwhost "hostname" || err "Failed ssh on $pwhostname"
done

#
# Verify screen on all hosts 
#
step "Verifying screen on all hosts"

for pwhostname in ${pwhostnames[*]}; do
    substep $pwhostname
    pwhost=$( pwhost_from_name $pwhostname )
    
    ssh -l $user $pwhost "which screen" || err "Failed finding screen on $pwhostname"
done

#
# Verify svn on all hosts 
#
step "Verifying svn on all hosts"

for pwhostname in ${pwhostnames[*]}; do
    substep $pwhostname
    pwhost=$( pwhost_from_name $pwhostname )
    
    ssh -l $user $pwhost "which svn" || err "Failed finding svn on $pwhostname"
done

#
# Write config files
#
step "Generating pwfarm config files"

mkdir -p -m go-w ~/polyworld_pwfarm/etc/
echo $pwnums > ~/polyworld_pwfarm/etc/pwhostnumbers
echo $user > ~/polyworld_pwfarm/etc/pwuser
