#!/usr/bin/python

import pylab
import os,sys, getopt
import datalib
from matplotlib.collections import LineCollection
from common_complexity import COMPLEXITY_TYPES, COMPLEXITY_NAMES, COMPLEXITY_FILENAME_AVR
from common_metric import METRIC_TYPES, METRIC_NAMES, METRIC_FILENAME_AVR

GROUPS = ['driven', 'passive', 'both']

DEFAULT_DIRECTORY = '/Volumes/halo/pwd/driven_vs_passive_b2'
DEFAULT_GROUP = GROUPS[2]
DEFAULT_X_DATA_TYPE = 'P'
DEFAULT_Y_DATA_TYPE = 'SC'

COLOR = {'driven':(0.5,1.0,0.5), 'passive':(0.5,0.5,1.0)}

end_points = []

def usage():
	print 'Usage:  plot_avr.py -x <data_type> -y <data_type> [-d|p|b] run_directory'
	print '  run_directory may be a run directory or a collection of run directories'
	print '  the x-axis will correspond to the -x <data_type>'
	print '  the y-axis will correspond to the -y <data_type>'
	print '  if -d or none is specified, only driven runs will be selected when multiple run directories exist'
	print '  if -p is specified, only passive runs will be selected when multiple run directories exist'
	print '  if -b is specified, both driven and passive runs will be selected when multiple run directories exist'
	print '  for current data types see common_complexity.py, common_metrics.py, common_motion.py'
	print '  e.g., complexities:',
	for type in COMPLEXITY_TYPES:
		print type,
	print '; metrics:',
	for type in METRIC_TYPES:
		print type,
	print
	

def get_filename(data_type):
	if data_type in COMPLEXITY_TYPES:
		return COMPLEXITY_FILENAME_AVR
	else:
		return METRIC_FILENAME_AVR


def get_run_dirs(input_dir, group):
	if not os.access(input_dir, os.F_OK):
		print 'Cannot access', input_dir
	for root, dirs, files in os.walk(input_dir):
		break
	if 'worldfile' in files:
		run_dirs = [input_dir]  # Only one run
	else:  # Multiple runs
		N = len(dirs)
		if group == 'driven':
			run_dir_names = dirs[0:(N/2)]  # all driven runs
		elif group == 'passive':
			run_dir_names = dirs[(N/2):N]  # all passive runs
	
		run_dirs = []
		for run_dir_name in run_dir_names:
			run_dirs.append(os.path.join(root,run_dir_name))
				
	return run_dirs


def parse_args():	
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'dpbx:y:', ['driven', 'passive', 'both', 'x_data_type', 'y_data_type'])
	except getopt.GetoptError, err:
		print str(err)
		usage()
		exit(2)
		
	if len(args) < 1:
		usage()
		print 'using defaults:', '--'+DEFAULT_GROUP, '-x', DEFAULT_X_DATA_TYPE, '-y', DEFAULT_Y_DATA_TYPE, DEFAULT_DIRECTORY
		return DEFAULT_DIRECTORY, DEFAULT_GROUP, DEFAULT_X_DATA_TYPE, DEFAULT_Y_DATA_TYPE
		
	input_dir = args[0].rstrip('/')
	
	group = DEFAULT_GROUP
	x_data_type = DEFAULT_X_DATA_TYPE
	y_data_type = DEFAULT_Y_DATA_TYPE
	for o, a in opts:
		if o in ('-d','--driven'):
			group = 'driven'
		elif o in ('-p', '--passive'):
			group = 'passive'
		elif o in ('-b', '--both'):
			group = 'both'
		elif o in ('-x', '--x_data_type'):
			x_data_type = a
		elif o in ('-y', '--y_data_type'):
			y_data_type = a			
		else:
			print 'Unknown options:', o
			usage()
			exit(2)
	
	if x_data_type not in COMPLEXITY_TYPES and x_data_type not in METRIC_TYPES:
		print 'unknown x_data_type:', x_data_type
		usage()
		exit(2)

	if y_data_type not in COMPLEXITY_TYPES and y_data_type not in METRIC_TYPES:
		print 'unknown y_data_type:', y_data_type
		usage()
		exit(2)
	
	if group not in GROUP_TYPES:
		print 'unknown group type:', group
					
	return input_dir, group, x_data_type, y_data_type
		
	
def retrieve_data(run_dir, x_data_type, y_data_type):
	recent_dir = os.path.join(run_dir, 'brain/Recent')
	x_filename = get_filename(x_data_type)
	x_path = os.path.join(recent_dir, x_filename)
	y_filename = get_filename(y_data_type)
	y_path = os.path.join(recent_dir, y_filename)
	if not (os.path.exists(x_path) and os.path.exists(y_path)):
		print 'Error! Needed Avr files are missing'
		exit(2)
		
	x_table = datalib.parse(x_path)[x_data_type]
	y_table = datalib.parse(y_path)[y_data_type]
	x_data = x_table.getColumn('mean').data
	y_data = y_table.getColumn('mean').data
	t_data = x_table.getColumn('Timestep').data # could come from either x or y
		
	return x_data, y_data, t_data


def plot_init(title, x_label, y_label, bounds, size):
	fig = pylab.figure(figsize=size)
	ax = fig.add_subplot(111)
# 	ax.set_xlim(bounds[0], bounds[1])
# 	ax.set_ylim(bounds[2], bounds[3])
	pylab.title(title)
	pylab.xlabel(x_label)
	pylab.ylabel(y_label)
	return ax
	

def plot(ax, x, y, time, sim_type):
	assert(len(x) == len(y) == len(time))

	l = len(time)
	time_to_grayscale = 0.8 / time[l-1]
	colors = []
	for i in range(len(time)-1):
		c = 0.8 - (time[i] * time_to_grayscale)**2.0
		if sim_type == 'driven':
			color = (c, 1.0, c, 0.8)
		else:
			color = (c, c, 1.0, 1.0)
		colors.append(color)
	
	points = zip(x,y)
	segments = zip(points[:-1], points[1:])
	lc = LineCollection(segments, colors=colors)
	lc.set_alpha(1.0)
	lc.set_linewidth(1.0)
	lc.set_antialiased(True)
	ax.add_collection(lc)
	end_points.append((x[l-1], y[l-1], sim_type))


def plot_dirs(ax, run_dirs, x_data_type, y_data_type, sim_type):
	for run_dir in run_dirs:
		x, y, time = retrieve_data(run_dir, x_data_type, y_data_type)
		plot(ax, x, y, time, sim_type)


def plot_finish(ax, bounds):
	for point in end_points:
		pylab.plot([point[0]], [point[1]], 'o', color=COLOR[point[2]])
# 	ax.set_xlim(bounds[0], bounds[1])
# 	ax.set_ylim(bounds[2], bounds[3])


def main():
	input_dir, group, x_data_type, y_data_type = parse_args()
	if x_data_type in COMPLEXITY_TYPES:
		x_label = 'Complexity (' + COMPLEXITY_NAMES[x_data_type] + ')'
	else:
		x_label = METRIC_NAMES[x_data_type]
	if y_data_type in COMPLEXITY_TYPES:
		y_label = 'Complexity (' + COMPLEXITY_NAMES[y_data_type] + ')'
	else:
		y_label = METRIC_NAMES[y_data_type]
	title = y_label + ' vs. ' + x_label + ' - Temporal Traces'
	bounds = (0.1, 0.5, 0.0, 3.0)  # (x_min, x_max, y_min, y_max)
	size = (11.0, 8.5)  # (width, height)
	ax = plot_init(title, x_label, y_label, bounds, size)
	
	if group == 'passive' or group == 'both':
		plot_dirs(ax, get_run_dirs(input_dir, 'passive'), x_data_type, y_data_type, 'passive')
	if group == 'driven' or group == 'both':
		plot_dirs(ax, get_run_dirs(input_dir, 'driven'), x_data_type, y_data_type, 'driven')

	plot_finish(ax, bounds)

	pylab.show()
	
	
main()	
	
