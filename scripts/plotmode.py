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
                  func_getValueNames ):
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

    def relpath( self, dataset ):
        if len(self.datasets):
            return self.func_relpath( dataset )
        else:
            return self.func_relpath()

    def pathRunFromValue( self, path, dataset ):
        if len(self.datasets):
            return self.func_pathRunFromValue( path, dataset )
        else:
            return self.func_pathRunFromValue( path )

    def parseValues( self, run_paths, dataset, value_names, run_as_key ):
        if len(self.datasets):
            return self.func_parseValues( run_paths,
                                          dataset,
                                          value_names,
                                          run_as_key )
        else:
            return self.func_parseValues( run_paths,
                                          value_names,
                                          run_as_key )

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
        # -D
        #
        if len(self.datasets) > 1:
            opts = True

            print """
     -D <dataset>
            Valid datasets:\
%s
            Note that datasets may be abbreviated.
            (default %s)""" % ('\n                  '.join( [''] + self.datasets ),
                               self.defaultDataset)

        #
        # -V
        #
        if len(self.values) > 1:
            opts = True

            print """
     -V <V>[,<V>]..."""
            print self.usage_values

        #
        # Avr Flags
        #
        if self.isAvr:
            opts = True

            print """
     -m/M, --mean/Mean
               Plot mean/meta-mean of each value (-V) type. (default -m on)

     -n/N, --min/Min
               Plot max/meta-max of each value (-V) type.

     -x/X, --max/Max
               Plot max/meta-max of each value (-V) type.

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
                                  common_complexity.get_names )

MODES['graphMetric'] = Mode( 'Graph Metric',
							 common_functions.RECENT_TYPES,
							 'Recent',
                             True,
                             common_metric.METRIC_TYPES,
                             'SP',
                             ['min', 'mean', 'max'],
                             'mean',
                             'Timestep',
                             common_metric.relpath_avr,
                             common_metric.path_run_from_avr,
                             common_metric.parse_avrs,
                             common_metric.get_names )

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
                                  common_motion.get_names )

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
                       common_stats.get_names )

for shortname, mode in MODES.items():
    mode.shortname = shortname

MODES['neuralComplexity'].usage_values = """\
               Plot NeuralComplexity across one or more 'V' neuron types,
            where V can be a composite of types (e.g. HB). Multiple V specs are
            separated by commas (e.g. -V P,HB,I).
            (default P)"""

MODES['graphMetric'].usage_values = """\
              Plot Graph theoretical metrics of one or more types 'V',
           where V is one of:
               CC - Clustering Coefficient
               SP - (Normalized) Shortest Path length"""

MODES['stats'].usage_values = """\
              Plot statistics of one or more types 'V',
           where V is one of:\
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
