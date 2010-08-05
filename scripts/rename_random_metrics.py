#! /usr/bin/env python

# rename_metrics
# Renames the random metric files (if present) from one or more Polyworld "run" directories:
#	brain
#	   Recent
#		  0 ... #
#			 metric_*random.*

import os
import sys
import getopt
from os.path import join
import glob
import datalib

DEFAULT_DIRECTORY = ''

AVR_FILE = 'AvrMetric.plt'
AVR_RANDOM_FILE = 'AvrMetricRandom.plt'
EPOCH_FILES_TO_RENAME = ['metric_swi*.plt', 'metric_*_random.plt']
EPOCH_METRICS_TO_RENAME = {'cc_a_bu':'cc_a_bu_ran_10_np',
					 	   'nsp_a_bu':'nsp_a_bu_ran_10_np',
					 	   'cpl_a_bu':'cpl_a_bu_ran_10_np',
					 	   'nm_a_wd':'nm_a_wd_ran_10_np',
					 	   'swi_nsp_a_bu':'swi_a_bu_nsp_10_np',
					 	   'swi_cpl_a_bu':'swi_a_bu_cpl_10_np'}
AVR_RANDOM_METRICS_TO_MOVE = {'cc_a_bu':'cc_a_bu_ran_10_np',
					 		  'nsp_a_bu':'nsp_a_bu_ran_10_np',
					 		  'cpl_a_bu':'cpl_a_bu_ran_10_np',
					 		  'nm_a_wd':'nm_a_wd_ran_10_np'}
AVR_METRICS_TO_RENAME = { 'swi_nsp_a_bu':'swi_a_bu_nsp_10_np',
					 	  'swi_cpl_a_bu':'swi_a_bu_cpl_10_np'}


test = False


def usage():
	print 'Usage:  rename_random_metrics.py [-t] <directory>'
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


def rename_file(file, new_file):
	global test
	if test:
		print '  renaming', file, 'to', new_file
	else:
		os.rename(file, new_file)


def rename_metrics_in_file(file, metrics_to_rename):
	global test
	tables = datalib.parse(file)
	not_renamed = []
	renamed = []
	for table in tables.values():
		if table.name in metrics_to_rename:
			renamed.append((table.name, metrics_to_rename[table.name]))
			table.name = metrics_to_rename[table.name]
		else:
			not_renamed.append(table.name)
	if renamed:
		if test:
			print 'renaming', renamed, 'in', file
		else:
			datalib.write(file, tables)
	if test and not_renamed:
		print 'not renaming', not_renamed, 'in', file

def rename_metric_file(file):
	root, name = os.path.split(file)  # get just the filename from the full path
	name = name[:-4]  # remove .plt
	if '_random' in file:
		name = name[:-7]  # remove _random
	metric = name[7:]  # get just the metric type
	if metric in EPOCH_METRICS_TO_RENAME:
		new_metric = EPOCH_METRICS_TO_RENAME[metric]
		new_file = 'metric_' + new_metric + '.plt'
		if root:
			new_file = join(root, new_file)
		rename_metrics_in_file(file, {metric:new_metric})
		rename_file(file, new_file)
	else:
		print '  unknown metric:', metric, '; not renaming', file


def move_metrics_in_file(source_file, target_file, metrics_to_move):
	global test
	tables = datalib.parse(source_file)
	not_moving = []
	moving = []
	for table in tables.values():
		if table.name in metrics_to_move:
			moving.append((table.name, metrics_to_move[table.name]))
			table.name = metrics_to_move[table.name]
		else:
			not_moving.append(table.name)
	if moving:
		if test:
			print 'moving', moving, 'from', source_file, 'to', target_file
			if len(not_moving) == 0:
				print 'unlinking', source_file
			else:
				print 'not unlinking', source_file, 'due to unmoved metrics:', not_moving
		else:
			datalib.write(target_file, tables, append=True)
			if len(not_moving) == 0:
				os.unlink(source_file)
	elif test and not_moving:
		print 'not moving', not_moving, 'from', source_file, 'and not unlinking file'


def rename_metrics_in_dir(directory):
	print 'renaming metrics in', directory
	recent_dir = join(join(directory, 'brain'), 'Recent')

	# Rename the metrics in the AvrMetric file
	file_to_alter = join(recent_dir, AVR_FILE)
	rename_metrics_in_file(file_to_alter, AVR_METRICS_TO_RENAME) # only rename the swi metrics in the AvrMetric file

	# Rename the metrics in the AvrMetricRandom file and move them into AvrMetric, eliminating the AvrMetricRandom file
	file_to_eliminate = join(recent_dir, AVR_RANDOM_FILE)
	move_to_file = join(recent_dir, AVR_FILE)
	move_metrics_in_file(file_to_eliminate, move_to_file, AVR_RANDOM_METRICS_TO_MOVE)

	# Rename the metric_* files in the epochs (and the metrics therein)
	for root, dirs, files in os.walk(recent_dir):
		break
	for dir in dirs:
		time_dir = join(recent_dir, dir)
		files_to_rename = []
		for expression in EPOCH_FILES_TO_RENAME:
			files_to_rename += glob.glob(join(time_dir, expression))
		for file in files_to_rename:
			rename_metric_file(file)


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
		rename_metrics_in_dir(directory)
	else:
		# this is a directory of run directories,
		# so recreate the run directories inside it,
		# and copy the desired pieces into each new run directory
		for run_dir in dirs:
			rename_metrics_in_dir(join(directory, run_dir))


main()
