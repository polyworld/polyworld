#!/usr/bin/env python

import sys

while True:
	header = sys.stdin.readline()
	if header == "exit\n":
		break

	assert( header == "<expr>\n" )

	expr = []
	while True:
		line = sys.stdin.readline()
		if line == "</expr>\n":
			break
		else:
			expr.append(line)

	expr = ''.join(expr)

	try:
		result = str( eval(expr) )
		sys.stdout.write( "S%10d%s" % (len(result), result) )
	except:
		msg = str(sys.exc_info()[1])
		sys.stdout.write( "F%10d%s" % (len(msg), msg) )

	sys.stdout.flush()
	
