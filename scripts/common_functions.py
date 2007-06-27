#!/usr/bin/python
import os, sys


# Return a value from the worldfile parameter.  If it is not found the value is False
def read_worldfile_parameter( worldfilename, parameter ):
#	print "worldfilename='%s'  parameter='%s'" % ( worldfilename, parameter )
	value = False

	if not os.path.isfile( worldfilename ):
		print "ERROR: '%s' was not a worldfile"
		sys.exit(1)
	worldfile = open( worldfilename )

	for line in worldfile:
		line = line.rstrip('\n')
		if line.endswith(parameter):
			value = line.split()[0]
			break

	worldfile.close()
	return value

