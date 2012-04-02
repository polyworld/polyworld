import os
import re
import sys

import datalib
import glob


####################################################################################
###
### FUNCTION parse_stats
###
####################################################################################
def parse_stats( path_run, types = None ):
	paths = glob.glob( os.path.join(path_run, 'stats', 'stat.*') )
	paths.sort( lambda x, y: __path2step(x) - __path2step(y) )
	
	tables = {}
	for path in paths:
		__add_step( tables, path, types )

	if types and set(tables.keys()) != set(types):
		raise datalib.MissingTableError( "Cannot find: " + ", ".join(set(types).difference( set(tables.keys()) )) )

	return tables


####################################################################################
###
### FUNCTION __add_step
###
####################################################################################
def __add_step( tables, path, labels ):
	step = __path2step( path )

	for label, type, value in __parse_file( path ):
		if label == 'step':
			assert( int(value) == step )
			continue

		if labels and label not in labels:
			continue

		try:
			table = tables[label]
		except KeyError:
			colnames = ['step', 'value']
			coltypes = ['int', type]

			tables[label] = table = datalib.Table( label,
												   colnames,
												   coltypes,
												   keycolname = 'step' )
		table = tables[label]
		row = table.createRow()
		row['step'] = step
		row['value'] = value


####################################################################################
###
### FUNCTION __parse_file
###
####################################################################################
def __parse_file( path ):
	regex_label = r'-?[a-zA-Z\.\[\]0-9]+'
	regex_number = r'-?[0-9]+(?:\.[0-9]+)?'
	regex_equals = r'\s*(%s)\s*=\s*(%s)(\s*$|\s+[^0-9])' % (regex_label, regex_number)

	major = None

	def __type(number):
		if '.' in number:
			return 'float'
		else:
			return 'int'

	for line in open( path ):
		line = line.strip()

		result = re.match( regex_equals, line )
		if result:
			label = result.group(1)
			number = result.group(2)
			type = __type(number)

			if label[0] == '-':
				label = major + label
			else:
				major = label

			yield label, type, number
		else:
			fields = line.split()

			if fields[0] == 'Domain':
				domain_id = fields[1]
			else:
				result = re.match( r'FP([0-9]+|\*)', fields[0] )
				if result:
					fp_id = result.group(1)

					for i in range(1, len(fields)):
						label = 'Domain[%s]FP[%s][%s]' % (domain_id, fp_id, i-1)
						number = fields[i]
						type = __type( number )

						yield label, type, number


####################################################################################
###
### FUNCTION __path2step
###
####################################################################################
def __path2step( path ):
    return int(path[ path.rfind('.') + 1 : ])
