#!/usr/bin/env python

import pylab
import os,sys, getopt
import datalib
from common_functions import list_difference, list_intersection
from matplotlib.collections import LineCollection
import common_complexity
import common_metric
from math import sqrt

GROUPS = ['driven', 'passive', 'both', 'PFit', 'PmeFit', 'allP', 'allPme', 'all']
OTHER_DATA_TYPES = ['time', 'neuronCount', 'synapseCount']	# data not found in a datalib.plt file

DEFAULT_DIRECTORY = '/farmruns/larryy/alifexi2/driven/run_1'

DEFAULT_GROUP = 'allP'

DEFAULT_X_DATA_TYPE = 'time'
DEFAULT_Y_DATA_TYPE = 'P'

LimitX = 0.0 # 30.0 for cpl_p_wd, 4.0 for swi, 0.0 otherwise
LimitY = 0.0 # 4.0 for swi, 0.0 otherwise

HF_MAX = 24.0

use_hf_coloration = False
use_color = True
fixed_color = True

xmin = 1000000
xmax = -xmin
ymin = xmin
ymax = xmax
xmean = 0.0
ymean = 0.0
xmean2 = 0.0
ymean2 = 0.0
num_points = 0
xlo = None
xhi = None
ylo = None
yhi = None

end_points = []

TimeOfDeath = {}

def usage():
	print 'Usage:  plot_points.py -x <data_type> -y <data_type> [Options] run_directory'
	print '	 run_directory must be a single run directory'
	print '	 the x-axis will correspond to the -x <data_type>'
	print '	 the y-axis will correspond to the -y <data_type>'
	print ' Options...'
	print '	 if -d or none is specified, only driven runs will be selected when multiple run directories exist'
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
# 	print 'recent_dir =', recent_dir
# 	print 'os.walk(recent_dir) =', os.walk(recent_dir)
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
	global xlo, xhi, ylo, yhi
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'dpbPmfFax:y:', ['driven', 'passive', 'both', 'PFit', 'PmeFit', 'allP', 'allPme', 'all', 'x_data_type', 'y_data_type', 'xlo=', 'xhi=', 'ylo=', 'yhi='])
	except getopt.GetoptError, err:
		print str(err)
		usage()
		exit(2)
		
	if len(args) < 1:
		usage()
		#print 'using defaults:', '--'+DEFAULT_GROUP, '-x', DEFAULT_X_DATA_TYPE, '-y', DEFAULT_Y_DATA_TYPE, DEFAULT_DIRECTORY
		return DEFAULT_DIRECTORY, DEFAULT_GROUP, DEFAULT_X_DATA_TYPE, DEFAULT_Y_DATA_TYPE
		
	run_dir = args[0].rstrip('/')
	
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
		elif o == '--xlo':
			xlo = float(a)
		elif o == '--xhi':
			xhi = float(a)
		elif o == '--ylo':
			ylo = float(a)
		elif o == '--yhi':
			yhi = float(a)
		else:
			print 'Unknown options:', o
			usage()
			exit(2)
	
	if group not in GROUPS:
		print 'unknown group type:', group
					
	return run_dir, group, x_data_type, y_data_type

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
		if limit:
			for i in range(len(data)):
				if data[i] > limit:
					data[i] = limit
	except KeyError:
		data = table.getColumn('Complexity').data # should change to 'Mean' for new format data

	try:
		agents = table.getColumn('AgentNumber').data # should change to 'AgentNumber' for new format data
	except KeyError:
		agents = table.getColumn('CritterNumber').data # should change to 'AgentNumber' for new format data

	return path, data, agents
	
def retrieve_data(timestep_dir, x_data_type, y_data_type, sim_type, time, max_time, opacity):
	global use_hf_coloration
	
	x_path, x_data, x_agents = retrieve_data_type(timestep_dir, x_data_type, sim_type, LimitX, y_data_type)
	y_path, y_data, y_agents = retrieve_data_type(timestep_dir, y_data_type, sim_type, LimitY, x_data_type)
	
	if not x_data or not y_data:
		return None, None, None
	
	# Deal with fact that some data types don't have a datalib path or list of agents
	if not x_path:
		x_path = y_path
	elif not y_path:
		y_path = x_path
	if not x_agents:
		x_agents = y_agents
	if not y_agents:
		y_agents = x_agents

	# Get rid of any agents (and corresponding data points) not in both data sets
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
				pass
	if got_hf_data:
		hf_data = hf_table.getColumn('Mean').data
		c_data = map(lambda x: get_hf_color(x, opacity), hf_data)
	else:
		if use_color:
			c_data = map(lambda x: get_time_color(float(time)/float(max_time), sim_type, opacity), range(len(x_data)))
		else:
			c_data = map(lambda x: 0.5, range(len(x_data)))
	
	return x_data, y_data, c_data


def plot_init(title, x_label, y_label, size):
	fig = pylab.figure(figsize=size)
	ax = fig.add_subplot(111)
	pylab.title(title)
	pylab.xlabel(x_label)
	pylab.ylabel(y_label)
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


def plot(ax, x, y, color):
	global xmin, xmax, ymin, ymax, xmean, ymean, xmean2, ymean2, num_points
	assert(len(x) == len(y) == len(color))
	pylab.scatter(x, y, 3, c=color, edgecolor=color)
	for i in range(len(x)):
		if x[i] < xmin:
			xmin = x[i]
		if x[i] > xmax:
			xmax = x[i]
		if y[i] < ymin:
			ymin = y[i]
		if y[i] > ymax:
			ymax = y[i]
		xmean += x[i]
		ymean += y[i]
		xmean2 += x[i]*x[i]
		ymean2 += y[i]*y[i]
		num_points += 1

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

def plot_dirs(ax, run_dir, timestep_dirs, x_data_type, y_data_type, sim_type, opacity):
	if x_data_type == 'time' or y_data_type == 'time':
		get_TimeOfDeath(run_dir)
	
	times = map(lambda x: int(os.path.basename(x)), timestep_dirs)
	max_time = max(times)
	i = 0
	for timestep_dir in timestep_dirs:
		x, y, color = retrieve_data(timestep_dir, x_data_type, y_data_type, sim_type, times[i], max_time, opacity)
		if x:
			plot(ax, x, y, color)
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

def main():
	global use_hf_coloration
	global xmin, xmax, ymin, ymax, xmean, ymean, xmean2, ymean2, num_points, xlo, xhi, ylo, yhi
	
	run_dir, group, x_data_type, y_data_type = parse_args()
	
	if x_data_type == 'hf' or y_data_type == 'hf':
		use_hf_coloration = False
	
	x_label = get_axis_label(x_data_type)
	y_label = get_axis_label(y_data_type)

	title = y_label + ' vs. ' + x_label + ' - Scatter Plot'
	size = (11.0, 8.5)	# (width, height)
	ax = plot_init(title, x_label, y_label, size)
	
	type = 'unknown'
	type_in_basename = False
	for possible_type in ('driven', 'passive', 'PFit', 'PmeFit'):
		if possible_type.lower() in os.path.basename(run_dir).lower():
			type_in_basename = True
			type = possible_type
			break
	if not type_in_basename:
		for possible_type in ('driven', 'passive', 'PFit', 'PmeFit'):
			if possible_type.lower() in run_dir.lower():
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
		plot_dirs(ax, run_dir, get_timestep_dirs(passive_dir, 'passive'), x_data_type, y_data_type, 'passive', 1.0)
	elif group == 'driven':
		plot_dirs(ax, run_dir, get_timestep_dirs(driven_dir, 'driven'), x_data_type, y_data_type, 'driven', 1.0)
	elif group == 'PFit':
		plot_dirs(ax, run_dir, get_timestep_dirs(PFit_dir, 'PFit'), x_data_type, y_data_type, 'PFit', 1.0)
	elif group == 'PmeFit':
		plot_dirs(ax, run_dir, get_timestep_dirs(PmeFit_dir, 'PmeFit'), x_data_type, y_data_type, 'PmeFit', 1.0)
	elif group in ('both', 'allP', 'allPme', 'all'):
		if group == 'allP':
			plot_dirs(ax, PFit_dir, get_timestep_dirs(PFit_dir, 'PFit'), x_data_type, y_data_type, 'PFit', 1.0)
		elif group == 'allPme':
			plot_dirs(ax, PmeFit_dir, get_timestep_dirs(PmeFit_dir, 'PmeFit'), x_data_type, y_data_type, 'PmeFit', 1.0)
		elif group == 'all':
			# try to put whichever run will have the broadest data scatter farthest back (draw it first)
			if x_data_type == 'Pme' or y_data_type == 'Pme':	# do 'PmeFit' first
				plot_dirs(ax, PmeFit_dir, get_timestep_dirs(PmeFit_dir, 'PmeFit'), x_data_type, y_data_type, 'PmeFit', 1.0)
				plot_dirs(ax, PFit_dir, get_timestep_dirs(PFit_dir, 'PFit'), x_data_type, y_data_type, 'PFit', 1.0)
			else:	# do 'PFit' first
				plot_dirs(ax, PFit_dir, get_timestep_dirs(PFit_dir, 'PFit'), x_data_type, y_data_type, 'PFit', 1.0)
				plot_dirs(ax, PmeFit_dir, get_timestep_dirs(PmeFit_dir, 'PmeFit'), x_data_type, y_data_type, 'PmeFit', 1.0)
		plot_dirs(ax, passive_dir, get_timestep_dirs(passive_dir, 'passive'), x_data_type, y_data_type, 'passive', 1.0)
		plot_dirs(ax, driven_dir, get_timestep_dirs(driven_dir, 'driven'), x_data_type, y_data_type, 'driven', 1.0)
	
	num_std_dev = 4.0
	xmean /= num_points
	ymean /= num_points
	xsigma = sqrt( xmean2 / num_points  -  xmean * xmean )
	ysigma = sqrt( ymean2 / num_points  -  ymean * ymean )

	if xlo == None:
		xlo = max( xmean - num_std_dev*xsigma, xmin )
	if xhi == None:
		xhi = min( xmean + num_std_dev*xsigma, xmax )
	if ylo == None:
		ylo = max( ymean - num_std_dev*ysigma, ymin )
	if yhi == None:
		yhi = min( ymean + num_std_dev*ysigma, ymax )

	print 'num_points =', num_points, ', num_std_dev =', num_std_dev
	print 'xmean =', xmean, ', xsigma =', xsigma, ', xm-Nxs =', xmean - num_std_dev*xsigma, ', xm+Nxs =', xmean + num_std_dev*xsigma
	print 'xmin =', xmin, ', xlo =', xlo, ', xmax =', xmax, ', xhi =', xhi
	print 'ymean =', ymean, ', ysigma =', ysigma, ', ym-Nys =', ymean - num_std_dev*ysigma, ', ym+Nys =', ymean + num_std_dev*ysigma
	print 'ymin =', ymin, ', ylo =', ylo, ', ymax =', ymax, ', yhi =', yhi
	
	ax.set_xlim(xlo, xhi)
	ax.set_ylim(ylo, yhi)

#  	pylab.savefig("./plot_points.eps")
 	pylab.savefig("./plot_points.pdf")
	
try:
	main()	
except KeyboardInterrupt:
	print "Exiting due to keyboard interrupt"

