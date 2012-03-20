#!/usr/bin/env python

#import pylab
import os,sys, getopt
import datalib
from common_functions import list_difference, list_intersection
from matplotlib.collections import LineCollection
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt
import common_complexity
import common_metric
from math import sqrt
import random

DEFAULT_DIRECTORY = '/farmruns/larryy/alifexi2/'
DEFAULT_RUN_TYPES = ['PFit', 'passive', 'driven']
DEFAULT_X_DATA_TYPE = 'cc_p_wd'
DEFAULT_Y_DATA_TYPE = 'cpl_p_wd'
DEFAULT_Z_DATA_TYPE = 'P'

DEFAULT_HI = { 'cc_p_wd':0.4, 'cpl_p_wd':60.0, 'swi_p_wd_cpl_10_wp':4.0, 'eff_p_wd':0.5, 'P':3.0 }
DEFAULT_C_RANGE = { 'cc_p_wd':(0.0,1.0), 'cpl_p_wd':(0.0,1.0), 'swi_p_wd_cpl_10_wp':(0.9,1.1), 'eff_p_wd':(0.0,1.0), 'P':(2.0,10.0) }

DEFAULT_COLOR_DATA_TYPE = 'swi_p_wd_cpl_10_wp'	# color type must be specified (default ignored)
DEFAULT_MOD_COLOR = (0.0, 0.0, 0.0) # (1.0, 1.0, 1.0)

OTHER_DATA_TYPES = ['time']	# data not found in a datalib.plt file

LimitX = 0.0 # 30.0 for cpl_p_wd, 4.0 for swi, 0.0 otherwise
LimitY = 0.0
LimitZ = 0.0

HF_MAX = 24.0

use_hf_coloration = False
use_color = True
fixed_color = True
use_run_colors = False

plot_type = 'png'

retain_point_probability = 0.2

xmin = 1000000
xmax = -xmin
ymin = xmin
ymax = xmax
zmin = xmin
zmax = xmax
xmean = 0.0
ymean = 0.0
zmean = 0.0
xmean2 = 0.0
ymean2 = 0.0
zmean2 = 0.0
xsigma = 0.0
ysigma = 0.0
zsigma = 0.0

num_points = 0
xlo = 0.0
xhi = None
ylo = 0.0
yhi = None
zlo = 0.0
zhi = None

xlow = 0.0
ylow = 0.0
zlow = 0.0
xhigh = 1.0
yhigh = 1.0
zhigh = 1.0

clo = None
chi = None
frame_lo = 0
frame_hi = 360
mod_color = DEFAULT_MOD_COLOR

def usage():
	print 'Usage:  plot_points_3d.py -x <data_type> -y <data_type> -z <data_type> -c <data_type> [Options] run_directory'
	print '  run_directory must be a single run directory'
	print '  the x-axis will correspond to the -x <data_type>'
	print '  the y-axis will correspond to the -y <data_type>'
	print '  the z-axis will correspond to the -z <data_type>'
	print '  if -c is specified the data_type given will be used to color the plotted data'
	print ' Options...'
	print '  if -d or none is specified, only driven runs will be selected when multiple run directories exist'
	print '	 if -p is specified, only passive runs will be selected'
	print '	 if -b is specified, both driven and passive runs will be selected'
	print '  if -P is specified, only PFit runs will be selected'
	print '  if -m is specified, only filtered PmeFit runs will be selected'
	print '  if -f is specified, all of driven, passive, and PFit runs will be selected'
	print '  if -F is specified, all of driven, passive, and PmeFit runs will be selected'
	print '  if -a is specified, all of driven, passive, PFit, and PmeFit runs will be selected'
	print '	 The data types are identified in common_complexity.py, common_metrics.py, common_motion.py, such as'
	print '	   complexities: ',
	for type in common_complexity.COMPLEXITY_TYPES:
		print type,
	print
	print '    metrics: ',
	for type in common_metric.METRIC_ROOT_TYPES:
		print type+'_{a|p}_{bu|bd|wu|wd}',
	print
	print '  --xlo=<float> specifies the minimum x coordinate of the graph'
	print '  --xhi=<float> specifies the maximum x coordinate of the graph'
	print '  --ylo=<float> specifies the minimum y coordinate of the graph'
	print '  --yhi=<float> specifies the maximum y coordinate of the graph'
	print '  --zlo=<float> specifies the minimum z coordinate of the graph'
	print '  --zhi=<float> specifies the maximum z coordinate of the graph'
	print '  --clo=<float> specifies the minimum value of the color data that will be specially colored'
	print '  --chi=<float> specifies the maximum value of the color data that will be specially colored'
	print '  --cm turns on per-run_id color modulation'
	print '  -C r,g,b specifies the color used when the color data is in its affective range'
	print '  -r lo,hi specifies the frame range, from low to high, that will be recorded'
	print '     defaults to 0,359 (Note: range is [lo,hi] inclusive)'
	print '  -1 (one), --pdf : a single PDF plot will be generated, otherwise multiple PNGs'
	print

def get_filename(data_type):
	if data_type[0] in common_complexity.COMPLEXITY_TYPES:
		return 'complexity_'+data_type+'.plt'
	else:
		return 'metric_'+data_type+'.plt'


def get_timestep_dirs(run_dir):
	def compare(x,y):
		if int(x) > int(y):
			return 1
		else:
			return -1
			
	if not os.access(run_dir, os.F_OK):
		print 'Cannot access', run_dir
	recent_dir = os.path.join(run_dir, 'brain/Recent')
	dirs = []
	for root, dirs, files in os.walk(recent_dir):
		break
	time_dirs = dirs[1:]  # get rid of timestep 0

	time_dirs.sort(compare)
	timestep_dirs = []
	for time_dir in time_dirs:
		timestep_dirs.append(os.path.join(root,time_dir))
		
	return timestep_dirs


def parse_args():
	global xlo, xhi, ylo, yhi, zlo, zhi, clo, chi, LimitX, LimitY, LimitZ, frame_lo, frame_hi
	global plot_type, mod_color, use_run_colors, retain_point_probability
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'dpbPmfFa1C:x:y:z:c:r:', ['driven', 'passive', 'both', 'PFit', 'PmeFit', 'allP', 'allPme', 'all', 'pdf', 'color', 'x_data_type', 'y_data_type', 'z_data_type', 'c_data_type', 'range', 'xlo=', 'xhi=', 'ylo=', 'yhi=', 'zlo=', 'zhi=', 'clo=', 'chi=', 'cm', 'prob='])
	except getopt.GetoptError, err:
		print str(err)
		usage()
		exit(2)
		
	if len(args) < 1:
		usage()
		#print 'using defaults:', '--'+DEFAULT_RUN_TYPES, '-x', DEFAULT_X_DATA_TYPE, '-y', DEFAULT_Y_DATA_TYPE, '-z', DEFAULT_Z_DATA_TYPE, DEFAULT_DIRECTORY
		return DEFAULT_DIRECTORY, DEFAULT_RUN_TYPES, DEFAULT_X_DATA_TYPE, DEFAULT_Y_DATA_TYPE, DEFAULT_Z_DATA_TYPE
		
	run_dir = args[0].rstrip('/')
	
	run_types = DEFAULT_RUN_TYPES
	x_data_type = DEFAULT_X_DATA_TYPE
	y_data_type = DEFAULT_Y_DATA_TYPE
	z_data_type = DEFAULT_Z_DATA_TYPE
	c_data_type = None
	
	for o, a in opts:
		if o in ('-d','--driven'):
			run_types = ['driven']
		elif o in ('-p', '--passive'):
			run_types = ['passive']
		elif o in ('-b', '--both'):
			run_types = ['passive', 'driven']
		elif o in ('-P', '--PFit'):
			run_types = ['PFit']
		elif o in ('-m', '--PmeFit'):
			run_types = ['PmeFit']
		elif o in ('-f', '--allP' ):
			run_types = ['PFit', 'passive', 'driven']
		elif o in ('-F', '--allPme' ):
			run_types = ['PmeFit', 'passive', 'driven']
		elif o in ('-a', '--all' ):
			run_types = ['PmeFit', 'PFit', 'passive', 'driven']
		elif o in ('-x', '--x_data_type'):
			x_data_type = a
		elif o in ('-y', '--y_data_type'):
			y_data_type = a			
		elif o in ('-z', '--z_data_type'):
			z_data_type = a			
		elif o in ('-c', '--c_data_type'):
			c_data_type = a
		elif o in ('-r', '--range'):
			range = a.split(',')
			frame_lo = int(range[0])
			frame_hi = int(range[1])
		elif o in ('-1', '--pdf'):
			plot_type = 'pdf'
			frame_lo = 0
			frame_hi = 1
		elif o in ('-C', '--color'):
			mod_color = map(float,a.split(','))
		elif o == '--xlo':
			xlo = float(a)
		elif o == '--xhi':
			xhi = float(a)
			LimitX = xhi
		elif o == '--ylo':
			ylo = float(a)
		elif o == '--yhi':
			yhi = float(a)
			LimitY = yhi
		elif o == '--zlo':
			zlo = float(a)
		elif o == '--zhi':
			zhi = float(a)
			LimitZ = zhi
		elif o == '--clo':
			clo = float(a)
		elif o == '--chi':
			chi = float(a)
		elif o == '--cm':
			use_run_colors = True
		elif o == '--prob':
			retain_point_probability = float(a)
		else:
			print 'Unknown options:', o
			usage()
			exit(2)
	
	if c_data_type:
		if clo == None:
			clo = DEFAULT_C_RANGE[c_data_type][0]
		if chi == None:
			chi = DEFAULT_C_RANGE[c_data_type][1]
	
	if not LimitX:
		LimitX = DEFAULT_HI[x_data_type]
	if not LimitY:
		LimitY = DEFAULT_HI[y_data_type]	
	if not LimitX:
		LimitZ = DEFAULT_HI[z_data_type]
	
	if plot_type == 'pdf' and (frame_lo != 0 or frame_hi != 1):
		print 'Cannot specify multiple frames with PDF file type'
		frame_lo = 0
		frame_hi = 1
					
	return run_dir, run_types, x_data_type, y_data_type, z_data_type, c_data_type
		
	
def retrieve_time_data(agents, death_times):
	time_data = []
	for agent in agents:
		time_data.append(death_times[agent])
	
	return time_data

# This is terribly evil, figure out a better way...
__i = 0

def filter_data(data, ran_list):
	global __i
	
	def __keep(x):
		global __i
		keep = ran_list[__i] < retain_point_probability
		__i += 1
		return keep
	
	__i = 0
	data = filter(__keep, data)

	return data

def retrieve_data_type(timestep_dir, data_type, run_type, limit, other_data_type, death_times):
	if data_type in OTHER_DATA_TYPES:	# for data not found in a datalib.plt file
		# use other_data_type as a template
		path, data, agents = retrieve_data_type(timestep_dir, other_data_type, run_type, limit, None, death_times)
		if not agents:
			return None, None, None
		if data_type == 'time':
			time_data = retrieve_time_data(agents, death_times)
			return None, time_data, None
	
	filename = get_filename(data_type)
	path = os.path.join(timestep_dir, filename)
	if not os.path.exists(path):
		print 'Warning:', path, '.plt file is missing:'
		return None, None, None

	table = datalib.parse(path)[data_type]
	
	try:
		data = table.getColumn('Mean').data # should change to 'Mean' for new format data
# 		if limit:
# 			for i in range(len(data)):
# 				if data[i] > limit:
# 					data[i] = 0.0
	except KeyError:
		data = table.getColumn('Complexity').data # should change to 'Mean' for new format data

	try:
		agents = table.getColumn('AgentNumber').data # should change to 'AgentNumber' for new format data
	except KeyError:
		agents = table.getColumn('CritterNumber').data # should change to 'AgentNumber' for new format data

	return path, data, agents
	
def retrieve_data(timestep_dir, x_data_type, y_data_type, z_data_type, c_data_type, run_type, time, max_time, cmult, opacity, death_times):
	global use_hf_coloration, xlow, xhigh, ylow, yhigh, zlow, zhigh
	global xmin, xmax, ymin, ymax, zmin, zmax, xmean, ymean, zmean, xmean2, ymean2, zmean2, num_points
	
	x_path, x_data, x_agents = retrieve_data_type(timestep_dir, x_data_type, run_type, LimitX, y_data_type, death_times)
	y_path, y_data, y_agents = retrieve_data_type(timestep_dir, y_data_type, run_type, LimitY, x_data_type, death_times)
	z_path, z_data, z_agents = retrieve_data_type(timestep_dir, z_data_type, run_type, LimitZ, x_data_type, death_times)
	if c_data_type:
		c_path, c_data_vals, c_agents = retrieve_data_type(timestep_dir, c_data_type, run_type, 0.0, x_data_type, death_times)
	
	if not x_data or not y_data or not z_data:
		return None, None, None, None
		
	# Deal with fact that some data types don't have a datalib path or list of agents
	if not x_path:
		if y_path:
			x_path = y_path
		else:
			x_path = z_path
	if not y_path:
		if x_path:
			y_path = x_path
		else:
			y_path = z_path
	if not z_path:
		if x_path:
			z_path = x_path
		else:
			z_path = y_path
	if not x_agents:
		if y_agents:
			x_agents = y_agents
		else:
			x_agents = z_agents
	if not y_agents:
		if x_agents:
			y_agents = x_agents
		else:
			y_agents = z_agents
	if not z_agents:
		if x_agents:
			z_agents = x_agents
		else:
			z_agents = y_agents

	# Get rid of any agents (and corresponding data points) not in all three data sets
	excluded_agents = list_difference(x_agents, y_agents)	# agents in x, but not in y
	if excluded_agents:
		for agent in excluded_agents:
			agent_idx = x_agents.index(agent)
			x_agents.pop(agent_idx)
			x_data.pop(agent_idx)
	excluded_agents = list_difference(y_agents, x_agents)	# agents in y, but not in x
	if excluded_agents:
		for agent in excluded_agents:
			agent_idx = y_agents.index(agent)
			y_agents.pop(agent_idx)
			y_data.pop(agent_idx)
	len_y = len(y_agents)
	excluded_agents = list_difference(y_agents, z_agents)	# agents in y, but not in z
	if excluded_agents:
		for agent in excluded_agents:
			agent_idx = y_agents.index(agent)
			y_agents.pop(agent_idx)
			y_data.pop(agent_idx)
	excluded_agents = list_difference(z_agents, y_agents)	# agents in z, but not in y
	if excluded_agents:
		for agent in excluded_agents:
			agent_idx = z_agents.index(agent)
			z_agents.pop(agent_idx)
			z_data.pop(agent_idx)
	if len_y != len(y_agents):
		# y had agents eliminated due to z, so now have to remove them from x as well
		excluded_agents = list_difference(x_agents, y_agents)	# agents in x, but not in y
		if excluded_agents:
			for agent in excluded_agents:
				agent_idx = x_agents.index(agent)
				x_agents.pop(agent_idx)
				x_data.pop(agent_idx)
	
	# Make the same random selections from all data sets to properly decimate the data
	ran_list = [random.random() for x in x_data]
	x_data = filter_data(x_data, ran_list)
	x_agents = filter_data(x_agents, ran_list)
	y_data = filter_data(y_data, ran_list)
	y_agents = filter_data(y_agents, ran_list)
	z_data = filter_data(z_data, ran_list)
	z_agents = filter_data(z_agents, ran_list)

	got_hf_data = False
	if use_hf_coloration:
		try:
			hf_table = datalib.parse(y_path)['hf']
			got_hf_data = True
		except KeyError:
			try:
				hf_table = datalib.parse(x_path)['hf']
				got_hf_data = True
			except KeyError:
				try:
					hf_table = datalib.parse(z_path)['hf']
					got_hf_data = True
				except KeyError:
					pass
	if got_hf_data:
		hf_data = hf_table.getColumn('Mean').data
		c_data = map(lambda x: get_hf_color(x, opacity), hf_data)
	else:
		if use_color:
			c_data = map(lambda x: get_time_color(float(time)/float(max_time), run_type, cmult, opacity), range(len(x_data)))
		else:
			c_data = map(lambda x: 0.5, range(len(x_data)))
	
	if c_data_type:
		modify_color(x_agents, c_agents, c_data_vals, c_data, opacity)

	for i in range(len(x_data)):
		if x_data[i] < xmin:
			xmin = x_data[i]
		if x_data[i] > xmax:
			xmax = x_data[i]
		if y_data[i] < ymin:
			ymin = y_data[i]
		if y_data[i] > ymax:
			ymax = y_data[i]
		if z_data[i] < zmin:
			zmin = z_data[i]
		if z_data[i] > zmax:
			zmax = z_data[i]
		xmean += x_data[i]
		ymean += y_data[i]
		zmean += z_data[i]
		xmean2 += x_data[i]*x_data[i]
		ymean2 += y_data[i]*y_data[i]
		zmean2 += z_data[i]*z_data[i]
		num_points += 1

	return x_data, y_data, z_data, c_data


def get_hf_color(f, opacity):
	hf_colors = [(0.0, (0.0, 0.0, 1.0)), (20.0, (0.0, 1.0, 0.0)), (24.0, (1.0, 0.0, 0.0))]
	for i in range(1,len(hf_colors)):
		f2, c2 = hf_colors[i]
		if f < f2:
			f1, c1 = hf_colors[i-1]
			r = c1[0]  +  (f - f1) * (c2[0] - c1[0]) / (f2-f1)
			g = c1[1]  +  (f - f1) * (c2[1] - c1[1]) / (f2-f1)
			b = c1[2]  +  (f - f1) * (c2[2] - c1[2]) / (f2-f1)
			c = (r, g, b, opacity)
			return c
	f_max, c_max = hf_colors[-1]
	return c_max


def get_time_color(time_frac, run_type, cmult, opacity):
	c_max = 0.8
	if fixed_color:
		c = 0.5
	else:
		c = c_max - (time_frac * c_max)**2.0
	c *= cmult
	full = cmult
	if run_type == 'driven':
		color = (c, full, c, opacity) 
	elif run_type == 'passive':
		color = (c, c, full, opacity)
	elif run_type == 'PFit':
		color = (full, c, c, opacity)
	elif run_type == 'PmeFit':
		color = (full, c, full, opacity)
	else:
		color = (cmult*0.5, cmult*0.5, cmult*0.5, opacity)
	return color

def modify_color(x_agents, c_agents, c_data_vals, c_data, opacity):
	for a in range(len(x_agents)):
		agent = x_agents[a]
		try:
			c = c_agents.index(agent)
			val = c_data_vals[c]
			if clo <= val <= chi:
				c_data[a] = (mod_color[0], mod_color[1], mod_color[2], opacity)
# 				mid = 0.5 * (clo + chi)
# 				if val <= mid:
# 					frac = (val - clo) / (mid - clo)
# 					r = c_data[idx][0] + frac * (mod_color[0] - c_data[idx][0])
# 					g = c_data[idx][1] + frac * (mod_color[1] - c_data[idx][1])
# 					b = c_data[idx][2] + frac * (mod_color[2] - c_data[idx][2])
# 				else:
# 					frac = (val - mid) / (chi - mid)
# 					r = mod_color[0] + frac * (c_data[idx][0] - mod_color[0])
# 					g = mod_color[1] + frac * (c_data[idx][1] - mod_color[1])
# 					b = mod_color[2] + frac * (c_data[idx][2] - mod_color[2])
# 				c_data[idx] = (r, g, b, opacity)
		except ValueError:
			pass

def plot(ax, x, y, z, color):
 	assert(len(x) == len(y) == len(z) == len(color))
	ax.scatter(x, y, z, zdir='z', c=color, edgecolor=color)

def get_death_times(run_dir):
	death_times = {}
	path = os.path.join(run_dir, "BirthsDeaths.log")
	log = open(path, 'r')
	for line in log:
		items = line.split(' ')
		if items[1] == 'DEATH':
			for agent in items[2:]:
				death_times[int(agent)] = int(items[0])
	log.close()
	return death_times

def gather_run_dir_data(run_dir, times, x_data_type, y_data_type, z_data_type, c_data_type, run_type, cmult, opacity):
	x_data = []
	y_data = []
	z_data = []
	c_data = []
	
	print os.path.basename(run_dir),
	sys.stdout.flush()
	if x_data_type == 'time' or y_data_type == 'time':
		death_times = get_death_times(run_dir)
	else:
		death_times = []
	
	max_time = times[len(times)-1] # max(times)
	for i in range(len(times)):
		timestep_dir = run_dir + '/brain/Recent/' + str(times[i])
		x, y, z, c = retrieve_data(timestep_dir, x_data_type, y_data_type, z_data_type, c_data_type, run_type, times[i], max_time, cmult, opacity, death_times)
		x_data += x
		y_data += y
		z_data += z
		c_data += c
	
	return x_data, y_data, z_data, c_data

def gather_run_type_data(type_dir, times, x_data_type, y_data_type, z_data_type, c_data_type, run_type, opacity):
	x_data = []
	y_data = []
	z_data = []
	c_data = []

	# for now assume there are always 10 runs
	# TODO: figure this out by looking in the type_dir for run_# dirs
	num_runs = 10
	
	print 'gathering data from', type_dir+'/',
	for run_id in range(num_runs):
		run_dir = type_dir + '/run_' + str(run_id)
		if use_run_colors:
			cmult = 1.0 - 0.05*run_id
		else:
			cmult = 1.0
		x, y, z, c = gather_run_dir_data(run_dir, times, x_data_type, y_data_type, z_data_type, c_data_type, run_type, cmult, opacity)
		x_data += x
		y_data += y
		z_data += z
		c_data += c
	print
	sys.stdout.flush()
	
	return x_data, y_data, z_data, c_data

def get_axis_label(data_type):
	if data_type == 'time':
		label = 'Time'
	elif data_type[0] in common_complexity.COMPLEXITY_TYPES:
		label = 'Complexity (' + common_complexity.get_name(data_type) + ')'
	else:
		label = common_metric.get_name(data_type)
	
	return label

def gather_data(data_dir, run_types, x_data_type, y_data_type, z_data_type, c_data_type):
	global use_hf_coloration, frame_lo, frame_hi
	global xmin, xmax, ymin, ymax, zmin, zmax, xmean, ymean, zmean, xmean2, ymean2, zmean2, xsigma, ysigma, zsigma
	global num_points, xlo, xhi, ylo, yhi, zlo, zhi
	global xlow, ylow, zlow, xhigh, yhigh, zhigh
	
	x_data = []
	y_data = []
	z_data = []
	c_data = []

	xmin = 1000000
	xmax = -xmin
	ymin = xmin
	ymax = xmax
	zmin = xmin
	zmax = xmax
	xmean = 0.0
	ymean = 0.0
	zmean = 0.0
	xmean2 = 0.0
	ymean2 = 0.0
	zmean2 = 0.0
	num_points = 0

	if 'hf' in (x_data_type, y_data_type, z_data_type):
		use_hf_coloration = False
	
	# For now assume the timestep_dirs are the same for all run types and run IDs
	timestep_dirs = get_timestep_dirs(data_dir+'/'+run_types[0]+'/run_0')
	times = map(lambda x: int(os.path.basename(x)), timestep_dirs)
	
	for run_type in run_types:
		x, y, z, c = gather_run_type_data(data_dir+'/'+run_type, times, x_data_type, y_data_type, z_data_type, c_data_type, run_type, 1.0)	
		x_data += x
		y_data += y
		z_data += z
		c_data += c
	
	num_std_dev = 4.0
	xmean /= num_points
	ymean /= num_points
	zmean /= num_points
	xsigma = sqrt( xmean2 / num_points  -  xmean * xmean )
	ysigma = sqrt( ymean2 / num_points  -  ymean * ymean )
	zsigma = sqrt( zmean2 / num_points  -  zmean * zmean )

	if xlo == None:
		#xlow = max( xmean - num_std_dev*xsigma, xmin )
		xlow = 0.0
	else:
		xlow = xlo
	if xhi == None:
		#xhigh = min( xmean + num_std_dev*xsigma, xmax )
		xhigh = DEFAULT_HI[x_data_type]
	else:
		xhigh = xhi
	if ylo == None:
		#ylow = max( ymean - num_std_dev*ysigma, ymin )
		ylow = 0.0
	else:
		ylow = ylo
	if yhi == None:
		#yhigh = min( ymean + num_std_dev*ysigma, ymax )
		yhigh = DEFAULT_HI[y_data_type]
	else:
		yhigh = yhi
	if zlo == None:
		#zlow = max( zmean - num_std_dev*zsigma, zmin )
		zlow = 0.0
	else:
		zlow = zlo
	if zhi == None:
		#zhigh = min( zmean + num_std_dev*zsigma, zmax )
		zhigh = DEFAULT_HI[z_data_type]
	else:
		zhigh = zhi

	num_limited = 0
	num_limited_x = 0
	num_limited_y = 0
	num_limited_z = 0
	for i in range(len(x_data)):
		limited = False
		if x_data[i] < xlow or x_data[i] > xhigh:
			x_data[i] = 0.0
			num_limited_x += 1
			limited = True
		if y_data[i] < ylow or y_data[i] > yhigh:
			y_data[i] = 0.0
			num_limited_y += 1
			limited = True
		if z_data[i] < zlow or z_data[i] > zhigh:
			z_data[i] = 0.0
			num_limited_z += 1
			limited = True
		if limited:
			num_limited += 1
	
	print 'num_points =', num_points, ', num_std_dev =', num_std_dev, ', num_limited =', num_limited
	print 'num_limited_x =', num_limited_x, ', num_limited_y =', num_limited_y, ', num_limited_z =', num_limited_z
	print 'xmean =', xmean, ', xsigma =', xsigma, ', xm-Nxs =', xmean - num_std_dev*xsigma, ', xm+Nxs =', xmean + num_std_dev*xsigma
	print 'xmin =', xmin, ', xlo =', xlo, ', xlow =', xlow, ', xmax =', xmax, ', xhi =', xhi, ', xhigh =', xhigh
	print 'ymean =', ymean, ', ysigma =', ysigma, ', ym-Nys =', ymean - num_std_dev*ysigma, ', ym+Nys =', ymean + num_std_dev*ysigma
	print 'ymin =', ymin, ', ylo =', ylo, ', ylow =', ylow, ', ymax =', ymax, ', yhi =', yhi, ', yhigh =', yhigh
	print 'zmean =', zmean, ', zsigma =', zsigma, ', zm-Nzs =', zmean - num_std_dev*zsigma, ', zm+Nzs =', zmean + num_std_dev*zsigma
	print 'zmin =', zmin, ', zlo =', zlo, ', zlow =', zlow, ', zmax =', zmax, ', zhi =', zhi, ', zhigh =', zhigh
	
	return x_data, y_data, z_data, c_data

def plot_init(title, x_label, y_label, z_label, bounds, size, elev, azim):
	fig = plt.figure(figsize=size)
	ax = Axes3D(fig, elev=elev, azim=azim)
	ax.set_title(title)
	ax.set_xlabel(x_label)
	ax.set_ylabel(y_label)
	ax.set_zlabel(z_label)
# 	ax.margins(0,0,0)
# 	ax.autoscale(False, tight=True)
# 	ax.set_xlim(xlow, xhigh)
# 	ax.set_ylim(ylow, yhigh)
# 	ax.set_zlim(zlow, zhigh)

	return ax


def gen_plot(x, y, z, c, x_label, y_label, z_label, elev, azim):
	global use_hf_coloration, frame_lo, frame_hi
	global xmin, xmax, ymin, ymax, zmin, zmax, xmean, ymean, zmean, xmean2, ymean2, zmean2, num_points, xlo, xhi, ylo, yhi, zlo, zhi
	global xlow, ylow, zlow, xhigh, yhigh, zhigh

	title = x_label + ', ' + y_label + ', ' + z_label + ' - Scatter Plot'
 	bounds = (xlow, xhigh, ylow, yhigh, zlow, zhigh)  # (x_min, x_max, y_min, y_max, z_min, z_max)
	size = (11.0, 8.5)  # (width, height)
	
	ax = plot_init(title, x_label, y_label, z_label, bounds, size, elev, azim)
	
	plot(ax, x, y, z, c)

	ax.set_xlim(xlow, xhigh)
	ax.set_ylim(ylow, yhigh)
	ax.set_zlim(zlow, zhigh)
	
	return ax

def main():
	global plot_type
	png_dir = 'pngs_N2'
	
	random.seed(42)	# do this here so each frame picks the same points to display

	data_dir, run_types, x_data_type, y_data_type, z_data_type, c_data_type = parse_args()
	
	if plot_type == 'png':
		try:
			os.mkdir(png_dir)
		except OSError:
			pass
	
	x_data, y_data, z_data, c_data = gather_data(data_dir, run_types, x_data_type, y_data_type, z_data_type, c_data_type)

	x_label = get_axis_label(x_data_type)
	y_label = get_axis_label(y_data_type)
	z_label = get_axis_label(z_data_type)

	ax = gen_plot(x_data, y_data, z_data, c_data, x_label, y_label, z_label, elev=10, azim=60)

	# Due to some kind of memory leak virtual memory use grows rapidly,
	# so it is recommended that the entire 360 degrees not be rendered at once.
	# 90 degress just barely works.  45 degrees is recommended.
	for i in range(frame_lo, frame_hi):
		print 'plotting angle', i
		ax.view_init(azim=60+i)
		if plot_type == 'png':
			filename = '%s/%03d.png' % (png_dir, i)
			plt.savefig(filename, dpi=72)
		else:
			filename = 'points3DN2.%s.%s.%s.pdf' % (z_data_type, y_data_type, x_data_type )
			plt.savefig(filename)

	
try:
	main()	
except KeyboardInterrupt:
	print "Exiting due to keyboard interrupt"
	
