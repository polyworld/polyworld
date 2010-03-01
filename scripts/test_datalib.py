#!/usr/bin/python

import datalib
import iterators

COLNAMES = ['T', 'A', 'B', 'C']
COLTYPES = ['int', 'int', 'int', 'int']

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

def print_datalib(path):
	tables = datalib.parse(path)
	for table in tables.values():
		print_table(table)
	print '--'

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
exit()
#############################################################

print '-----------------------------------'

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
row['Time'] = 2
row['A'] = 200.0
row['B'] = 201.0

it = iterators.MatrixIterator(table, range(1,3), ['B'])
for a in it:
    print a

datalib.write('/tmp/datalib2', table)

tables = datalib.parse('/tmp/datalib2', keycolname = 'Time')

table = tables['Example 2']
print 'key=',table.keycolname

print tables['Example 2'][1]['A']
