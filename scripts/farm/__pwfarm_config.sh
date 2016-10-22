#!/bin/bash

#
# This module is included by __lib.sh. We implement the whole thing as a function since it's used so much
# within $(), and it's noticably slow loading a new bash process for each invocation.
#

function __pwfarm_config()
{
    local prompt=true
    if [ "$1" == --noprompt ]; then
	shift
	prompt=false
    fi

    local mode=$1

    function __usage()
    {
	local progname=$( basename $0 )
	echo "\
usage: $progname define farm <farm_name> <pwuser> <dstdir> (<osuser> <field_number>...)+

          Define a farm.

          EXAMPLE:

            $progname define farm mini frank /farmruns adminX {0..9} adminY {100..109}

       $progname define session <farm_name> <session_name> [<field_number>...]

          Define a farm session.

       $progname env set <propname> <propvalue> [<subpropvalue>]...

          Set an environment value.

          EXAMPLE:

            . $progname env set session minifarm default

       $progname stat

          Print information about current configuration to stdout.

       $progname env get <propname>

          Get an environment value.

          EXAMPLES:

            $progname env get farmname
            $progname env get sessionname

       $progname query <propname> [<subpropname>]...

          Query a configuration value

          EXAMPLES:

            $progname query farmnames
            $progname query sessionnames minifarm
            $progname query fieldnumbers minifarm session_x" >&2
	if [ ! -z "$1" ]; then
	    echo >&2
	    echo $1 >&2
	fi
	exit 1
    }

    if [ "$#" == 0 ]; then
	__usage
    fi

    ###############################################################
    ###
    ### PROCESS USER REQUEST
    ###
    ###############################################################
    case $mode in
        ###############################################################
        ###
        ### ENV
        ###
        ###############################################################
	"env")
	    envop="$2"

	    case "$envop" in
		"export")
		    echo "export PWFARM_CONFIG_ENV__FARM=\"$PWFARM_CONFIG_ENV__FARM\" ;"
		    echo "export PWFARM_CONFIG_ENV__SESSION=\"$PWFARM_CONFIG_ENV__SESSION\" ;"
		    echo "export PWFARM_CONFIG_ENV__FIELD=\"$PWFARM_CONFIG_ENV__FIELD\" ;"
		    echo "export PWFARM_CONFIG_ENV__TASKID=\"$PWFARM_CONFIG_ENV__TASKID\" ;"
		    echo "export PWFARM_CONFIG_ENV__PWUSER=$( pwenv pwuser ) ;"
		    ;;
		"set")
		    envname="$3"

		    case "$envname" in
			"session")
			    farmname="$4"
			    require "$farmname" "'env set session' arg farmname"
			    sessionname="$5"
			    require "$sessionname" "'env set session' arg sessionname"

			    if ! $( pwquery defined session $farmname $sessionname ); then
				err "Undefined farm/session for 'env set session' ($farmname/$sessionname)"
			    fi

			    export PWFARM_CONFIG_ENV__FARM=$farmname
			    export PWFARM_CONFIG_ENV__SESSION=$sessionname
			    ;;
			"fieldnumber")
			    fieldnumber="$4"
			    require "$fieldnumber" "'env set fieldnumber' arg fieldnumber"

			    export PWFARM_CONFIG_ENV__FIELD=$fieldnumber
			    ;;
			"fieldnumbers")
			    shift 3
			    fieldnumbers=$( expand_int_list $* )
			    require "$fieldnumbers" "'env set fieldnumbers' arg fieldnumbers"

			    export PWFARM_CONFIG_ENV__FIELDNUMBERS="$fieldnumbers"
			    ;;
			"taskid")
			    taskid="$4"
			    require "$taskid" "'env set taskid' arg taskid"

			    export PWFARM_CONFIG_ENV__TASKID=$taskid
			    ;;
			*)
			    err "Invalid 'env set' name ($envname)"
			    ;;
		    esac
		    ;;
		"get")
		    envname="$3"

		    case "$envname" in
			"farmname")
			    echo $PWFARM_CONFIG_ENV__FARM
			    ;;
			"sessionname")
			    echo $PWFARM_CONFIG_ENV__SESSION
			    ;;
			"fieldnumber")
			    echo $PWFARM_CONFIG_ENV__FIELD
			    ;;
			"taskid")
			    echo $PWFARM_CONFIG_ENV__TASKID
			    ;;
			"pwuser")
			    if [ ! -z "$PWFARM_CONFIG_ENV__PWUSER" ]; then
				echo $PWFARM_CONFIG_ENV__PWUSER
			    else
				pwquery pwuser $PWFARM_CONFIG_ENV__FARM
			    fi
			    ;;
			"osuser")
			    if [ ! -z "$4" ]; then
				local fieldnumber="$4"
			    elif [ ! -z "$PWFARM_CONFIG_ENV__FIELD" ]; then
				local fieldnumber=$PWFARM_CONFIG_ENV__FIELD
			    else
				err "env get osuser requires field number"
			    fi

			    pwquery osuser $PWFARM_CONFIG_ENV__FARM $fieldnumber
			    ;;
			"runresults_dir")
			    pwquery runresults_dir $PWFARM_CONFIG_ENV__FARM
			    ;;
			"domains")
			    pwquery domains $PWFARM_CONFIG_ENV__FARM
			    ;;
			"fieldnumbers")
			    if [ ! -z "$PWFARM_CONFIG_ENV__FIELDNUMBERS" ]; then
				echo $PWFARM_CONFIG_ENV__FIELDNUMBERS
			    else
				pwquery sessionfieldnumbers $PWFARM_CONFIG_ENV__FARM $PWFARM_CONFIG_ENV__SESSION
			    fi
			    ;;
			"dispatcherstate_dir")
			    pwquery dispatcherstate_dir $PWFARM_CONFIG_ENV__FARM $PWFARM_CONFIG_ENV__SESSION
			    ;;
			"farmerstate_dir")
			    require "$PWFARM_CONFIG_ENV__FIELD" "'env get farmerstate_dir' requires field context"
			    pwquery farmerstate_dir $PWFARM_CONFIG_ENV__FARM $PWFARM_CONFIG_ENV__SESSION $PWFARM_CONFIG_ENV__FIELD
			    ;;
			"fieldstate_dir")
			    require "$PWFARM_CONFIG_ENV__FIELD" "'env get fieldstate_dir' requires field context"
			    pwquery fieldstate_dir $PWFARM_CONFIG_ENV__FARM $PWFARM_CONFIG_ENV__SESSION $PWFARM_CONFIG_ENV__FIELD $( pwenv pwuser )
			    ;;
			"fieldscreensession")
			    require "$PWFARM_CONFIG_ENV__FIELD" "'env get fieldscreensession' requires field context"
			    pwquery fieldscreensession $PWFARM_CONFIG_ENV__FARM $PWFARM_CONFIG_ENV__SESSION $PWFARM_CONFIG_ENV__FIELD $( pwenv pwuser )
			    ;;
			*)
			    err "Invalid 'env get' name ($envname)"
			    ;;
		    esac
		    ;;
		*)
		    err "Invalid envop ($envop)"
		    ;;
	    esac
	    ;; # end mode env

        ###############################################################
        ###
        ### STAT
        ###
        ###############################################################
	"stat")
	    for farmname in $( pwquery farmnames ); do
		echo "---"
		echo "--- FARM $farmname"
		echo "---"
		echo "user: $( pwquery pwuser $farmname )"
		echo "run results dir: $( pwquery runresults_dir $farmname )"
		echo "domains:"
		pwquery domains $farmname |
		sed "s|\(.*\):\(.*\)|osuser = \1 ; fieldnumbers = \2|" |
		indent
		echo "sessions:"
		for sessionname in $( pwquery sessionnames $farmname ); do
		    echo -n "$sessionname: "
		    if [ -e $(pwquery sessionfieldnumbers_path $farmname $sessionname) ]; then
			echo "fields = {$( pwquery sessionfieldnumbers $farmname $sessionname )}"
		    else
			echo "fields = {*}"
		    fi
		done |
		indent
		echo
		echo
	    done

	    echo "current farm: $( pwenv farmname 2>/dev/null || echo none )"
	    echo "current session: $( pwenv sessionname 2>/dev/null || echo none )"
	    echo "current fields: $( pwenv fieldnumbers 2>/dev/null || echo none )"
	    ;;

        ###############################################################
        ###
        ### QUERY
        ###
        ###############################################################
	"query")

	    propname="$2"

	    case "$propname" in
		"farmhome")
		    echo $HOME/pwfarm
		    ;;
		"farmetc_dir")
		    farmname="$3"
		    if [ -z "$farmname" ]; then
			err "Missing 'farmetc_dir' farmname arg"
		    fi
		    val=$( pwquery farmhome )/etc/farms/$farmname || exit 1
		    echo $val
		    ;;
		"farmstate_dir")
		    farmname="$3"
		    if [ -z "$farmname" ]; then
			err "Missing 'farmstate_dir' farmname arg"
		    fi
		    val=$( pwquery farmhome )/state/farms/$farmname || exit 1
		    echo $val
		    ;;
		"farmnames")
		    dir=$( pwquery farmhome ) || exit 1
		    ls -1 $dir/etc/farms
		    ;;
		"sessionetc_dir")
		    farmname="$3"
		    if [ -z "$farmname" ]; then
			err "Missing 'sessionetc_dir' farmname arg"
		    fi
		    sessionname="$4"
		    if [ -z "$sessionname" ]; then
			err "Missing 'sessionetc_dir' sessionname arg"
		    fi
		    val=$( pwquery farmetc_dir $farmname )/sessions/$sessionname || exit 1
		    echo $val
		    ;;
		"sessionstate_dir")
		    farmname="$3"
		    if [ -z "$farmname" ]; then
			err "Missing 'sessionstate_dir' farmname arg"
		    fi
		    sessionname="$4"
		    if [ -z "$sessionname" ]; then
			err "Missing 'sessionstate_dir' sessionname arg"
		    fi
		    val=$( pwquery farmstate_dir $farmname )/sessions/$sessionname || exit 1
		    echo $val
		    ;;
		"sessionnames")
		    farmname="$3"
		    if [ -z "$farmname" ]; then
			err "Missing 'sessionetc_dir' farmname arg"
		    fi
		    dir=$( pwquery farmetc_dir $farmname )/sessions || exit 1
		    if [ -e $dir ]; then
			ls -1 $dir
		    fi
		    ;;
		"sessionfieldnumbers_path")
		    farmname="$3"
		    if [ -z "$farmname" ]; then
			err "Missing 'sessionfieldnumbers_path' farmname arg"
		    fi
		    sessionname="$4"
		    if [ -z "$sessionname" ]; then
			err "Missing 'sessionfieldnumbers_path' sessionname arg"
		    fi
		    val=$( pwquery sessionetc_dir $farmname $sessionname )/fieldnumbers || exit 1
		    echo $val
		    ;;
		"sessionfieldnumbers")
		    farmname="$3"
		    if [ -z "$farmname" ]; then
			err "Missing 'sessionfieldnumbers' farmname arg"
		    fi
		    sessionname="$4"
		    if [ -z "$sessionname" ]; then
			err "Missing 'sessionfieldnumbers' sessionname arg"
		    fi
		    path=$( pwquery sessionfieldnumbers_path $farmname $sessionname ) || exit 1
		    if [ -e $path ]; then
			cat $path
		    else
			pwquery fieldnumbers $farmname
		    fi
		    ;;
		"dispatcherstate_dir")
		    farmname="$3"
		    if [ -z "$farmname" ]; then
			err "Missing 'dispatcherstate_dir' farmname arg"
		    fi
		    sessionname="$4"
		    if [ -z "$sessionname" ]; then
			err "Missing 'dispatcherstate_dir' sessionname arg"
		    fi
		    val=$( pwquery sessionstate_dir $farmname $sessionname )/dispatcher || exit 1
		    echo $val		
		    ;;
		"farmerstate_dir")
		    farmname="$3"
		    require "$farmname" "'farmerstate_dir' farmname arg"
		    sessionname="$4"
		    require "$sessionname" "'farmerstate_dir' sessionname arg"
		    fieldnumber="$5"
		    require "$fieldnumber" "'farmerstate_dir' fieldnumber arg"
		    val=$( pwquery dispatcherstate_dir $farmname $sessionname )/farmer${fieldnumber} || exit 1
		    echo $val		
		    ;;
		"fieldstate_dir")
		    farmname="$3"
		    require "$farmname" "'fieldstate_dir' farmname arg"
		    sessionname="$4"
		    require "$sessionname" "'fieldstate_dir' sessionname arg"
		    fieldnumber="$5"
		    require "$fieldnumber" "'fieldstate_dir' fieldnumber arg"
		    pwuser="$6"
		    require "$pwuser" "'fieldstate_dir' pwuser arg"

		    echo "\$HOME/pwfarm/state/fields/$pwuser/$farmname/$sessionname"
		    ;;
		"fieldscreensession")
		    farmname="$3"
		    require "$farmname" "'fieldscreensession' farmname arg"
		    sessionname="$4"
		    require "$sessionname" "'fieldscreensession' sessionname arg"
		    fieldnumber="$5"
		    require "$fieldnumber" "'fieldscreensession' fieldnumber arg"
		    pwuser="$6"
		    require "$pwuser" "'fieldscreensession' pwuser arg"

		    echo "____pwfarm_field__user_${pwuser}__farm_${farmname}__session_${sessionname}____"
		    ;;
		"pwuser_path")
		    farmname="$3"
		    val=$( pwquery farmetc_dir $farmname )/pwuser || exit 1
		    echo $val
		    ;;
		"pwuser")
		    farmname="$3"
		    path=$( pwquery pwuser_path $farmname ) || exit 1
		    cat $path
		    ;;
		"osuser")
		    farmname="$3"
		    fieldnumber="$4"

		    local domain=$( grep "\b$fieldnumber\b" $(pwquery domains_path $farmname) ) || exit 1

		    echo $domain | cut -d ":" -f 1
		    ;;
		"runresults_dir_path")
		    farmname="$3"
		    val=$( pwquery farmetc_dir $farmname )/runresults_dir || exit 1
		    echo $val
		    ;;
		"runresults_dir")
		    farmname="$3"
		    path=$( pwquery runresults_dir_path $farmname ) || exit 1
		    cat $path
		    ;;
		"domains_path")
		    farmname="$3"
		    val=$( pwquery farmetc_dir $farmname )/domains || exit 1
		    echo $val
		    ;;
		"domains")
		    farmname="$3"
		    path=$( pwquery domains_path $farmname ) || exit 1
		    cat $path
		    ;;
		"fieldnumbers")
		    farmname="$3"

		    pwquery domains $farmname |
		    cut -d ":" -f 2 |
		    tr "\n" " "

		    echo
		    ;;
		"defined")
		    type="$3"
		    if [ -z "$type" ]; then
			err "Missing 'defined' type arg"
		    fi
		    
		    case "$type" in
			"farm")
			    farmname="$4"
			    if [ -z "$farmname" ]; then
				err "Missing 'defined farm' farmname arg"
			    fi
			    if [ -e $( pwquery farmetc_dir $farmname ) ]; then
				echo "true"
			    else
				echo "false"
			    fi
			    ;;
			"session")
			    farmname="$4"
			    if [ -z "$farmname" ]; then
				err "Missing 'defined session' farmname arg"
			    fi
			    sessionname="$5"
			    if [ -z "$sessionname" ]; then
				err "Missing 'defined session' sessionname arg"
			    fi
			    if [ -e $( pwquery sessionetc_dir $farmname $sessionname ) ]; then
				echo "true"
			    else
				echo "false"
			    fi
			    ;;
			*)
			    err "Invalid 'defined' type ($type)"
			    ;;
		    esac
		    ;;
		*)
		    __usage "Invalid query propname ($propname)"
		    ;;
	    esac
	    ;;

        ###############################################################
        ###
        ### DEFINE
        ###
        ###############################################################
	"define")
	    type="$2"

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

	    case "$type" in
                ###############################################################
                ###
                ### define FARM
                ###
                ###############################################################
		"farm")
		    shift 2

		    farmname="$1"
		    shift

		    pwuser="$1"
		    shift

		    runresults_dir="$( canonpath $1 )"
		    shift

		    domain_user=""
		    domain_fieldnumbers=""
		    domains=()

		    function __create_domain()
		    {
			if [ ! -z "$domain_user" ]; then
			    if [ -z "$domain_fieldnumbers" ]; then
				err "No field numbers for user $domain_user"
			    fi

			    domains[${#domains[@]}]="$domain_user:$(trim "$domain_fieldnumbers")"

			    domain_user=""
			    domain_fieldnumbers=""
			fi
		    }

		    for arg in $*; do
			if is_integer "$arg"; then
			    if [ -z "$domain_user" ]; then
				err "Must provide osuser"
			    fi

			    domain_fieldnumbers="$domain_fieldnumbers $arg"
			else
			    __create_domain

			    domain_user=$arg
			fi
		    done

		    __create_domain

		    echo "--- ARGS ---"
		    echo farm name = $farmname
		    echo farm user = $pwuser
		    echo run results dir = $runresults_dir

		    for (( i=0; i < ${#domains[@]}; i++ )); do
			user=$( echo ${domains[$i]} | cut -f 1 -d ":" )
			fieldnumbers=$( echo ${domains[$i]} | cut -f 2 -d ":" )

			echo "domain $i :"
			echo "  user = $user"
			echo "  fieldnumbers = $fieldnumbers"
		    done

		    echo "------------"

		    if $prompt; then
			read -p "Is this correct? [y/n]: " reply
			if [ "$reply" != "y" ]; then
			    __usage "Aborted by user"
			fi
		    fi

		    if $prompt; then
			if $( pwquery defined farm $farmname ); then
			    read -p "Farm of this name already defined. Continue? (Session definitions will be untouched) [y/n]: " reply
			    if [ "$reply" != "y" ]; then
				__usage "Aborted by user"
			    fi
			fi
		    fi

		    users=()
		    hostnames=()

		    for (( i=0; i < ${#domains[@]}; i++ )); do
			user=$( echo ${domains[$i]} | cut -f 1 -d ":" )
			fieldnumbers=$( echo ${domains[$i]} | cut -f 2 -d ":" )

			for x in ${fieldnumbers[@]}; do
			    users[${#users[@]}]=$user
			    hostnames[${#hostnames[@]}]=$(fieldhostname_from_num $x)
			done
		    done

		    #
                    # Try pinging all the fields
                    #
		    step "Pinging all fields"

		    for hostname in ${hostnames[@]}; do
			substep "Pinging $hostname..."

			host=$( fieldhost_from_name $hostname )
			ping -c 1 $host || err Cannot ping $hostname
		    done

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
		    keys="~/.ssh/authorized_keys"

                    #
                    # Install key on domain masters
                    #
		    for (( i=0; i < ${#domains[@]}; i++ )); do
			domain_user=$( echo ${domains[$i]} | cut -f 1 -d ":" )
			domain_fieldnumbers=( $(echo ${domains[$i]} | cut -f 2 -d ":") )

			master_fieldnumber=${domain_fieldnumbers[0]}
			master_hostname=$(fieldhostname_from_num $master_fieldnumber)
			master_host=$(fieldhost_from_name $master_hostname)
			unset domain_fieldnumbers[0]

			domain_hostnames=( $(fieldhostnames_from_nums ${domain_fieldnumbers[@]}) )

			key_install="
				if [ ! -e $keys ]; then cd ; mkdir -p .ssh ; echo \"$key_data\" > $keys;
				elif ! grep -q \"$key_data\" $keys; then echo \"$key_data\" >> $keys;
				else echo Public key already installed
				fi
			"

			step "Installing key on $master_hostname"

			ssh -l $domain_user $master_host "$key_install"  || err Failed installing public key on $master_hostname

			#
			# Ensure master can ping other hosts in domain
			#
			step "Pinging all domain hosts from $master_hostname"

			ssh -l $domain_user $master_host "
				for hostname in ${domain_hostnames[@]}; do
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

			for hostname in ${domain_hostnames[@]}; do
			    substep $hostname

			    # Force presence of tty via -t in case user must enter password.
			    ssh -t -l $domain_user $master_host "
				ssh \$$hostname \"
				    if [ ! -e $keys ]; then cd ; mkdir -p .ssh ; echo \\\"$key_data\\\" > $keys;
				    elif ! grep -q \\\"$key_data\\\" $keys; then echo \\\"$key_data\\\" >> $keys;
				    else echo Public key already installed
				    fi
				\"
        		    "
			done

		    done

		    #
		    # Update known_hosts
		    #
		    step "Updating known_hosts"

		    for hostname in ${hostnames[@]}; do
			substep $hostname

			host=$( fieldhost_from_name $hostname )
			# remove any previous entry
			ssh-keygen -R $host
        	        # add entry
			ssh-keyscan -H $host >> ~/.ssh/known_hosts 
		    done

		    required_programs="screen svn"

        	    #
        	    # Verify required programs on all hosts 
        	    #
		    step "Verifying required programs on all hosts"

		    for (( i=0; i < ${#hostnames[@]}; i++ )); do
			hostname=${hostnames[$i]}
			user=${users[$i]}

			substep $hostname

			host=$( fieldhost_from_name $hostname )
			
			ssh -l $user $host "for prog in $required_programs; do if ! which \$prog; then echo Cannot locate \$prog; exit 1; fi; done" || exit 1
		    done

        	    #
        	    # Write config files
        	    #
		    step "Generating pwfarm config files"

		    farmetc_dir=$( pwquery farmetc_dir $farmname  )
		    mkdir -p $farmetc_dir || exit 1

		    echo $pwuser > $( pwquery pwuser_path $farmname )
		    echo $runresults_dir > $( pwquery runresults_dir_path $farmname )
		    for (( i=0; i < ${#domains[@]}; i++ )); do
			echo ${domains[$i]}
		    done > $( pwquery domains_path $farmname )

        	    #
        	    # Ensure session defined
        	    #
		    step "Checking session definitions"

		    if [ $( pwquery sessionnames $farmname | wc -l ) == "0"  ]; then
			substep "Creating default session"

			$0 define session $farmname default
		    else
			pwquery sessionnames $farmname
		    fi

		    ;; # end define farm

                ###############################################################
                ###
                ### define SESSION
                ###
                ###############################################################
		"session")
		    farmname="$3"
		    if [ -z "$farmname" ]; then
			err "Missing 'define session' farmname arg"
		    fi
		    if ! $( pwquery defined farm "$farmname" ); then
			err "Unknown farm name: $farmname"
		    fi
		    sessionname="$4"
		    if [ -z "$sessionname" ]; then
			err "Missing 'define session' sessionname arg"
		    fi

		    shift 4
		    fieldnumbers="$*"

		    mkdir -p $( pwquery sessionetc_dir $farmname $sessionname ) || exit 1

		    if [ -z "$fieldnumbers" ]; then
			rm -f $( pwquery sessionfieldnumbers_path $farmname $sessionname )
		    else
			echo "$fieldnumbers" > $( pwquery sessionfieldnumbers_path $farmname $sessionname )
		    fi
		    
		    ;; # end define session
		*)
		    err "Invalid 'define' type ($type)"
		    ;;
	    esac # define $type
	    ;; # define
        ###############################################################
        ###
        ### UNDEF
        ###
        ###############################################################
	"undef")
	    local type="$2"
	    case "$type" in
		"farm")
		    local farmname="$3"
		    require "$farmname" "'undef farm' farmname arg"
		    if ! $( pwquery defined farm "$farmname" ); then
			err "No farm of name '$farmname'"
		    fi
		    local farmetc_dir=$( pwquery farmetc_dir $farmname  )
		    rm -rf $farmetc_dir
		    ;;
		"session")
		    local farmname="$3"
		    require "$farmname" "'undef farm' farmname arg"
		    if ! $( pwquery defined farm "$farmname" ); then
			err "No farm of name '$farmname'"
		    fi
		    local sessionname="$4"
		    require "$sessionname" "'undef session' sessionname arg"
		    if ! $( pwquery defined session "$farmname" "$sessionname" ); then
			err "No session of name '$sessionname'"
		    fi

		    rm -rf $( pwquery sessionetc_dir "$farmname" "$sessionname" )
		    ;;
		*)
		    err "Invalid undef type ($type)"
		    ;;
	    esac 
	    ;; # undef
	*)
	    err "Invalid mode ($mode)"
	    ;;
    esac # $mode
}