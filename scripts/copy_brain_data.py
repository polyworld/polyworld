#! /usr/bin/python

# copy_brain_data
# Copies the following files (if present) from one or more Polyworld "run" directories:
#	BirthsDeaths.log
#	brain
#	   Recent
#		  0 ... #
#			 complexity_*.*
#			 metric_*.*
#		  Avr*.plt
#		  Complexity*.plt
#	   worldfile

import os
import sys
import getopt
from os.path import join
from shutil import copy2
import glob

RECENT_FILES_TO_COPY = ['Avr*.plt', 'Complexity*.plt']
EPOCH_FILES_TO_COPY = ['complexity_*.*', 'metric_*.*']

source_dir = ''
target_dir = ''
target_dir_specified = False
test = False
overwrite = False

def usage():
	print 'Usage:  copy_brain_data.py [-t] [-o] source_directory [target_directory]'
	print '  source_directory may be a run directory or a collection of run directories'
	print '  target_directory is optional, but if present must be the same kind of directory as source_directory'
	print '    If target_directory is not specified, a new, unique target_directory will be created'
	print '    If target_directory is specified, existing brain files will only be overwritten if -o is specified'
	print '      and BirthsDeaths.log and worldfile will not be overwritten whether -o is specified or not'
	print '  -t will print out the directories that would have been affected, but nothing will be changed'

def parse_args():
	global source_dir, target_dir, target_dir_specified, test, overwrite
	
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'to', ['test','overwrite'] )
	except getopt.GetoptError, err:
		print str(err) # will print something like "option -a not recognized"
		usage()
		exit(2)

	if len(args) == 1:
		source_dir = args[0].rstrip('/')
	elif len(args) == 2:
		source_dir = args[0].rstrip('/')
		target_dir = args[1].rstrip('/')
		target_dir_specified = True
	elif len(args) > 2:
		print "Error - too many arguments: ",
		usage()
		exit(2)
	else:
		usage()
		exit(0)
	
	for o, a in opts:
		if o in ('-t', '--test'):
			test = True
		elif o in ('-o', '--overwrite'):
			overwrite = True
		else:
			print 'Unknown option:', o
			usage()
			exit(2)

def copy(source_file, target_file):
	global test
	if test:
		print '  copying', source_file, 'to', target_file
	else:
		copy2(source_file, target_file)

def mkdir(dir):
	global test
	if test:
		print 'making directory', dir
	else:
		os.mkdir(dir)

def copy_run_dir(source, target):
	global overwrite
	print 'copying files from', source, 'to', target
	target_brain_dir = join(target, 'brain')
	target_Recent_dir = join(target_brain_dir, 'Recent')
	source_Recent_dir = join(join(source, 'brain'), 'Recent')

	if not target_dir_specified:
		copy(join(source, 'BirthsDeaths.log'), join(target, 'BirthsDeaths.log'))
		copy(join(source, 'worldfile'), join(target, 'worldfile'))
		mkdir(target_brain_dir)
		mkdir(target_Recent_dir)
	
	files_to_copy = []
	for expression in RECENT_FILES_TO_COPY:
		files_to_copy += glob.glob(join(source_Recent_dir, expression))

	for source_file in files_to_copy:
		if os.access(source_file, os.F_OK):
			target_file = join(target_Recent_dir, os.path.basename(source_file))
			if not target_dir_specified or overwrite or not os.access(target_file, os.F_OK):
				copy(source_file, target_file)
	
	for root, dirs, files in os.walk(source_Recent_dir):
		break
	for time_dir in dirs:
		source_time_dir = join(source_Recent_dir, time_dir)
		target_time_dir = join(target_Recent_dir, time_dir)
		if not target_dir_specified:
			mkdir(target_time_dir)
		
		files_to_copy = []
		for expression in EPOCH_FILES_TO_COPY:
			files_to_copy += glob.glob(join(source_time_dir, expression))
		
		for source_file in files_to_copy:
			if os.access(source_file, os.F_OK):
				target_file = join(target_time_dir, os.path.basename(source_file))
				if not target_dir_specified or overwrite or not os.access(target_file, os.F_OK):
					copy(source_file, target_file)


def main():
	global source_dir, target_dir, target_dir_specified
	parse_args()
	
	for root, dirs, files in os.walk(source_dir):
		break
	
	if 'worldfile' not in files and len(dirs) < 1:
		print 'No worldfile and no directories, so nothing to do'
		usage()
		exit(1)
	
	# create a parallel, unique directory, if target_dir was not specified
	if not target_dir_specified:
		suffix = 1
		while os.access(source_dir+'_'+str(suffix), os.F_OK):
			suffix += 1
		target_dir = source_dir+'_'+str(suffix)
		mkdir(target_dir)

	if 'worldfile' in files:
		# this is a single run directory,
		# so just copy the desired pieces into the new directory
		if target_dir_specified and not os.access(join(target_dir, 'worldfile'), os.F_OK):
			print 'Error: attempted to copy a run directory to a non-run directory'
			usage()
			exit(3)
		else:
			copy_run_dir(source_dir, target_dir)
	else:
		# this is a directory of run directories,
		# so recreate the run directories inside it,
		# and copy the desired pieces into each new run directory
		if target_dir_specified and os.access(join(target_dir, 'worldfile'), os.F_OK):
			print 'Error: attempted to copy a non-run directory to a run directory'
			usage()
			exit(4)
		else:
			for run_dir in dirs:
				target_run_dir = join(target_dir, run_dir)
				if not target_dir_specified:
					mkdir(target_run_dir)
				copy_run_dir(join(source_dir, run_dir), target_run_dir)


main()
