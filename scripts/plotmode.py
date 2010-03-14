import common_complexity
import common_functions
import common_metric
import common_motion
import common_stats


####################################################################################
###
### CLASS Mode
###
####################################################################################
class Mode:
    def __init__( self,
                  name,
                  datasets,
                  defaultDataset,
                  isAvr,
                  values,
                  defaultValues,
                  curvetypes,
                  defaultCurvetype,
                  colName_timestep,
                  func_relpath,
                  func_pathRunFromValue,
                  func_parseValues,
                  func_getValueNames,
                  func_normalizeValueNames):
        self.name = name
        self.datasets = datasets
        self.defaultDataset = defaultDataset if defaultDataset else 'defaultDataset'
        self.isAvr = isAvr
        self.values = values
        self.defaultValues = defaultValues
        self.curvetypes = curvetypes
        self.defaultCurvetype = defaultCurvetype
        self.colName_timestep = colName_timestep
        self.func_relpath = func_relpath
        self.func_pathRunFromValue = func_pathRunFromValue
        self.func_parseValues = func_parseValues
        self.func_getValueNames = func_getValueNames
        self.func_normalizeValueNames = func_normalizeValueNames
   
    def __str__( self ):
    	s1 = []
    	
    	return """Mode: name = %s,
                        datasets = %s
                        defaultDataset = %s,
                        isAvr = %s,
                        values = %s
                        defaultValues = %s
                        curvetypes = %s
                        defaultCurvetype = %s,
                        colname_timestep = %s,
                        func_relpath = %s,
                        func_pathRunFromValue = %s,
                        func_parseValues = %s,
                        func_getValueNames = %s\n""" % (
                        self.name,
                        str([x for x in self.datasets]),
                        self.defaultDataset,
                        str(self.isAvr),
                        str([x for x in self.values]),
                        str([x for x in self.defaultValues]),
                        str([str(x) for x in self.curvetypes]),
                        str(self.defaultCurvetype),
                        str(self.colName_timestep),
                        str(self.func_relpath),
                        str(self.func_pathRunFromValue),
                        str(self.func_parseValues),
                        str(self.func_getValueNames) )

    def relpath( self,
                 classification,
                 dataset ):
        return self.func_relpath( classification,
                                  dataset )

    def pathRunFromValue( self,
                          path,
                          classification,
                          dataset ):
        return self.func_pathRunFromValue( path,
                                           classification,
                                           dataset )

    def parseValues( self,
                     run_paths,
                     classification,
                     dataset,
                     value_names,
                     run_as_key ):
        return self.func_parseValues( run_paths,
                                      classification,
                                      dataset,
                                      value_names,
                                      run_as_key )

    def normalizeValueNames( self,
                             classification,
                             value_names ):
        if self.func_normalizeValueNames == None:
            return valueNames
        else:
            return self.func_normalizeValueNames(classification, value_names)

    def usage( self ):
        #
        # Usage
        #
        print """\
USAGE

     plot %s [<option>]... <directory>...
""" % (self.shortname)

        #
        # Options Header
        #
        opts = False
        print """\
OPTIONS
     (use 'help opts' to see general options)\
"""

        #
        # -d
        #
        if len(self.datasets) > 1:
            opts = True

            print """
     -d <dataset>
            Valid datasets:\
%s
            Note that datasets may be abbreviated.
            (default %s)""" % ('\n                  '.join( [''] + self.datasets ),
                               self.defaultDataset)

        #
        # -v
        #
        if len(self.values) > 1:
            opts = True

            print """
     -v <v>[,<v>]..."""
            print self.usage_values

        #
        # Avr Flags
        #
        if self.isAvr:
            opts = True

            print """
     -m/M, --mean/Mean
               Plot mean/meta-mean of each value (-v) type. (default -m on)

     -n/N, --min/Min
               Plot max/meta-max of each value (-v) type.

     -x/X, --max/Max
               Plot max/meta-max of each value (-v) type.

     -e/E, --err/Err
               Draw Standard Error bars for the mean/meta-mean.

     -w/W, --whiskers/Whiskers
               Box-and-Whiskers plot of run/meta-average.
"""

        #
        # Single-curvetype Flags
        #
        if len(self.curvetypes) == 1:
            opts = True

            print """
     -m/M
               Plot data/meta-data. (default -m on)
"""
            

        #
        # No Mode Options
        #
        if not opts:
            print """
     No mode-specific options.
"""
            

####################################################################################
###
### VAR MODES
###
####################################################################################
MODES = {}

MODES['neuralComplexity'] = Mode( 'Neural Complexity',
                                  common_functions.RECENT_TYPES,
                                  'Recent',
                                  True,
                                  common_complexity.COMPLEXITY_TYPES,
                                  'P',
                                  ['min', 'mean', 'max'],
                                  'mean',
                                  'Timestep',
                                  common_complexity.relpath_avr,
                                  common_complexity.path_run_from_avr,
                                  common_complexity.parse_avrs,
                                  common_complexity.get_names,
                                  None)

MODES['graphMetric'] = Mode( 'Graph Metric',
							 common_functions.RECENT_TYPES,
							 'Recent',
                             True,
                             common_metric.METRIC_TYPES,
                             'cc_a_bu',
                             ['min', 'mean', 'max'],
                             'mean',
                             'Timestep',
                             common_metric.relpath_avr,
                             common_metric.path_run_from_avr,
                             common_metric.parse_avrs,
                             common_metric.get_names,
                             common_metric.normalize_metrics_names)

MODES['motionComplexity'] = Mode( 'Motion',
                                  [],
                                  None,
                                  False,
                                  common_motion.COMPLEXITY_TYPES,
                                  'Olaf',
                                  ['COMPLEXITY'],
                                  'COMPLEXITY',
                                  'EPOCH-START',
                                  common_motion.relpath_complexity,
                                  common_motion.path_run_from_complexity,
                                  common_motion.parse_complexity,
                                  common_motion.get_names,
                                  None)

MODES['stats'] = Mode( 'Stats',
                       [],
                       None,
                       False,
                       common_stats.STAT_TYPES,
                       'agents',
                       ['value'],
                       'value',
                       'step',
                       lambda: 'stats/stat.1',
                       common_stats.path_run_from_stats,
                       common_stats.parse_stats,
                       common_stats.get_names,
                       None)

for shortname, mode in MODES.items():
    mode.shortname = shortname

MODES['neuralComplexity'].usage_values = """\
               Plot NeuralComplexity across one or more 'v' neuron types,
            where v can be a composite of types (e.g. HB). Multiple v specs are
            separated by commas (e.g. -v P,HB,I).
            (default P)"""

MODES['graphMetric'].usage_values = """\
              Plot Graph theoretical metrics of one or more types 'v',
           where v is one of:
               CC - Clustering Coefficient
               SP - (Normalized) Shortest Path length"""

MODES['stats'].usage_values = """\
              Plot statistics of one or more types 'v',
           where v is one of:\
%s""" % ('\n               '.join( [''] + common_stats.STAT_TYPES ))


####################################################################################
###
### FUNCTION ismode
###
####################################################################################
def ismode( name ):
    return expandmode( name ) != None

####################################################################################
###
### FUNCTION expandmode
###
####################################################################################
def expandmode( name ):
    try:
        return common_functions.expand_abbreviations( name,
                                                      MODES.keys(),
                                                      case_sensitive = False )
    except common_functions.IllegalAbbreviationError, x:
        return None
