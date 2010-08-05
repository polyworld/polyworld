#!/usr/bin/env python

import datalib
import iterators

def main():
	#test__misc()
	#test__append()
	test__version3()
	#test__stream()

def print_table(t):
	print t.name
	data = []
	colnames = t.colnames
	for name in colnames:
		print name,
		data.append(t.getColumn(name).data)
	print
	for i in range(len(t.rows())):
		for j in range(len(colnames)):
			print data[j][i],
		print
	print

def print_tables(tables):
	for table in tables.values():
		print_table(table)
	print '--'

def print_datalib(path):
	print_tables( datalib.parse(path) )

#############################################################
###
### This test code was developed along with support for
### streamed parsing.
###
#############################################################
def test__stream():
	colnames = ['ID', 'separation']
	coltypes = ['int', 'float']

	t1 = datalib.Table('1', colnames, coltypes)
	t2 = datalib.Table('2', colnames, coltypes)

	def __addrow(table, id, separation):
		row = table.createRow()
		row['ID'] = id
		row['separation'] = separation

	__addrow(t1, '1', .01)
	__addrow(t1, '2', .02)
	__addrow(t2, '11', .11)
	__addrow(t2, '12', .12)

	datalib.write( 'stream.plt', [t1,t2] )

	def stream_row( row ):
		print row['ID'], row['separation']

	datalib.parse( 'stream.plt', stream_row = stream_row )

	

#############################################################
###
### This test code was developed along with support for
### version 3.
###
#############################################################
def test__version3():
	colnames = ['ID', 'separation']
	coltypes = ['int', 'float']

	t1 = datalib.Table('1', colnames, coltypes)
	t2 = datalib.Table('2', colnames, coltypes)
	t3 = datalib.Table('3', colnames, coltypes)

	def __addrow(table, id, separation):
		row = table.createRow()
		row['ID'] = id
		row['separation'] = separation

	__addrow(t1, '2', .02)
	__addrow(t1, '3', .03)

	__addrow(t2, '3', .04)

	datalib.write( 'version3__table_fixed.plt', [t1,t2,t3] )
	datalib.write( 'version3__single_none.plt', [t1,t2,t3], randomAccess=False, singleSchema=True)

	tables1 = datalib.parse( 'version3__table_fixed.plt', keycolname = 'ID', tablename2key = int )
	tables2 = datalib.parse( 'version3__single_none.plt', keycolname = 'ID', tablename2key = int )

	assert( tables1[1][2]['separation'] == .02 )
	assert( tables1[1][3]['separation'] == .03 )
	assert( tables1[2][3]['separation'] == .04 )

	assert( tables2[1][2]['separation'] == .02 )
	assert( tables2[1][3]['separation'] == .03 )
	assert( tables2[2][3]['separation'] == .04 )
	

#############################################################
###
### This test code was developed along with the 'append'
### functionality.
###
#############################################################
def test__append():
	print '-----------------------------------'
	
	t1 = datalib.Table('V1', ['Time', 'mean'], ['int', 'float'])
	t2 = datalib.Table('V2', ['Time', 'mean'], ['int', 'float'])
	
	for i in range(5):
		row = t1.createRow()
		row['Time'] = i
		row['mean'] = float(i*10)
		row = t2.createRow()
		row['Time'] = i
		row['mean'] = float(i*100)
	
	datalib.write('test.plt', [t1, t2])
	
	print_datalib('test.plt')
	
	#--
	
	t3 = datalib.Table('V3', ['Time', 'mean'], ['int', 'float'])
	for i in range(5):
		row = t3.createRow()
		row['Time'] = i
		row['mean'] = float(i*1000)
	
	datalib.write('test.plt', t3, append=True)
	
	print_datalib('test.plt')
	
	#--
	
	t4 = datalib.Table('V2', ['Time', 'mean'], ['int', 'float'])
	for i in range(5):
		row = t4.createRow()
		row['Time'] = i
		row['mean'] = i*0.1
	
	datalib.write('test.plt', t4, append=True)
	
	print_datalib('test.plt')
	
	#--
	
	t4 = datalib.Table('V2', ['Time', 'mean'], ['int', 'float'])
	for i in range(5):
		row = t4.createRow()
		row['Time'] = i
		row['mean'] = float(i*100)
	
	datalib.write('test.plt', t4, append=True, replace=False)
	
	print_datalib('test.plt')
	
	#--
	
	t4 = datalib.Table('V4', ['Time', 'mean'], ['int', 'float'])
	for i in range(5):
		row = t4.createRow()
		row['Time'] = i
		row['mean'] = i*0.1
	
	datalib.write('test.plt', t4)
	
	print_datalib('test.plt')
	

#############################################################
###
### This is just a hodge-podge of test code
###
#############################################################
def test__misc():
	print '-----------------------------------'
	
	COLNAMES = ['T', 'A', 'B', 'C']
	COLTYPES = ['int', 'int', 'int', 'int']
	
	t = datalib.Table('test', COLNAMES, COLTYPES, keycolname = 'T')
	
	row = t.createRow()
	
	row.set('T', 0)
	row['A'] = 10
	row.set('B', 11)
	row.set('C', 12)
	
	row = t.createRow()
	row['T'] = 1
	row['A'] = 20
	row['B'] = 21
	row['C'] = 22
	
	print t[0]['A']
	print t[1]['A']
	
	it = iterators.MatrixIterator(t, range(2), ['A','B'])
	for a in it:
	    print a
	
	datalib.write('/tmp/datalib1', t)
	
	print '-----------------------------------'
	
	table = datalib.Table(name='Example 2',
	                      colnames=['Time','A','B'],
	                      coltypes=['int','float','float'],
	                      keycolname='Time')
	row = table.createRow()
	row['Time'] = 1
	row['A'] = 100.0
	row['B'] = 101.0
	row = table.createRow()
	row['Time'] = 10001
	row['A'] = 200.0
	row['B'] = 201.0

	it = iterators.MatrixIterator(table, range(1,10002,10000), ['B'])
	for a in it:
	    print a
	
	datalib.write('/tmp/datalib2', table)
	
	tables = datalib.parse('/tmp/datalib2', keycolname = 'Time')
	
	table = tables['Example 2']
	print 'key=',table.keycolname
	
	print tables['Example 2'][1]['A']

main()
