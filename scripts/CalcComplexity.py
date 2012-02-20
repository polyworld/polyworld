#!/usr/bin/env python

CurrentVersion=2

### Configurable parameters
OutputFilename2='ComplexityHist.plt'

#CalcComplexity.py

#### Don't modify anything beneath here unless you know what you're doing
import abstractfile
import algorithms
import getopt, glob, string, sys, os
import common_functions
from common_functions import err, warn, list_difference
import common_complexity
import datalib
import farm
import shlex
import subprocess
import datetime

### Now initialize some global variables
OutputFilename = common_complexity.FILENAME_AVR
NUMBINS = common_complexity.DEFAULT_NUMBINS
OverwriteEpochComplexities=False

DEFAULT_RECENT = 'Recent'

LEGACY_MODES = ['force','prefer','off']
LegacyMode = 'off'
####

####################################################################################
###
### main()
###
####################################################################################
def main():
	check_environment()

	complexities, recent_type, arg_paths = parse_args( sys.argv[1:] )

	try:
		run_paths = common_functions.find_run_paths( arg_paths,
													 'brain/function/' )
	except common_functions.InvalidDirError, e:
		show_usage(str(e))

	recent_subpath = os.path.join('brain', recent_type)
	recent_dirs = []

	for run_dir in run_paths:
		recent_dir = os.path.join(run_dir, recent_subpath)

		if not os.path.exists( recent_dir ):
			if recent_type != 'Recent':
				show_usage( 'Unsupported recent type for -d' )

			make_recent( run_dir )

		recent_dirs.append( recent_dir )

	for recent_dir in recent_dirs:
		analyze_recent_dir(complexities, recent_dir)

	print "Done!"
	return 0

####################################################################################
###
### FUNCTION make_recent()
###
####################################################################################
def make_recent( run_dir ):
	script = common_functions.pw_env('makeRecent')
	os.system( script + " " + run_dir )

####################################################################################
###
### FUNCTION check_environment()
###
####################################################################################
def check_environment():
	global CALC_COMPLEXITY

	CALC_COMPLEXITY = common_functions.pw_env('complexity')

####################################################################################
###
### FUNCTION parse_args()
###
####################################################################################
def parse_args(argv):
	global NUMBINS, OverwriteEpochComplexities, LegacyMode

	if len(argv) == 0:
		show_usage()

	complexities = common_complexity.DEFAULT_COMPLEXITIES
	recent_type = DEFAULT_RECENT

	short = 'C:Ol:b:r:d'
	long = ['legacy=', 'bins=']

	try:
		opts, args = getopt.getopt(argv, short, long)
	except getopt.GetoptError, e:
		show_usage(str(e))

	for opt, value in opts:
		opt = opt.strip('-')

		if opt == 'C':
			complexities = value.split(',')
			for i in range(0,len(complexities)):
				# strip any trailing single-digit ones
				if complexities[i][-1] == '1' and not complexities[i][-2].isdigit():
					complexities[i] = complexities[i][:-1]
		elif opt == 'O':
			OverwriteEpochComplexities = True
		elif opt == 'l' or opt == 'legacy':
			if value not in LEGACY_MODES:
				show_usage('Invalid legacy mode (%s)' % value)
			LegacyMode = value
		elif opt == 'b' or opt == 'bins':
			try:
				NUMBINS = int(value)
			except:
				show_usage("Invalid integer argument for --bins (%s)" % value)
		elif opt == 'r':
			recent_type = value
		else:
			assert(False)

	if not recent_type in common_functions.RECENT_TYPES:
		show_usage('Invalid recent type (%s).' % recent_type)

	if len(args) == 0:
		show_usage('Must specify run/run-parent directory.')

	if OverwriteEpochComplexities and LegacyMode != 'off':
		err( '-O makes no sense with -l' )

	paths = list(args)

	return complexities, recent_type, paths

####################################################################################
###
### analyze_recent_dir()
###
####################################################################################
def analyze_recent_dir(complexities, recent_dir):
	outputpath = os.path.join(recent_dir, OutputFilename);
	
	print "- recent directory='%s'" %(recent_dir)
	print "- output='%s'" % (outputpath)
	
	#-----------------------------------------------------------------------------------
	#--
	#-- Find epoch/timestep directories
	#--
	#-----------------------------------------------------------------------------------
	timesteps = []
	# list all of the timesteps, make sure they are all integers (and directories), then sort them by number.
	for potential_timestep in os.listdir( recent_dir ):
		if not potential_timestep.isdigit(): continue					# if timestep IS NOT a digit (note, 0 is considered a digit), skip.
		if not os.path.isdir( os.path.join(recent_dir, potential_timestep) ): continue	# if the timestep isn't a directory, skip it.
	
		timesteps.append( int(potential_timestep) )						# add timestep to our list
	
	if len(timesteps) == 0:
		err('No epochs found. Not a valid recent directory.')

	timesteps.sort()									# sort the timesteps, lowest numbers come first.
	
	#-----------------------------------------------------------------------------------
	#--
	#-- Compute complexities for all timesteps
	#--
	#-- (store values to file in timestep dir)
	#--
	#-----------------------------------------------------------------------------------
	DATA={ }
	
	print "Final Timestep: %s" % ( max(timesteps) )
	print datetime.datetime.now()
	print "Processing:",
	
	for t in timesteps:
		timestep_directory = os.path.join(recent_dir, str(t))
		print '%s at %s...' % (t, datetime.datetime.now())
		sys.stdout.flush()	
	
		DATA[t] = tdata = {}

		complexities_remaining = complexities

		if LegacyMode != 'off':
			complexities_read = read_legacy_complexities( complexities_remaining,
														  timestep_directory,
														  tdata )
			complexities_remaining = list_difference( complexities_remaining,
													  complexities_read )
			if len(complexities_remaining) != 0:
				if LegacyMode == 'force':
					err('Failed to find data for %s' % ','.join(complexities_remaining))
			
			print '  Legacy =', complexities_read

		if len(complexities_remaining) > 0:
			complexities_computed = compute_complexities( complexities_remaining,
														  t,
														  timestep_directory,
														  tdata )
			complexities_remaining = list_difference( complexities_remaining,
													  complexities_computed )

		assert(len(complexities_remaining) == 0)
	
	#-----------------------------------------------------------------------------------
	#--
	#-- Create 'Avr' File
	#--
	#-----------------------------------------------------------------------------------
	AVR = algorithms.avr_table( DATA,
								complexities,
								timesteps )
	
	datalib.write(outputpath, AVR, append=True)
	
	
	#-----------------------------------------------------------------------------------
	#--
	#-- Create 'Norm' file
	#--
	#-----------------------------------------------------------------------------------
	tables = compute_bins( DATA,
						   timesteps,
						   complexities,
						   AVR,
						   lambda row: row.get('min'),
						   lambda row: row.get('max') )
	
	outputpath = os.path.join(recent_dir, OutputFilename2.replace( '.', 'Norm.'))
	
	datalib.write(outputpath, tables)
	
	
	#-----------------------------------------------------------------------------------
	#--
	#-- Create 'Raw' file
	#--
	#-----------------------------------------------------------------------------------
	MAXGLOBAL = dict([(type, float('-inf')) for type in complexities])
	MINGLOBAL = dict([(type, float('inf')) for type in complexities])
	
	for avr_table in AVR.values():
		for row in avr_table.rows():
			type = avr_table.name
	
			MAXGLOBAL[type] = max(MAXGLOBAL[type], row.get('max'));
			MINGLOBAL[type] = min(MINGLOBAL[type], row.get('min'));
	
	tables = compute_bins( DATA,
						   timesteps,
						   complexities,
						   AVR,
						   lambda row: MINGLOBAL[row.table.name],
						   lambda row: MAXGLOBAL[row.table.name] )
	
	outputpath = os.path.join(recent_dir, OutputFilename2.replace( '.', 'Raw.'))
	
	datalib.write(outputpath, tables)
	
####################################################################################
###
### FUNCTION read_legacy_complexities()
###
####################################################################################
def read_legacy_complexities( complexities,
							  timestep_directory,
							  tdata ):
	def __path(type):
		return os.path.join(timestep_directory, 'complexity_' + type + '.txt')

	complexities_read = []
	
	for type in complexities:
		path = __path(type)
		if os.path.isfile(path):
			data = common_complexity.parse_legacy_complexities(path)
			tdata[type] = common_complexity.normalize_complexities(data)
			complexities_read.append(type)

	return complexities_read

####################################################################################
###
### FUNCTION compute_complexities()
###
####################################################################################
def compute_complexities( complexities,
						  timestep,
						  timestep_directory,
						  tdata ):
	def __path(type):
		return os.path.join(timestep_directory, 'complexity_' + type + '.plt')

	# --- Read in any complexities computed on a previous invocation of this script
	complexities_read = []
	if not OverwriteEpochComplexities:
		for type in complexities:
			path = __path(type)

			if os.path.isfile(path):
				try:
					table = datalib.parse(path)[type]
					data = table.getColumn('Complexity').data
					tdata[type] = common_complexity.normalize_complexities(data)
	
					complexities_read.append(type)
				except datalib.InvalidFileError, e:
					# file must have been incomplete
					print "Failed reading ", path, "(", e, ") ... regenerating"
	
	    
	complexities_remaining = list_difference(complexities, complexities_read)
	
	print "  AlreadyHave =", complexities_read
	print "  ComplexitiesToGet =", complexities_remaining
	
	# --- Compute complexities not already found on the file system
	if complexities_remaining:
		farm.status( 'CalcComplexity [%s at %s]' % (timestep, datetime.datetime.now()) )

		# --- Execute CalcComplexity on all brainFunction files in the timestep dir
		query = os.path.join(timestep_directory, "brainFunction*.txt")
		brainFunction_files = abstractfile.ls([query], returnConcrete = False)
		brainFunction_files.sort(lambda x, y: cmp(int(os.path.basename(x)[14:-4]),
							  int(os.path.basename(y)[14:-4])))

		if len(brainFunction_files) == 0:
			err('No brainfunction files found in %s' % timestep_directory)

		cmd="%s brainfunction --bare --list %s -- %s" % ( CALC_COMPLEXITY,
								  ' '.join(brainFunction_files),
								  ' '.join(complexities_remaining))
		cmd = shlex.split(cmd)
		proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
		stdout = proc.communicate()[0]
		complexity_all = filter(lambda x: len(x), stdout.split('\n'))
		if proc.returncode != 0:
			err('Failed executing CalcComplexity, exit=%d' % proc.returncode)
		proc.stdout.close()
	
		colnames = ['AgentNumber', 'Complexity']
		coltypes = ['int', 'float']
		tables = dict([(type, datalib.Table(type, colnames, coltypes))
			       for type in complexities_remaining])
	
		# --- Parse the output
		for line in complexity_all:
			fields = line.split()
			agent_number = fields.pop(0)

			for type in complexities_remaining:
				table = tables[type]
				row = table.createRow()
	
				row.set('AgentNumber', agent_number)
				row.set('Complexity', fields.pop(0))
	
		# --- Write to file and normalize data (eg sort and remove 0's)
		for type in complexities_remaining:
			table = tables[type]
	
			datalib.write(__path(type),
				      table)
	
			data = table.getColumn('Complexity').data
	
			tdata[type] = common_complexity.normalize_complexities(data)

	return complexities

####################################################################################
###
### FUNCTION divide(numerator, denominator)
###
####################################################################################
def divide(numerator, denominator):
	if denominator == 0:
		return 0.0
	else:
		return numerator / denominator

####################################################################################
###
### FUNCTION make_percents()
###
####################################################################################
def make_percents(tables, totals):
	for table in tables.values():
		for row in table.rows():
			t = row.get('Timestep')
			total = totals[t][table.name]

			for bin in range(NUMBINS):
				row.mutate(bin, lambda x: 100 * divide(x, total))

####################################################################################
###
### FUNCTION compute_bins()
###
####################################################################################
def compute_bins(DATA, timesteps, complexities, AVR, minfunc, maxfunc):
	totals = dict([(t, {}) for t in timesteps])
	
	colnames = ['Timestep'] + [x for x in range(NUMBINS)]
	coltypes = ['int'] + ['int' for x in range(NUMBINS)]
	tables = {}
	
	for type in complexities:
		table_bins = datalib.Table(type, colnames, coltypes)
		tables[type] = table_bins
	
		table_avr = AVR[type]
	
		for row_avr in table_avr.rows():
			t = row_avr.get('Timestep')
			minimum = minfunc(row_avr)
			maximum = maxfunc(row_avr)
	
			row_bins = table_bins.createRow()
			row_bins.set('Timestep', t)
			for bin in range(NUMBINS): row_bins.set(bin, 0)
	
			data = DATA[t][type]
	
			totals[t][type] = 0
	
			if(maximum > minimum):
				for complexity in data:
					bin = int(((complexity - minimum)/(maximum - minimum))*NUMBINS)
					if bin >= NUMBINS:
						bin -= 1
					row_bins.mutate(bin, lambda x: x + 1)
	
					totals[t][type] += 1
	
	make_percents(tables, totals)

	return tables
			
####################################################################################
###
### FUNCTION show_usage()
###
####################################################################################
def show_usage(msg = None):
################################################################################
	print"""\
USAGE

  %s [<options>]... <directory>...

DESCRIPTION

     Analyzes the complexities of the epochs contained in one or more recent
  directories.

     <directory> can specify either a run directory or a parent of one or more
  run directories.

    The results of the analysis are written to AvrComplexity.plt, 
  ComplexityHistNorm.plt, and ComplexityHistRaw.plt in the appropriate recent
  directory of each respective run directory. The generated files contain
  statistics on the complexity at each timestep.

OPTIONS

     -C <C>[,<C>]...
               Override default NeuralComplexity types for analysis, where C can
            be a composite of types (e.g. HB). Multiple C specs are separated by
            commas (e.g. -C A,P,I,B,H,HB).  <C> must be uppercase.
            
            Lowercase letters indicate event filters to be applied to neural
            activation prior to computing C.  Currently acceptable values are
            'm'ate and 'e'at.  (E.g., -C Am,Pe,Pme.)
            
            Appended digits indicate the number of points to use in integrating
            C between the I(X)(k/N) and <I(X_k)> curves.  A value of zero
            indicates all points are to be used, thus producing full TSE
            complexity.  No digits or a value of one indicate just the N-1
            point should be used, thus producing the traditional "simplified"
            TSE complexity.  (E.g., -C A0,P1,HB15.)
            (default %s)

     -O
            Overwrite/recompute epoch complexity files if they exist.
     
     -r <recent_type>
               Specify recent directory type. Valid values:
%s

            (default %s)

     -l, --legacy (force|prefer|off)
               force: Do not perform complexity calculations, but
            instead use results from run of older version of this script.

               prefer: Use results from older version if available,
            otherwise perform complexity calculations.

               off: Do not use any legacy files.

            (default off)

            Note: Legacy file paths are of the form
             <recent_directory>/<epoch>/complexity_<C>.txt.

     -b, --bins <n>
               Specify number of bins used in histogram calculations.
	    (default %d)""" % (sys.argv[0],
			       ','.join(map( lambda x: x + '1', common_complexity.DEFAULT_COMPLEXITIES )),
			       ''.join(map( lambda x: '\n                  ' + x, common_functions.RECENT_TYPES )) ,
			       DEFAULT_RECENT,
			       common_complexity.DEFAULT_NUMBINS)

        if msg:
            print "--------------------------------------------------------------------------------"
            print
            print 'Error!', msg
	sys.exit(1)

####################################################################################
###
### Primary Code Path
###
####################################################################################

exit_value = main()

sys.exit(exit_value)
