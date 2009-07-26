import plotlib

COMPLEXITY_TYPES = ['A', 'P', 'I', 'B', 'H']
COMPLEXITY_NAMES = {'A':'All', 'P':'Processing', 'I':'Input', 'B':'Behavior', 'H':'Health'}
DEFAULT_COMPLEXITIES = ['A', 'P', 'I', 'B', 'HB']
FILENAME_AVR = 'AvrComplexity.plt'
DEFAULT_NUMBINS = 11

####################################################################################
###
### FUNCTION get_name()
###
####################################################################################
def get_name(type):
    result = []

    for c in type:
        result.append(COMPLEXITY_NAMES[c])

    return '+'.join(result)

####################################################################################
###
### FUNCTION get_names()
###
####################################################################################
def get_names(types):
    return map(lambda x: get_name(x), types)

####################################################################################
###
### FUNCTION normalize_complexities()
###
####################################################################################
def normalize_complexities(data):
    data = map(float, data)
    # ignore 0.0, since it is a critter that was ignored in the complexity
    # calculation due to short lifespan.
    #
    # also, must be sorted
    data = filter(lambda x: x != 0.0, data)
    data.sort()
    return data

####################################################################################
###
### FUNCTION parse_legacy_complexities()
###
####################################################################################
def parse_legacy_complexities(path):

    f = open(path, 'r')

    complexities = []

    for line in f:
        complexities.append(float(line.strip()))

    f.close()

    return complexities

