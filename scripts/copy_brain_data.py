#! /usr/bin/env python

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
import subprocess
import datalib

DEFAULT_RECENT_FILES_TO_COPY = ['Avr*.plt', 'Complexity*.plt']
DEFAULT_EPOCH_FILES_TO_COPY = ['complexity_*.*', 'metric_*.*']

AVR_METRIC_FILENAME = 'AvrMetric.plt'

source_dir = ''
target_dir = ''
target_dir_specified = False
test = False
overwrite = False
avr_only = False
gzip = False
recent_files_to_copy = DEFAULT_RECENT_FILES_TO_COPY[:]
epoch_files_to_copy = DEFAULT_EPOCH_FILES_TO_COPY[:]
metrics_to_copy = []

def usage():
	print 'Usage:  copy_brain_data.py [-t] [-a] [-o] [-g] [-e epoch_file_pattern,...] [-r recent_file_pattern,...] [-m metric,...] source_directory [target_directory]'
	print '  source_directory may be a run directory or a collection of run directories'
	print '  target_directory is optional, but if present must be the same kind of directory as source_directory'
	print '    If target_directory is not specified, a new, unique target_directory will be created'
	print '    If target_directory is specified, existing brain files will only be overwritten if -o is specified'
	print '      and BirthsDeaths.log and worldfile will not be overwritten whether -o is specified or not'
	print '  -a will copy only the Avr files (no epoch files)'
	print '  -g will gzip any uncompressed .plt files as they are being copied'
	print '     WARNING: -g is not really useful until datalib.py and datalib.cp have been updated to use AbstractFiles'
	print '  -e comma-separated epoch files to copy (standard glob symbols allowed)'
	print '     (default: %s)' % ','.join(DEFAULT_EPOCH_FILES_TO_COPY)
	print '  -r comma-separated recent files to copy (standard glob symbols allowed)'
	print '     (default: %s)' % ','.join(DEFAULT_RECENT_FILES_TO_COPY)
	print '  -m comma-separated metrics to copy from one AvrMetric.plt file to the other'
	print '     IF -m IS SPECIFIED, NO RECENT / Avr FILES WILL BE COPIED, ONLY THE METRICS THEREIN'
	print '     AND THE ONLY EPOCH FILES COPIED WILL BE "metric_<metric>.*" FOR THE METRICS SPECIFIED WITH -m'
	print '     (default: All or none, depending on target_directory and -o, as the entire file is copied by default)'
	print '  -t will print out the directories and files that would have been affected, but nothing will be changed'

def parse_args():
	global	source_dir, target_dir, target_dir_specified, test, overwrite, avr_only, \
			gzip, recent_files_to_copy, epoch_files_to_copy, metrics_to_copy
	
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'toage:r:m:', ['test','overwrite','avr','gzip','epoch_files','recent_files','metrics'] )
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
	
	a = False
	e = False
	r = False
	m = False
	for opt, arg in opts:
		if opt in ('-t', '--test'):
			test = True
		elif opt in ('-o', '--overwrite'):
			overwrite = True
		elif opt in ('-a', '--avr_only'):
			avr_only = True
			a = True
		elif opt in ('-g', '--gzip'):
			gzip = True
		elif opt in ('-e', '--epoch_files'):
			epoch_files_to_copy = arg.split(',')
			e = True
		elif opt in ('-r', '--recent_files'):
			recent_files_to_copy = arg.split(',')
			r = True
		elif opt in ('-m', '--metrics'):
			metrics_to_copy = arg.split(',')
			epoch_files_to_copy = []
			for metric in metrics_to_copy:
				epoch_files_to_copy.append('metric_' + metric + '.plt')
			m = True
		else:
			print 'Unknown option:', opt
			usage()
			exit(2)
	
	if m and (e or r):
		print 'Cannot specify -m with any of the -e, or -r options'
		usage()
		exit(2)
	
	if m and a:
		print 'Note: Because -a was specified, only the metrics in the %s file will be copied'
		print '      None of the <epoch>/metric_*.plt files will be copied'

def copy(source_file, target_file):
	global test, gzip
	if test:
		print '  copying', source_file, 'to', target_file
		if gzip and target_file[-4:] == '.plt':
			print '  gzipping', target_file, 'to', target_file+'.gz'
	else:
		copy2(source_file, target_file)
		if gzip and target_file[-4:] == '.plt':
			subprocess.call(['gzip', target_file])

def mkdir(dir):
	global test
	if test:
		print 'making directory', dir
	else:
		os.mkdir(dir)

def copy_metrics(source_file, target_file):
	global test, overwrite, metrics_to_copy
	if test:
		print '  copying metrics (%s) from' % (','.join(metrics_to_copy)), source_file, 'to', target_file
	else:
		tables_to_copy = {}
		source_tables = datalib.parse(source_file)
		for metric in metrics_to_copy:
			tables_to_copy[metric] = source_tables[metric]
		datalib.write(target_file, tables_to_copy, append=True, replace=overwrite)

def copy_run_dir(source, target):
	global overwrite, metrics_to_copy, recent_files_to_copy, epoch_files_to_copy
	print 'copying files from', source, 'to', target
	target_brain_dir = join(target, 'brain')
	target_Recent_dir = join(target_brain_dir, 'Recent')
	source_Recent_dir = join(join(source, 'brain'), 'Recent')

	if not target_dir_specified:
		copy(join(source, 'BirthsDeaths.log'), join(target, 'BirthsDeaths.log'))
		if os.access(join(source, 'worldfile'), os.F_OK):
			copy(join(source, 'worldfile'), join(target, 'worldfile'))
		if os.access(join(source, 'original.wf'), os.F_OK):
			copy(join(source, 'original.wf'), join(target, 'original.wf'))
			copy(join(source, 'normalized.wf'), join(target, 'normalized.wf'))
			copy(join(source, 'reduced.wf'), join(target, 'reduced.wf'))
			copy(join(source, 'original.wfs'), join(target, 'original.wfs'))
			copy(join(source, 'normalized.wfs'), join(target, 'normalized.wfs'))
		mkdir(target_brain_dir)
		mkdir(target_Recent_dir)
	
	if metrics_to_copy:
		source_file = join(source_Recent_dir, AVR_METRIC_FILENAME)
		if not os.access(source_file, os.F_OK):
			print 'Unable to access source file:', source_file
			exit(3)
		target_file = join(target_Recent_dir, AVR_METRIC_FILENAME)
		copy_metrics(source_file, target_file)
	else:
		files_to_copy = []
		for expression in recent_files_to_copy:
			files_to_copy += glob.glob(join(source_Recent_dir, expression))
	
		for source_file in files_to_copy:
			if os.access(source_file, os.F_OK):
				target_file = join(target_Recent_dir, os.path.basename(source_file))
				if not target_dir_specified or overwrite or not os.access(target_file, os.F_OK):
					copy(source_file, target_file)
		
	if not avr_only:
		for root, dirs, files in os.walk(source_Recent_dir):
			break
		for time_dir in dirs:
			source_time_dir = join(source_Recent_dir, time_dir)
			target_time_dir = join(target_Recent_dir, time_dir)
			if not target_dir_specified and epoch_files_to_copy:
				mkdir(target_time_dir)
			
			files_to_copy = []
			for expression in epoch_files_to_copy:
				files_to_copy += glob.glob(join(source_time_dir, expression))
			for source_file in files_to_copy:
				if os.access(source_file, os.F_OK):
					target_file = join(target_time_dir, os.path.basename(source_file))
					if target_dir_specified and os.access(target_file, os.F_OK) and not overwrite:
						print 'Target file already exists and -o was not specified:', target_file
					else:
						copy(source_file, target_file)


def main():
	global source_dir, target_dir, target_dir_specified

	parse_args()
	
	for root, dirs, files in os.walk(source_dir):
		break
	
	worldfile = 'worldfile' in files or 'original.wf' in files
	
	if not worldfile and len(dirs) < 1:
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

	if worldfile:
		# this is a single run directory,
		# so just copy the desired pieces into the new directory
		if target_dir_specified and not os.access(join(target_dir, 'worldfile'), os.F_OK) \
								and not os.access(join(target_dir, 'original.wf'), os.F_OK):
			print 'Error: attempted to copy a run directory to a non-run directory'
			usage()
			exit(3)
		else:
			copy_run_dir(source_dir, target_dir)
	else:
		# this is a directory of run directories,
		# so recreate the run directories inside it,
		# and copy the desired pieces into each new run directory
		if target_dir_specified and (os.access(join(target_dir, 'worldfile'), os.F_OK) or \
									 os.access(join(target_dir, 'original.wf'), os.F_OK)):
			print 'Error: attempted to copy a non-run directory to a run directory'
			usage()
			exit(4)
		else:
			for run_dir in dirs:
				source_run_dir = join(source_dir, run_dir)
				if not os.access(join(source_run_dir, 'worldfile'), os.F_OK) and \
				   not os.access(join(source_run_dir, 'original.wf'), os.F_OK):
					print 'not copying non-run directory:', source_run_dir
					continue
				target_run_dir = join(target_dir, run_dir)
				if not target_dir_specified:
					mkdir(target_run_dir)
				copy_run_dir(source_run_dir, target_run_dir)


main()
