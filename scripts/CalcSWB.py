#!/usr/bin/env python

# CalcSWB.py
# Calculates Small World Bias, a driven vs. passive version of
# Humphrey's actual vs. random Small World Index

import os,sys, getopt
import datalib
import common_metric

DEFAULT_DIRECTORY = '' # '/pwd/driven_vs_passive_b2'
DEFAULT_VALUES = ['swb_a_bu_npl', 'swb_p_bu_npl', 'swb_a_wd_npl', 'swb_p_wd_npl', \
				  'swb_a_bu_cpl', 'swb_p_bu_cpl', 'swb_a_wd_cpl', 'swb_p_wd_cpl', \
				  'swb_a_bu_cl' , 'swb_p_bu_cl' , 'swb_a_wd_cl' , 'swb_p_wd_cl'  ]

def usage():
	print 'Usage:  CalcSWB.py -v swb_<neuron-set>_<graph-type>_<length-type>,... <directory>'
	print '  Multiple swb_* metrics may be specified at once'
	print '  <directory> may be a driven run directory'
	print '              or a collection of driven and passive run directories'
	print '  Only driven directories for which a matching passive directory'
	print '  can be found will be used.'

def get_matching_passive_directory(driven_dir):
	driven_start = driven_dir.rfind('driven')
	if driven_start == -1:
		print 'The specified directory has a worldfile but is not a "driven" directory', input_dir
		exit(1)
	passive_dir = driven_dir[:driven_start] + 'passive' + driven_dir[driven_start+6:]
	if not os.access(passive_dir, os.F_OK):
		print 'Cannot access matching passive directory', passive_dir
		passive_dir = None
	
	return passive_dir


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
			driven_dir = input_dir  # Only one run
			passive_dir = get_matching_passive_directory(input_dir)  # Only one matching run
			if not passive_dir:
				exit(1)  # error message already issued in get_matching...
			run_dirs = [(driven_dir,passive_dir)]
		else:
			print 'Error! Specified run directory is not a driven run directory', input_dir
			exit(1)
	else:  # Multiple runs
		run_dirs = []
		for dir in dirs:
			if is_driven_run(dir):
				driven_dir = os.path.join(root,dir)
				passive_dir = get_matching_passive_directory(driven_dir)
				if passive_dir:
					run_dirs.append((driven_dir,passive_dir))
		if not run_dirs:
			print 'Error! No matching driven/passive runs found'
			exit(2)
				
	return run_dirs


def parse_args():	
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'v:', ['value'])
	except getopt.GetoptError, err:
		print str(err)
		usage()
		exit(2)
		
	if len(args) < 1:
		usage()
		if DEFAULT_DIRECTORY and DEFAULT_VALUES:
			print 'using defaults: -v', ','.join(value for value in DEFAULT_VALUES), DEFAULT_DIRECTORY
			input_dir = DEFAULT_DIRECTORY
		else:
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


def retrieve_timesteps(dir):
	avr_file = get_avr_file(dir)
	tables = datalib.parse(avr_file)
	metric_table = tables[tables.keys()[0]]  # all tables should have Timesteps and the same number of them
	timesteps = metric_table.getColumn('Timestep').data
	
	return timesteps


def get_cc_len(dir, neuron_set, graph_type, length_type):
	avr_file = get_avr_file(dir)
	tables = datalib.parse(avr_file)
	cc_table = tables['cc_'+neuron_set+'_'+graph_type]
	len_table = tables[length_type+'_'+neuron_set+'_'+graph_type]
	cc = cc_table.getColumn('mean').data
	len = len_table.getColumn('mean').data
	
	return cc, len
		
	
def retrieve_data(run_dir, neuron_set, graph_type, length_type):
	cc_d, len_d = get_cc_len(run_dir[0], neuron_set, graph_type, length_type)
	cc_p, len_p = get_cc_len(run_dir[1], neuron_set, graph_type, length_type)
		
	return cc_d, len_d, cc_p, len_p


def plot_dirs(ax, run_dirs, x_data_type, y_data_type, sim_type):
	for run_dir in run_dirs:
		x, y, time = retrieve_data(run_dir, x_data_type, y_data_type)
		plot(ax, x, y, time, sim_type)


def __list_division(a,b):
	assert(len(a) == len(b))
	c = []
	for i in range(len(a)):
		if b[i] == 0:
			c.append(0.0)
		else:
			c.append(float(a[i])/float(b[i]))
	return c


def avr_tables(metrics, data, timesteps):	
	colnames = ['Timestep', 'mean']
	coltypes = ['int', 'float']

	tables = {}

	for metric in metrics:
		table = datalib.Table(metric, colnames, coltypes)
		tables[metric] = table
		assert(len(data[metric]) == len(timesteps))
		for i in range(len(data[metric])):
			row = table.createRow()
			row.set('Timestep', timesteps[i])
			row.set('mean', data[metric][i])

	return tables


def write_data(dir, tables):
	avr_file = get_avr_file(dir)
# 	print 'dir =', dir
# 	print 'tables =', tables
	datalib.write(avr_file, tables, append=True)


def main():
	input_dir, metrics = parse_args()

	run_dirs = get_run_dirs(input_dir)
	
	for run_dir in run_dirs:
		print 'Processing', run_dir[0]
		timesteps = retrieve_timesteps(run_dir[0])
		data = {}
		for metric in metrics:
			parts = metric.split('_')
			neuron_set = parts[1]
			graph_type = parts[2]
			length_type = parts[3]
			cc_d, len_d, cc_p, len_p = retrieve_data(run_dir, neuron_set, graph_type, length_type)
			g = __list_division(cc_d, cc_p)
			l = __list_division(len_d, len_p)
			s = __list_division(g, l)
			data[metric] = s
		tables = avr_tables(metrics, data, timesteps)
		write_data(run_dir[0], tables)
	
	
main()	
	
