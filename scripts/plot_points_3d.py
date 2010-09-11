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

GROUPS = ['driven', 'passive', 'both']

#DEFAULT_DIRECTORY = '/pwd/dynamic_food/run_df8_F30_complexity_0_partial'
DEFAULT_DIRECTORY = '/pwd/b2_driven_vs_passive/run_b2_driven_0'
DEFAULT_GROUP = GROUPS[2]
#DEFAULT_X_DATA_TYPE = 'swi_p_wd_npl_10_wp'
DEFAULT_X_DATA_TYPE = 'cc_p_wd'
DEFAULT_Y_DATA_TYPE = 'npl_p_wd'
DEFAULT_Z_DATA_TYPE = 'P'

LimitX = 0.0 # 4.0 for swi, 0.0 otherwise
LimitY = 0.0
LimitZ = 0.0

HF_MAX = 24.0

use_hf_coloration = True

end_points = []

def usage():
	print 'Usage:  plot_points.py -x <data_type> -y <data_type> -z <data_type> [-d|p|b] run_directory'
	print '  run_directory must be a single run directory'
	print '  the x-axis will correspond to the -x <data_type>'
	print '  the y-axis will correspond to the -y <data_type>'
	print '  the z-axis will correspond to the -z <data_type>'
	print '  if -d or none is specified, only driven runs will be selected when multiple run directories exist'
	print '  if -p is specified, only passive runs will be selected when multiple run directories exist'
	print '  if -b is specified, both driven and passive runs will be selected when multiple run directories exist'
	print '  for current data types see common_complexity.py, common_metrics.py, common_motion.py'
	print '  e.g., complexities:',
	for type in common_complexity.COMPLEXITY_TYPES:
		print type,
	print '; metrics:',
	for type in common_metric.METRIC_ROOT_TYPES:
		print type+'_{a|p}_{bu|bd|wu|wd}',
	print
	

def get_filename(data_type):
	if data_type in common_complexity.COMPLEXITY_TYPES:
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
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'dpbx:y:z:', ['driven', 'passive', 'both', 'x_data_type', 'y_data_type', 'z_data_type'])
	except getopt.GetoptError, err:
		print str(err)
		usage()
		exit(2)
		
	if len(args) < 1:
		usage()
		print 'using defaults:', '--'+DEFAULT_GROUP, '-x', DEFAULT_X_DATA_TYPE, '-y', DEFAULT_Y_DATA_TYPE, '-z', DEFAULT_Z_DATA_TYPE, DEFAULT_DIRECTORY
		return DEFAULT_DIRECTORY, DEFAULT_GROUP, DEFAULT_X_DATA_TYPE, DEFAULT_Y_DATA_TYPE, DEFAULT_Z_DATA_TYPE
		
	run_dir = args[0].rstrip('/')
	
	group = DEFAULT_GROUP
	x_data_type = DEFAULT_X_DATA_TYPE
	y_data_type = DEFAULT_Y_DATA_TYPE
	z_data_type = DEFAULT_Z_DATA_TYPE
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
		elif o in ('-z', '--z_data_type'):
			z_data_type = a			
		else:
			print 'Unknown options:', o
			usage()
			exit(2)
	
	if group not in GROUP_TYPES:
		print 'unknown group type:', group
					
	return run_dir, group, x_data_type, y_data_type, z_data_type
		
	
def retrieve_data(timestep_dir, x_data_type, y_data_type, z_data_type, sim_type, time, max_time):
	global use_hf_coloration
	
	x_filename = get_filename(x_data_type)
	x_path = os.path.join(timestep_dir, x_filename)
	y_filename = get_filename(y_data_type)
	y_path = os.path.join(timestep_dir, y_filename)
	z_filename = get_filename(z_data_type)
	z_path = os.path.join(timestep_dir, z_filename)
	if not os.path.exists(x_path) or not os.path.exists(y_path) or not os.path.exists(z_path):
		print 'Warning: Some .plt files are missing:'
		if not os.path.exists(x_path):
			print ' ', x_path
		if not os.path.exists(y_path):
			print ' ', y_path
		if not os.path.exists(z_path):
			print ' ', z_path
		return None, None, None, None
		
	x_table = datalib.parse(x_path)[x_data_type]
	y_table = datalib.parse(y_path)[y_data_type]
	z_table = datalib.parse(z_path)[z_data_type]

	try:
		x_data = x_table.getColumn('Mean').data # should change to 'Mean' for new format data
	except KeyError:
		x_data = x_table.getColumn('Complexity').data # should change to 'Mean' for new format data
	if LimitX:
		for i in range(len(x_data)):
			if x_data[i] > LimitX:
				x_data[i] = 0

	try:
		y_data = y_table.getColumn('Mean').data
	except KeyError:
		y_data = y_table.getColumn('Complexity').data
	if LimitY:
		for i in range(len(y_data)):
			if y_data[i] > LimitY:
				y_data[i] = 0

	try:
		z_data = z_table.getColumn('Mean').data
	except KeyError:
		z_data = z_table.getColumn('Complexity').data
	if LimitZ:
		for i in range(len(z_data)):
			if z_data[i] > LimitZ:
				z_data[i] = 0

	try:
		x_agents = x_table.getColumn('AgentNumber').data# should change to 'AgentNumber' for new format data
	except KeyError:
		x_agents = x_table.getColumn('CritterNumber').data# should change to 'AgentNumber' for new format data

	try:
		y_agents = y_table.getColumn('AgentNumber').data
	except KeyError:
		y_agents = y_table.getColumn('CritterNumber').data
	
	try:
		z_agents = z_table.getColumn('AgentNumber').data
	except KeyError:
		z_agents = z_table.getColumn('CritterNumber').data
	
	#assert(len(x_agents) == len(y_agents))
	
	# Get rid of extra agents to make len(x_data) == len(y_data)
	if len(x_agents) != len(y_agents):
		excluded_agents = list_difference(x_agents,y_agents)
		if len(x_agents) > len(y_agents):
			for agent in excluded_agents:
				agent_id = x_agents.index(agent)
				x_agents.pop(agent_id)
				x_data.pop(agent_id)
		else:
			for agent in excluded_agents:
				agent_id = y_agents.index(agent)
				y_agents.pop(agent_id)
				y_data.pop(agent_id)
	
	# Get rid of extra agents to make len(x_data) == len(z_data) (and len(y_data))
	if len(x_agents) != len(z_agents):
		excluded_agents = list_difference(x_agents,z_agents)
		if len(x_agents) > len(z_agents):
			for agent in excluded_agents:
				agent_id = x_agents.index(agent)
				x_agents.pop(agent_id)
				x_data.pop(agent_id)
				y_agents.pop(agent_id)
				y_data.pop(agent_id)
		else:
			for agent in excluded_agents:
				agent_id = x_agents.index(agent)
				z_agents.pop(agent_id)
				z_data.pop(agent_id)
		
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
		c_data = map(lambda x: get_hf_color(x), hf_data)
# 		c_data = []
# 		for f in hf_data:
# 			c.append(get_hf_color(f))
	else:
		c_data = map(lambda x: get_time_color(float(time)/float(max_time), sim_type), range(len(x_data)))
# 		c = get_time_color(float(time)/float(max_time), sim_type)
# 		c_data = []
# 		for i in range(len(x_data)):
# 			c_data.append(c)

	return x_data, y_data, z_data, c_data


def plot_init(title, x_label, y_label, z_label, bounds, size, elev, azim):
	fig = plt.figure(figsize=size)
	ax = Axes3D(fig, elev=elev, azim=azim)
#	ax = fig.add_subplot(111, projection='3d')
# 	ax.set_xlim(bounds[0], bounds[1])
# 	ax.set_ylim(bounds[2], bounds[3])
	ax.set_title(title)
	ax.set_xlabel(x_label)
	ax.set_ylabel(y_label)
	ax.set_zlabel(z_label)
	return ax


def get_hf_color(f):
	hf_colors = [(0.0, (0.0, 0.0, 1.0)), (20.0, (0.0, 1.0, 0.0)), (24.0, (1.0, 0.0, 0.0))]
	for i in range(1,len(hf_colors)):
		f2, c2 = hf_colors[i]
		if f < f2:
			f1, c1 = hf_colors[i-1]
			r = c1[0]  +  (f - f1) * (c2[0] - c1[0]) / (f2-f1)
			g = c1[1]  +  (f - f1) * (c2[1] - c1[1]) / (f2-f1)
			b = c1[2]  +  (f - f1) * (c2[2] - c1[2]) / (f2-f1)
			c = (r, g, b)
			return c
	f_max, c_max = hf_colors[-1]
	return c_max


def get_time_color(time_frac, sim_type):
	c_max = 0.8
	c = c_max - (time_frac * c_max)**2.0
	if sim_type == 'driven':
		color = (c, 1.0, c, 0.01) 
	else:
		color = (c, c, 1.0, 1.0)
	return color


def plot(ax, x, y, z, color):
 	assert(len(x) == len(y) == len(z) == len(color))
	ax.scatter(x, y, z, zdir='z', c=color, edgecolor=color)
	#pylab.plot(x, y, 50, color, marker='+')


def plot_dirs(ax, timestep_dirs, x_data_type, y_data_type, z_data_type, sim_type):
	times = map(lambda x: int(os.path.basename(x)), timestep_dirs)
	max_time = max(times)
	i = 0
	for timestep_dir in timestep_dirs:
		x, y, z, color = retrieve_data(timestep_dir, x_data_type, y_data_type, z_data_type, sim_type, times[i], max_time)
		if x:
			plot(ax, x, y, z, color)
		i += 1


def gen_plot(run_dir, group, x_data_type, y_data_type, z_data_type, elev, azim):
	if x_data_type in common_complexity.COMPLEXITY_TYPES:
		x_label = 'Complexity (' + common_complexity.COMPLEXITY_NAMES[x_data_type] + ')'
	else:
		x_label = common_metric.get_name(x_data_type)
	if y_data_type in common_complexity.COMPLEXITY_TYPES:
		y_label = 'Complexity (' + common_complexity.COMPLEXITY_NAMES[y_data_type] + ')'
	else:
		y_label = common_metric.get_name(y_data_type)
	if z_data_type in common_complexity.COMPLEXITY_TYPES:
		z_label = 'Complexity (' + common_complexity.COMPLEXITY_NAMES[z_data_type] + ')'
	else:
		z_label = common_metric.get_name(z_data_type)
	title = x_label + ', ' + y_label + ', ' + z_label + ' - Scatter Plot'
	bounds = (0.0, 0.8, 0.0, 8.0, 0.0, 8.0)  # (x_min, x_max, y_min, y_max, z_min, z_max)
	size = (11.0, 8.5)  # (width, height)
	
	ax = plot_init(title, x_label, y_label, z_label, bounds, size, elev, azim)
	
	if group == 'passive':
		plot_dirs(ax, get_timestep_dirs(run_dir, 'passive'), x_data_type, y_data_type, z_data_type, 'passive')
	if group == 'driven':
		plot_dirs(ax, get_timestep_dirs(run_dir, 'driven'), x_data_type, y_data_type, z_data_type, 'driven')
	if group == 'both':
		if 'passive' in os.path.basename(run_dir):
			passive_dir = run_dir
			start = run_dir.rfind('passive')
			driven_dir = run_dir[:start] + 'driven' + run_dir[start+7:]
		else:
			driven_dir = run_dir
			start = run_dir.rfind('driven')
			passive_dir = run_dir[:start] + 'passive' + run_dir[start+6:]
		plot_dirs(ax, get_timestep_dirs(passive_dir, 'passive'), x_data_type, y_data_type, z_data_type, 'passive')
		plot_dirs(ax, get_timestep_dirs(driven_dir, 'driven'), x_data_type, y_data_type, z_data_type, 'driven')
	

def main():
	run_dir, group, x_data_type, y_data_type, z_data_type = parse_args()
	
	# WARNING: Due to a memory leak it is not possible generate all
	# 360 degrees in a single go.  90 degrees at a time seems to just
	# barely work.
	for i in range(0,360):
		gen_plot(run_dir, group, x_data_type, y_data_type, z_data_type, elev=10, azim=60+i)
		filename = 'pngs/%03d.png' % (i)
		plt.savefig(filename, dpi=72)

	
main()	
	
