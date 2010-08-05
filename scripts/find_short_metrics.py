#! /usr/bin/env python

# find_short_metrics
# Finds the metrics that were calculated before the missing anatomy files
# were replaced, in one or more Polyworld "run" directories

import os
import sys
import getopt
from os.path import join
from shutil import copy2
import glob
import datalib


METRIC_FILES_TO_TEST = ['metric_*.*']
BASE_METRIC_FILE = 'metric_cc_a_bu.plt'


def usage():
	print 'Usage:  find_short_metrics.py <directory>'
	print '  <directory> may be a run directory or a collection of run directories'


def parse_args():
	try:
		opts, args = getopt.getopt(sys.argv[1:], '', [] )
	except getopt.GetoptError, err:
		print str(err) # will print something like "option -a not recognized"
		usage()
		exit(2)

	if len(args) == 1:
		data_dir = args[0].rstrip('/')
	else:
		usage()
		exit(0)
	
	return data_dir


def test_run_dir(run_dir):
	print 'testing metrics', run_dir
	recent_dir = join(run_dir, 'brain', 'Recent')
	
	for root, dirs, files in os.walk(recent_dir):
		break
	for dir in dirs:
		time_dir = join(recent_dir, dir)
		base_metric_file = join(time_dir, BASE_METRIC_FILE)
		base_digest = datalib.parse_digest(base_metric_file)
		base_tables = base_digest['tables']
		base_nrows = base_tables[base_tables.keys()[0]]['nrows']
		files_to_test = []
		for expression in METRIC_FILES_TO_TEST:
			files_to_test += glob.glob(join(time_dir, expression))
		for metric_file in files_to_test:
			if metric_file != base_metric_file:
				if os.access(metric_file, os.F_OK):
					metric_digest = datalib.parse_digest(metric_file)
					metric_tables = metric_digest['tables']
					metric_nrows = metric_tables[metric_tables.keys()[0]]['nrows']
					if metric_nrows != base_nrows:
						print 'bad: %s (%d != %d)' % (metric_file, metric_nrows, base_nrows)

def main():
	data_dir = parse_args()
	
	for root, dirs, files in os.walk(data_dir):
		break
	
	if 'worldfile' not in files and len(dirs) < 1:
		print 'No worldfile and no directories, so nothing to do'
		usage()
		exit(1)
	
	if 'worldfile' in files:
		# this is a single run directory
		test_run_dir(data_dir)
	else:
		# this is a directory of run directories,
		for run_dir in dirs:
			test_run_dir(join(data_dir, run_dir))


main()
