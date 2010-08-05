#! /usr/bin/env python

# fix_anatomy_files
# Copies (and appropriately renames) anatomy files that are missing from the
# anatomy directory, but may still be found in the bestRecent epoch directories

import os
import sys
import getopt
from os.path import join
import glob
import datalib

DEFAULT_DIRECTORY = ''


test = False


def usage():
	print 'Usage:  fix_missing_anatomy_files.py [-t] <directory>'
	print '  <directory> may be a run directory or a collection of run directories'
	print '  -t will print out the files that would have been affected, but nothing will be changed'


def parse_args():
	global test
	
	try:
		opts, args = getopt.getopt(sys.argv[1:], 't', ['test'] )
	except getopt.GetoptError, err:
		print str(err) # will print something like "option -a not recognized"
		usage()
		exit(2)

	if len(args) == 1:
		directory = args[0].rstrip('/')
	elif len(args) > 1:
		print "Error: too many arguments"
		usage()
		exit(2)
	elif DEFAULT_DIRECTORY:
		directory = DEFAULT_DIRECTORY
	else:
		usage()
		exit(0)
	
	for o, a in opts:
		if o in ('-t', '--test'):
			test = True
		else:
			print 'Unknown option:', o
			usage()
			exit(2)
	
	return directory


def link(source_file, target_file):
	global test
	if test:
		print '  linking', source_file, 'to', target_file
	else:
		os.link(source_file, target_file)


def fix_anatomy_files(directory):
	print 'fixing anatomy files in', directory
	brain_dir = join(directory, 'brain')
	anatomy_dir = join(brain_dir, 'anatomy')
	bestRecent_dir = join(brain_dir, 'bestRecent')

	for root, dirs, files in os.walk(bestRecent_dir):
		break
	
	num_fixed = 0
	for dir in dirs:
		epoch_dir = join(bestRecent_dir, dir)
		best_anatomy_files = glob.glob(join(epoch_dir, '*brainAnatomy*'))
		for file in best_anatomy_files:
			anatomy_file = join(anatomy_dir, os.path.basename(file)[2:])
			if not os.path.exists(anatomy_file):
				num_fixed += 1
				link(file, anatomy_file)
	
	print '   ', num_fixed, 'fixed'

def main():
	directory = parse_args()
	
	for root, dirs, files in os.walk(directory):
		break
	
	if 'worldfile' not in files and len(dirs) < 1:
		print 'No worldfile and no directories, so nothing to do'
		usage()
		exit(1)
	
	if 'worldfile' in files:
		# this is a single run directory,
		# so just rename the files therein
		fix_anatomy_files(directory)
	else:
		# this is a directory of run directories,
		# so recreate the run directories inside it,
		# and copy the desired pieces into each new run directory
		count = 0
		for run_dir in dirs:
			fix_anatomy_files(join(directory, run_dir))


main()
