#!/usr/bin/env python

import datalib
import sys

def main():
	argv = sys.argv[1:]

	if not len(argv):
		usage()

	path_in = argv[0]
	path_out = argv[1]
	tablename = argv[2]

	clauses = parse_clauses( argv[3:] )

	table = datalib.parse( path_in, [tablename] )[tablename]

	for args in clauses:
		mode = args[0]
		if mode == 'sort':
			table = dosort( table, args[1:] )
		elif mode == 'rowfilter':
			table = dorowfilter( table, args[1:] )
		else:
			print 'invalid mode:', mode
			sys.exit( 1 )

	datalib.write( path_out, [table] )

def parse_clauses( argv ):
	clauses = [[]]
	clause = clauses[0]

	for arg in argv:
		if arg == ':':
			clause = []
			clauses.append( clause )
		else:
			clause.append( arg )

	return clauses

def dosort( table, args ):
	colnames = args
	assert( len(colnames) )

	def __cmp( a, b ):
		for colname in colnames:
			x = a[colname]
			y = b[colname]
			if x < y:
				return -1
			elif x > y:
				return 1

		return 0

	table.sortrows( __cmp )
	return table

def dorowfilter( table, args ):
	rowfilter = args[0]
	def __rowfilter( row ):
		return eval(rowfilter)

	table_filtered = datalib.Table( table.name, table.colnames, table.coltypes )

	for row in table.rows():
		if __rowfilter( row ):
			row_filtered = table_filtered.createRow()
			for col in table.colnames:
				row_filtered[col] = row[col]

	del table
	return table_filtered

def usage():
	print """\
usage: datautil.py PATH_IN PATH_OUT TABLENAME MODE MODE_ARGS... [ : MODE MODE_ARGS ]...

modes:
       sort COLNAME...
       rowfilter EXPR

Examples:

  Show only the first 100 agents, ordered by DeathReason and Agent ID:

       datautil.py run/lifespans.txt ./output LifeSpans rowfilter "row['Agent'] <= 100" : sort DeathReason Agent
"""

	sys.exit( 1 )

main()
