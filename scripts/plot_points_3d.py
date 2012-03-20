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

GROUPS = ['driven', 'passive', 'both', 'PFit', 'PmeFit', 'allP', 'allPme', 'all']
OTHER_DATA_TYPES = ['time', 'neuronCount', 'synapseCount']	# data not found in a datalib.plt file

DEFAULT_DIRECTORY = '/farmruns/larryy/alifexi2/driven/run_1'

DEFAULT_GROUP = 'both'

DEFAULT_X_DATA_TYPE = 'cc_p_wd'
DEFAULT_Y_DATA_TYPE = 'cpl_p_wd'
DEFAULT_Z_DATA_TYPE = 'P'
DEFAULT_COLOR_DATA_TYPE = 'swi_p_wd_cpl_10_wp'	# color type must be specified (default ignored)
DEFAULT_CLO = 1.0
DEFAULT_CHI = 1.2
DEFAULT_MOD_COLOR = (0.0, 0.0, 0.0) # (1.0, 1.0, 1.0)

LimitX = 0.0 # 30.0 for cpl_p_wd, 4.0 for swi, 0.0 otherwise
LimitY = 0.0
LimitZ = 0.0

HF_MAX = 24.0

use_hf_coloration = False
use_color = True
fixed_color = True
plot_type = 'png'

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
xlo = None
xhi = None
ylo = None
yhi = None
zlo = None
zhi = None
clo = None
chi = None
frame_lo = 0
frame_hi = 359
mod_color = DEFAULT_MOD_COLOR

end_points = []

TimeOfDeath = {}

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
	print '	 current data types include neuronCount (n or nc), synapseCount (s or sc or ec)'
	print '  however a better choice is nc_p_wd and ec_p_wd, etc.  Plus the types'
	print '  identified in common_complexity.py, common_metrics.py, common_motion.py, such as'
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


def get_timestep_dirs(run_dir, group):
	def compare(x,y):
		if int(x) > int(y):
			return 1
		else:
			return -1
			
	if not os.access(run_dir, os.F_OK):
		print 'Cannot access', run_dir
	recent_dir = os.path.join(run_dir, 'brain/Recent')
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
	global plot_type, mod_color
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'dpbPmfFa1C:x:y:z:c:r:', ['driven', 'passive', 'both', 'PFit', 'PmeFit', 'allP', 'allPme', 'all', 'pdf', 'color', 'x_data_type', 'y_data_type', 'z_data_type', 'c_data_type', 'range', 'xlo=', 'xhi=', 'ylo=', 'yhi=', 'zlo=', 'zhi=', 'clo=', 'chi='])
	except getopt.GetoptError, err:
		print str(err)
		usage()
		exit(2)
		
	if len(args) < 1:
		usage()
		#print 'using defaults:', '--'+DEFAULT_GROUP, '-x', DEFAULT_X_DATA_TYPE, '-y', DEFAULT_Y_DATA_TYPE, '-z', DEFAULT_Z_DATA_TYPE, DEFAULT_DIRECTORY
		return DEFAULT_DIRECTORY, DEFAULT_GROUP, DEFAULT_X_DATA_TYPE, DEFAULT_Y_DATA_TYPE, DEFAULT_Z_DATA_TYPE
		
	run_dir = args[0].rstrip('/')
	
	group = DEFAULT_GROUP
	x_data_type = DEFAULT_X_DATA_TYPE
	y_data_type = DEFAULT_Y_DATA_TYPE
	z_data_type = DEFAULT_Z_DATA_TYPE
	c_data_type = None
	for o, a in opts:
		if o in ('-d','--driven'):
			group = 'driven'
		elif o in ('-p', '--passive'):
			group = 'passive'
		elif o in ('-b', '--both'):
			group = 'both'
		elif o in ('-P', '--PFit'):
			group = 'PFit'
		elif o in ('-m', '--PmeFit'):
			group = 'PmeFit'
		elif o in ('-f', '--allP' ):
			group = 'allP'
		elif o in ('-F', '--allPme' ):
			group = 'allPme'
		elif o in ('-a', '--all' ):
			group = 'all'
		elif o in ('-x', '--x_data_type'):
			if a in ('n', 'nc'):
				a = 'neuronCount'
			elif a in ('s', 'sc', 'ec'):
				a = 'synapseCount'
			x_data_type = a
		elif o in ('-y', '--y_data_type'):
			if a in ('n', 'nc'):
				a = 'neuronCount'
			elif a in ('s', 'sc', 'ec'):
				a = 'synapseCount'
			y_data_type = a			
		elif o in ('-z', '--z_data_type'):
			if a in ('n', 'nc'):
				a = 'neuronCount'
			elif a in ('s', 'sc', 'ec'):
				a = 'synapseCount'
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
			frame_hi = 0
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
		else:
			print 'Unknown options:', o
			usage()
			exit(2)
		
	if clo == None:
		clo = DEFAULT_CLO
	if chi == None:
		chi = DEFAULT_CHI
	
	if group not in GROUPS:
		print 'unknown group type:', group
	
	if plot_type == 'pdf' and (frame_lo != 0 or frame_hi != 0):
		print 'Cannot specify multiple frames with PDF file type'
		frame_lo = 0
		frame_hi = 0
					
	return run_dir, group, x_data_type, y_data_type, z_data_type, c_data_type
		
	
def retrieve_time_data(agents):
	time_data = []
	for agent in agents:
		time_data.append(TimeOfDeath[agent])
	
	return time_data

def retrieve_neuron_data(timestep_dir, agents):
	neuron_data = []
	for agent in agents:
		path = os.path.join(timestep_dir, 'brainFunction_'+str(agent)+'.txt')
		neuron_data.append(common_complexity.num_neurons(path))
	
	return neuron_data

def retrieve_synapse_data(timestep_dir, agents):
	synapse_data = []
	for agent in agents:
		path = os.path.join(timestep_dir, 'brainFunction_'+str(agent)+'.txt')
		synapse_data.append(common_complexity.num_synapses(path))
	
	return synapse_data

def retrieve_data_type(timestep_dir, data_type, sim_type, limit, other_data_type):
	if data_type in OTHER_DATA_TYPES:	# for data not found in a datalib.plt file
		# use P complexity as a template
		path, data, agents = retrieve_data_type(timestep_dir, other_data_type, sim_type, limit, None)
		if not agents:
			return None, None, None
		if data_type == 'time':
			time_data = retrieve_time_data(agents)
			return None, time_data, None
		if data_type == 'neuronCount':
			neuron_data = retrieve_neuron_data(timestep_dir, agents)
			return None, neuron_data, agents
		if data_type == 'synapseCount':
			synapse_data = retrieve_synapse_data(timestep_dir, agents)
			return None, synapse_data, agents
	
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
	
def retrieve_data(timestep_dir, x_data_type, y_data_type, z_data_type, c_data_type, sim_type, time, max_time, opacity):
	global use_hf_coloration, LimitX, LimitY, LimitZ
	
	x_path, x_data, x_agents = retrieve_data_type(timestep_dir, x_data_type, sim_type, LimitX, y_data_type)
	y_path, y_data, y_agents = retrieve_data_type(timestep_dir, y_data_type, sim_type, LimitY, x_data_type)
	z_path, z_data, z_agents = retrieve_data_type(timestep_dir, z_data_type, sim_type, LimitZ, x_data_type)
	if c_data_type:
		c_path, c_data_vals, c_agents = retrieve_data_type(timestep_dir, c_data_type, sim_type, 0.0, x_data_type)
	
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
			c_data = map(lambda x: get_time_color(float(time)/float(max_time), sim_type, opacity), range(len(x_data)))
		else:
			c_data = map(lambda x: 0.5, range(len(x_data)))
	
	if c_data_type:
		modify_color(x_agents, c_agents, c_data_vals, c_data, opacity)

	return x_data, y_data, z_data, c_data


def plot_init(title, x_label, y_label, z_label, bounds, size, elev, azim):
	fig = plt.figure(figsize=size)
	ax = Axes3D(fig, elev=elev, azim=azim)
	ax.set_title(title)
	ax.set_xlabel(x_label)
	ax.set_ylabel(y_label)
	ax.set_zlabel(z_label)
	return ax


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


def get_time_color(time_frac, sim_type, opacity):
	c_max = 0.8
	if fixed_color:
		c = 0.5
	else:
		c = c_max - (time_frac * c_max)**2.0
	if sim_type == 'driven':
		color = (c, 1.0, c, opacity) 
	elif sim_type == 'passive':
		color = (c, c, 1.0, opacity)
	elif sim_type == 'PFit':
		color = (1.0, c, c, opacity)
	elif sim_type == 'PmeFit':
		color = (1.0, c, 1.0, opacity)
	else:
		color = (0.5, 0.5, 0.5, opacity)
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
	global xmin, xmax, ymin, ymax, zmin, zmax, xmean, ymean, zmean, xmean2, ymean2, zmean2, num_points, LimitX, LimitY, LimitZ
 	assert(len(x) == len(y) == len(z) == len(color))
	for i in range(len(x)):
		if x[i] < xmin:
			xmin = x[i]
		if x[i] > xmax:
			xmax = x[i]
		if y[i] < ymin:
			ymin = y[i]
		if y[i] > ymax:
			ymax = y[i]
		if z[i] < zmin:
			zmin = z[i]
		if z[i] > zmax:
			zmax = z[i]
		xmean += x[i]
		ymean += y[i]
		zmean += z[i]
		xmean2 += x[i]*x[i]
		ymean2 += y[i]*y[i]
		zmean2 += z[i]*z[i]
		num_points += 1
		if LimitX and x[i] > LimitX:
			x[i] = 0.0
		if LimitY and y[i] > LimitY:
			y[i] = 0.0
		if LimitZ and z[i] > LimitZ:
			z[i] = 0.0

	ax.scatter(x, y, z, zdir='z', c=color, edgecolor=color)

def get_TimeOfDeath(run_dir):
	global TimeOfDeath
	TimeOfDeath.clear()
	path = os.path.join(run_dir, "BirthsDeaths.log")
	log = open(path, 'r')
	for line in log:
		items = line.split(' ')
		if items[1] == 'DEATH':
			for agent in items[2:]:
				TimeOfDeath[int(agent)] = int(items[0])

def plot_dirs(ax, run_dir, timestep_dirs, x_data_type, y_data_type, z_data_type, c_data_type, sim_type, opacity):
	if x_data_type == 'time' or y_data_type == 'time' or z_data_type == 'time':
		get_TimeOfDeath(run_dir)
	
	times = map(lambda x: int(os.path.basename(x)), timestep_dirs)
	max_time = max(times)
	i = 0
	for timestep_dir in timestep_dirs:
		x, y, z, color = retrieve_data(timestep_dir, x_data_type, y_data_type, z_data_type, c_data_type, sim_type, times[i], max_time, opacity)
		if x:
			plot(ax, x, y, z, color)
		i += 1

def get_axis_label(data_type):
	if data_type == 'time':
		label = 'Time'
	elif data_type == 'neuronCount':
		label = 'Neuron Count'
	elif data_type == 'synapseCount':
		label = 'Synapse Count'
	elif data_type[0] in common_complexity.COMPLEXITY_TYPES:
		label = 'Complexity (' + common_complexity.get_name(data_type) + ')'
	else:
		label = common_metric.get_name(data_type)
	
	return label

def gen_plot(run_dir, group, x_data_type, y_data_type, z_data_type, c_data_type, elev, azim):
	global use_hf_coloration, frame_lo, frame_hi
	global xmin, xmax, ymin, ymax, zmin, zmax, xmean, ymean, zmean, xmean2, ymean2, zmean2, num_points, xlo, xhi, ylo, yhi, zlo, zhi

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
	
	x_label = get_axis_label(x_data_type)
	y_label = get_axis_label(y_data_type)
	z_label = get_axis_label(z_data_type)

	title = x_label + ', ' + y_label + ', ' + z_label + ' - Scatter Plot'
 	bounds = (0.0, 0.8, 0.0, 8.0, 0.0, 8.0)  # (x_min, x_max, y_min, y_max, z_min, z_max)
	size = (11.0, 8.5)  # (width, height)
	
	ax = plot_init(title, x_label, y_label, z_label, bounds, size, elev, azim)
	
	type = 'unknown'
	type_in_basename = False
	for possible_type in ('driven', 'passive', 'PFit', 'PmeFit'):
		if possible_type in os.path.basename(run_dir):
			type_in_basename = True
			type = possible_type
			break
	if not type_in_basename:
		for possible_type in ('driven', 'passive', 'PFit', 'PmeFit'):
			if possible_type in run_dir:
				type = possible_type
				break
	if type == 'unknown':
		print 'Sorry, but type of run', '"'+run_dir+'"', 'is unknown; exiting'
		exit()

	tstart = run_dir.rfind(type)
	tlen = len(type)
	driven_dir = run_dir[:tstart] + 'driven' + run_dir[tstart+tlen:]
	passive_dir = run_dir[:tstart] + 'passive' + run_dir[tstart+tlen:]
	PFit_dir = run_dir[:tstart] + 'PFit' + run_dir[tstart+tlen:]
	PmeFit_dir = run_dir[:tstart] + 'PmeFit' + run_dir[tstart+tlen:]
	
# 	print 'driven_dir =', driven_dir
# 	print 'passive_dir =', passive_dir
# 	print 'PFit_dir =', PFit_dir
# 	print 'PmeFit_dir =', PmeFit_dir

	if group == 'passive':
		plot_dirs(ax, run_dir, get_timestep_dirs(passive_dir, 'passive'), x_data_type, y_data_type, z_data_type, c_data_type, 'passive')
	elif group == 'driven':
		plot_dirs(ax, run_dir, get_timestep_dirs(driven_dir, 'driven'), x_data_type, y_data_type, z_data_type, c_data_type, 'driven')
	elif group == 'PFit':
		plot_dirs(ax, run_dir, get_timestep_dirs(PFit_dir, 'PFit'), x_data_type, y_data_type, z_data_type, c_data_type, 'PFit')
	elif group == 'PmeFit':
		plot_dirs(ax, run_dir, get_timestep_dirs(PmeFit_dir, 'PmeFit'), x_data_type, y_data_type, z_data_type, c_data_type, 'PmeFit')
	elif group in ('both', 'allP', 'allPme', 'all'):
		if group == 'allP':
			plot_dirs(ax, PFit_dir, get_timestep_dirs(PFit_dir, 'PFit'), x_data_type, y_data_type, z_data_type, c_data_type, 'PFit', 1.0)
		elif group == 'allPme':
			plot_dirs(ax, PmeFit_dir, get_timestep_dirs(PmeFit_dir, 'PmeFit'), x_data_type, y_data_type, z_data_type, c_data_type, 'PmeFit', 1.0)
		elif group == 'all':
			# try to put whichever run will have the broadest data scatter farthest back (draw it first)
			if x_data_type == 'Pme' or y_data_type == 'Pme':	# do 'PmeFit' first
				plot_dirs(ax, PmeFit_dir, get_timestep_dirs(PmeFit_dir, 'PmeFit'), x_data_type, y_data_type, z_data_type, c_data_type, 'PmeFit', 1.0)
				plot_dirs(ax, PFit_dir, get_timestep_dirs(PFit_dir, 'PFit'), x_data_type, y_data_type, z_data_type, c_data_type, 'PFit', 1.0)
			else:	# do 'PFit' first
				plot_dirs(ax, PFit_dir, get_timestep_dirs(PFit_dir, 'PFit'), x_data_type, y_data_type, z_data_type, c_data_type, 'PFit', 1.0)
				plot_dirs(ax, PmeFit_dir, get_timestep_dirs(PmeFit_dir, 'PmeFit'), x_data_type, y_data_type, z_data_type, c_data_type, 'PmeFit', 1.0)
		plot_dirs(ax, passive_dir, get_timestep_dirs(passive_dir, 'passive'), x_data_type, y_data_type, z_data_type, c_data_type, 'passive', 1.0)
		plot_dirs(ax, driven_dir, get_timestep_dirs(driven_dir, 'driven'), x_data_type, y_data_type, z_data_type, c_data_type, 'driven', 1.0)

	num_std_dev = 4.0
	xmean /= num_points
	ymean /= num_points
	zmean /= num_points
	xsigma = sqrt( xmean2 / num_points  -  xmean * xmean )
	ysigma = sqrt( ymean2 / num_points  -  ymean * ymean )
	zsigma = sqrt( zmean2 / num_points  -  zmean * zmean )

	if xlo == None:
		xlow = max( xmean - num_std_dev*xsigma, xmin )
	else:
		xlow = xlo
	if xhi == None:
		xhigh = min( xmean + num_std_dev*xsigma, xmax )
	else:
		xhigh = xhi
	if ylo == None:
		ylow = max( ymean - num_std_dev*ysigma, ymin )
	else:
		ylow = ylo
	if yhi == None:
		yhigh = min( ymean + num_std_dev*ysigma, ymax )
	else:
		yhigh = yhi
	if zlo == None:
		zlow = max( zmean - num_std_dev*zsigma, zmin )
	else:
		zlow = zlo
	if zhi == None:
		zhigh = min( zmean + num_std_dev*zsigma, zmax )
	else:
		zhigh = zhi

	print 'num_points =', num_points, ', num_std_dev =', num_std_dev
	print 'xmean =', xmean, ', xsigma =', xsigma, ', xm-Nxs =', xmean - num_std_dev*xsigma, ', xm+Nxs =', xmean + num_std_dev*xsigma
	print 'xmin =', xmin, ', xlo =', xlo, ', xlow =', xlow, ', xmax =', xmax, ', xhi =', xhi, ', xhigh =', xhigh
	print 'ymean =', ymean, ', ysigma =', ysigma, ', ym-Nys =', ymean - num_std_dev*ysigma, ', ym+Nys =', ymean + num_std_dev*ysigma
	print 'ymin =', ymin, ', ylo =', ylo, ', ylow =', ylow, ', ymax =', ymax, ', yhi =', yhi, ', yhigh =', yhigh
	print 'zmean =', zmean, ', zsigma =', zsigma, ', zm-Nzs =', zmean - num_std_dev*zsigma, ', zm+Nzs =', zmean + num_std_dev*zsigma
	print 'zmin =', zmin, ', zlo =', zlo, ', zlow =', zlow, ', zmax =', zmax, ', zhi =', zhi, ', zhigh =', zhigh

	ax.set_xlim(xlow, xhigh)
	ax.set_ylim(ylow, yhigh)
	ax.set_zlim(zlow, zhigh)

def main():
	global plot_type
	
	run_dir, group, x_data_type, y_data_type, z_data_type, c_data_type = parse_args()
	
	if plot_type == 'png':
		try:
			os.mkdir('pngs')
		except OSError:
			pass

	# WARNING: Due to a memory leak it is not possible generate all
	# 360 degrees in a single go.  90 degrees at a time seems to just
	# barely work.
	for i in range(frame_lo, frame_hi+1):
		print 'plotting angle', i
		gen_plot(run_dir, group, x_data_type, y_data_type, z_data_type, c_data_type, elev=10, azim=60+i)
		if plot_type == 'png':
			filename = 'pngs/%03d.png' % (i)
			plt.savefig(filename, dpi=72)
		else:
			filename = 'points3D.%s.%s.%s.pdf' % (z_data_type, y_data_type, x_data_type )
			plt.savefig(filename)

	
try:
	main()	
except KeyboardInterrupt:
	print "Exiting due to keyboard interrupt"
	
