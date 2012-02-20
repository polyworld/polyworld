import os
from subprocess import Popen, PIPE

####################################################################################
###
### FUNCTION is_valid_env()
###
####################################################################################
def is_valid_env():
	return bash( 'validate_farm_env' ).exitval == 0

####################################################################################
###
### FUNCTION find_runs_local()
###
####################################################################################
def find_runs_local( runid ):
	return bash( 'find_runs_local "*" ' + runid ).stdout_lines()

####################################################################################
###
### FUNCTION status()
###
####################################################################################
def status( msg ):
	if os.getenv( 'PWFARM_STATUS' ):
		bash( 'PWFARM_STATUS "%s" ' % msg )


FARMSRC = os.path.dirname( __file__ )
RUNUTIL = os.path.join( FARMSRC, '__pwfarm_runutil.sh' )

class BashResult:
	def __init__( self, exitval, stdout, stderr ):
		self.exitval = exitval
		self.stdout = stdout
		self.stderr = stderr

	def stdout_lines( self ):
		return self.stdout.strip().split( '\n' )

def bash(cmd, import_runutil = True):
	if import_runutil:
		cmd = 'source %s && %s' % ( RUNUTIL, cmd )

	cmdv = ['/bin/bash', '-c', cmd]

	proc = Popen(cmdv, stdout=PIPE, stderr=PIPE)
	
	stdout, stderr = proc.communicate()

	return BashResult( proc.returncode, stdout, stderr )
