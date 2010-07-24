export PWFARM_COMMAND='sleep 30'
export PWFARM_COMPLETION_FILE='test_completion'

testcase="ssh"

function canonpath()
{
    python -c "import os.path; print os.path.realpath('$1')"
}

function canondirname()
{
    dirname `canonpath "$1"`
}

case "$testcase" in
    "ssh")
	./pwfarm_install_ssh_keys.sh farmer `canondirname $0`/pwhostnumbers larryy
	;;
    "dispatcher")
	./pwfarm_dispatcher.sh clear ./pwhostnumbers
	./pwfarm_dispatcher.sh dispatch ./pwhostnumbers sean payload.zip './myscript.sh'
	;;
    *)
	echo invalid testcase
	;;
esac

if false ; then
    rm ./test_completion
    ./pwfarm_field.sh . ./test_completion 'sleep 30'
    exit
fi