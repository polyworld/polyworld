#!/usr/bin/env python

# get_range.py
# Gets and prints the min, max, and mean of the specified values
# over the specified run directories

import os,sys, getopt
import datalib
import common_metric

DEFAULT_DIRECTORY = '' # '/pwd/driven_vs_passive_b2'
DEFAULT_VALUES = ['nc_a_wd', 'ec_a_wd']

def usage():
	print 'Usage:  get_range.py -v <metric>_<neuron-set>_<graph-type>,... <directory>'
	print '  Multiple metrics may be specified at once'
	print '  <directory> may be a driven run directory'
	print '              or a collection of driven and passive run directories'
	print '  Driven and Passive results will be specified separately and together'


def is_driven_run(dir):
	run_start = dir.rfind('run')
	driven_start = dir[run_start+3:].rfind('driven')
	return (driven_start != -1)
	

def get_run_dirs(input_dir):
	if not os.access(input_dir, os.F_OK):
		print 'Cannot access', input_dir
		exit(1)
	for root, dirs, files in os.walk(input_dir):
		break
	if 'worldfile' in files:
		if is_driven_run(input_dir):
			driven_dirs = [input_dir]
			passive_dirs = []
		else:
			driven_dirs = []
			passive_dirs = [input_dir]
	else:  # Multiple runs
		driven_dirs = []
		passive_dirs = []
		for dir in dirs:
			path = os.path.join(root,dir)
			if is_driven_run(dir):
				driven_dirs.append(path)
			else:
				passive_dirs.append(path)
				
	return driven_dirs, passive_dirs


def parse_args():	
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'v:', ['value'])
	except getopt.GetoptError, err:
		print str(err)
		usage()
		exit(2)
		
	if len(args) < 1:
		if DEFAULT_DIRECTORY and DEFAULT_VALUES:
			print '-------------------------------------------------------------'
			print 'using defaults: -v', ','.join(value for value in DEFAULT_VALUES), DEFAULT_DIRECTORY
			input_dir = DEFAULT_DIRECTORY
		else:
			usage()
			exit(0)
	else:
		input_dir = args[0]
		
	values = DEFAULT_VALUES
	for o, a in opts:
		if o in ('-v','--value'):
			values = a.split(',')
		else:
			print 'Unknown options:', o
			usage()
			exit(2)
	
	input_dir = input_dir.rstrip('/')
						
	return input_dir, values


def get_avr_file(dir):
	avr_file = os.path.join(dir, 'brain/Recent', common_metric.FILENAME_AVR)
	if not os.path.exists(avr_file):
		print 'Error! Avr file missing:', avr_file
		exit(2)

	return avr_file


def print_range(dirs, type, metrics):
	if not dirs or not metrics:
		return
	
	minv = {}
	maxv = {}
	mean = {}
	
	for metric in metrics:
		minv[metric] = 1000000.0
		maxv[metric] = 0.0
		mean[metric] = 0.0
	
	for dir in dirs:
		data = {}
		avr_file = get_avr_file(dir)
		tables = datalib.parse(avr_file)
		for metric in metrics:
			metric_table = tables[metric]
			metric_min = metric_table.getColumn('min').data
			metric_max = metric_table.getColumn('max').data
			metric_mean = metric_table.getColumn('mean').data
			minv[metric] = min(minv[metric], float(min(metric_min)))
			maxv[metric] = max(maxv[metric], max(metric_max))
			mean[metric] += sum(metric_mean) / len(metric_mean)
	
	for metric in metrics:
		mean[metric] /= len(dirs)

	print '%s:' % (type)
	print '  metric       min     max     mean'
	for metric in metrics:
		print '  %s:  %6g\t%6g\t%6g' % (metric, minv[metric], maxv[metric], mean[metric])


def main():
	input_dir, metrics = parse_args()

	driven_dirs, passive_dirs = get_run_dirs(input_dir)
	
	print_range(driven_dirs, 'Driven', metrics)
	print_range(passive_dirs, 'Passive', metrics)
	if driven_dirs and passive_dirs:
		print_range(driven_dirs + passive_dirs, 'Combination', metrics)
	
	
main()	
	
