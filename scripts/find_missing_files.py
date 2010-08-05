#!/usr/bin/env python

# Finds any missing anatomy files (agents for which a function file exists, but not an anatomy file)
# This is more complicated that it needs to be because it is based directly on CalcMetricsOverRecent

#### Don't modify anything beneath here unless you know what you're doing
import getopt, glob, string, sys, os
import common_functions
from common_functions import err, warn, list_difference, list_intersection

####################################################################################
###
### Global variables
###
####################################################################################
DefaultRecent = 'Recent'

####################################################################################
###
### main()
###
####################################################################################
def main():
	recent_type, arg_paths = parse_args(sys.argv[1:])
	
	recent_subpath = os.path.join('brain', recent_type)
	try:
		run_paths = common_functions.find_run_paths(arg_paths,
								recent_subpath)
	except common_functions.InvalidDirError, e:
		show_usage(str(e))

	recent_dirs = map(lambda x: os.path.join(x, recent_subpath),
			  run_paths)

	for recent_dir in recent_dirs:
		analyze_recent_dir(recent_dir)

	print "Done!"
	return 0


####################################################################################
###
### FUNCTION parse_args()
###
####################################################################################
def parse_args(argv):
	if len(argv) == 0:
		show_usage()

	recent_type = DefaultRecent

	short = 'd'
	long = []

	try:
		opts, args = getopt.getopt(argv, short, long)
	except getopt.GetoptError, e:
		show_usage(str(e))

	for opt, value in opts:
		opt = opt.strip('-')

		if opt == 'd':
			try:
				recent_type = common_functions.expand_abbreviations( value,
											 common_functions.RECENT_TYPES,
											 case_sensitive = False )
			except common_functions.IllegalAbbreviationError, x:
				err(str(x))
		else:
			assert(False)

	if len(args) == 0:
		show_usage('Must specify run/run-parent directory.')
	
	paths = list(args)

	return recent_type, paths

####################################################################################
###
### analyze_recent_dir()
###
####################################################################################
def analyze_recent_dir(recent_dir):
	
	print "- recent directory='%s'" %(recent_dir)

	#-----------------------------------------------------------------------------------
	#--
	#-- Find epoch/timestep directories
	#--
	#-----------------------------------------------------------------------------------
	timesteps = []
	# list all of the timesteps, make sure they are all integers (and directories), then sort them by number.
	for potential_timestep in os.listdir( recent_dir ):
		if not potential_timestep.isdigit():
			continue	# if timestep isn't a digit, skip it.
		if not os.path.isdir( os.path.join(recent_dir, potential_timestep) ):
			continue	# if the timestep isn't a directory, skip it.
		
		timesteps.append( int(potential_timestep) )						# add timestep to our list
	
	if len(timesteps) == 0:
		err('No epochs found. Not a valid recent directory.')

	timesteps.sort()									# sort the timesteps, lowest numbers come first.
	
	print "Processing:",
	for t in timesteps:
		analyze_epoch_dir(recent_dir, str(t))
	
	
####################################################################################
###
### FUNCTION analyze_epoch_dir()
###
####################################################################################
def analyze_epoch_dir(recent_dir, t):
	timestep_directory = os.path.join(recent_dir, t)
	print '%s...' % (timestep_directory)

	# Look for all corresponding brainAnatomy files in anatomy dir
	brainFunction_files = glob.glob(os.path.join(timestep_directory, "brainFunction*.txt"))
	#Extract critter number from brainFunction files
	number_list = map(lambda x: int(os.path.basename(x)[14:-4]), brainFunction_files)
			
	anatomy_directory = os.path.join(timestep_directory, "..", "..", "anatomy")
	
	number_list.sort()
	brainAnatomy_files = []
	for id in number_list:
		anatomy_file = os.path.join(anatomy_directory, 'brainAnatomy_' + str(id) + '_death.txt')
		if not os.access(anatomy_file, os.F_OK):
			print '  missing:', anatomy_file
										   
			
####################################################################################
###
### FUNCTION show_usage()
###
####################################################################################
def show_usage(msg = None):
################################################################################
	print"""\
USAGE

  %s <directory>

DESCRIPTION

     Finds missing anatomy files.

     <directory> can specify either a run directory or a parent of one or more
  run directories.

OPTIONS

     (none)""" % (sys.argv[0])

	if msg:
		print "--------------------------------------------------------------------------------"
		print
		print 'Error!', msg
	sys.exit(1)

####################################################################################
###
### Primary Code Path
###
####################################################################################

exit_value = main()

sys.exit(exit_value)
