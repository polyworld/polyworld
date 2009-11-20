#!/usr/bin/python

import datalib
import iterators

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

datalib.write('/tmp/datalib', table)

tables = datalib.parse('/tmp/datalib', keycolname = 'Time')

table = tables['Example 2']
print 'key=',table.keycolname

print tables['Example 2'][1]['A']
