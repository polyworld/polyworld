import os
import re
import sys


################################################################################
###
### FUNCTION init_env
###
################################################################################
def init_env(env):
	env.Append( LIBPATH = ['/usr/local/lib',
						   '/usr/lib'] )

	if PFM == 'linux':
		env.Append( DEFAULTFRAMEWORKPATH = [] )
		env.Append( FRAMEWORKPATH = [] )
		env.Append( LIBPATH = ['/usr/lib/x86_64-linux-gnu', '/usr/lib/i386-linux-gnu'] )
	elif PFM == 'mac':
		env.Append( DEFAULTFRAMEWORKPATH = ['/System/Library/Frameworks',
											'/Library/Frameworks'] )
		env.Append( FRAMEWORKPATH = [] )

        addlib(env, 'pthread')
        addlib(env, 'dl')

	return env

################################################################################
###
### FUNCTION import_OpenGL
###
################################################################################
def import_OpenGL(env):
	env.Append( CPPPATH = [which('gl.h',
								 dir = True,
								 path = ['/usr/include/GL',
										 '/System/Library/Frameworks/OpenGL.framework/Versions/A/Headers',
										 '/System/Library/Frameworks/AGL.framework/Versions/A/Headers/'],
								 err = 'Cannot locate OpenGL')] )

	if PFM == 'linux':
		addlib(env, 'GL')
		addlib(env, 'GLU')
	elif PFM == 'mac':
		addlib(env, 'AGL')
		addlib(env, 'OpenGL')

################################################################################
###
### FUNCTION import_OpenMP
###
################################################################################
def import_OpenMP(env):
	env.Append( LIBS = 'gomp' )
	env.Append( CPPFLAGS = '-fopenmp' )

################################################################################
###
### FUNCTION import_GSL
###
################################################################################
def import_GSL(env):
	addlib(env, 'gsl')
	addlib(env, 'gslcblas')

################################################################################
###
### FUNCTION import_zlib
###
################################################################################
def import_zlib(env):
	addlib(env, 'z')

################################################################################
###
### FUNCTION import_python
###
################################################################################
def import_python(env, version=None):
	if PFM == 'linux':
		if not version: version='2.7'
		env.Append( CPPPATH = ['/usr/include/python'+version] )
	elif PFM == 'mac':
		if not version: version='2.7'
		env.Append( CPPPATH = ['/System/Library/Frameworks/Python.framework/Versions/'+version+'/include/python'+version] )
	else:
		assert(False)

	addlib(env, 'python'+version)

################################################################################
###
### FUNCTION import_Qt
###
################################################################################
def import_Qt(env,
			  qtmodules = ['QtCore']):

	def expand(path, module = ""):
		return map(lambda p: p.replace('$MODULE', module).replace('$VERSION',str(qtversion)),
				   path)

	###
	### Include Path
	###
	if PFM == 'linux':
		qthome = None
		PFM_CPPPATH = ['/usr/share/qt$VERSION/include/$MODULE']
		qtversion = 4

		env.Append( CPPPATH = expand(['/usr/share/qt$VERSION/include/']) )
	elif PFM == 'mac':
		qthome = os.path.expanduser( '~/QtSDK/Desktop/Qt/4.8.1/gcc' )
		if os.path.exists( qthome ):
			PFM_CPPPATH = [qthome + '/include/$MODULE']
			qtversion = 4
		else:
			qthome = os.path.expanduser( '~/Qt5.2.0/5.2.0/clang_64' )
			if os.path.exists( qthome ):
				PFM_CPPPATH = [qthome + '/lib/$MODULE.framework/Headers']
				qtversion = 5
			else:
				qthome = None
				PFM_CPPPATH = ['/Library/Frameworks/$MODULE.framework/Versions/$VERSION/Headers']
				qtversion = 4
	else:
		assert(false)

	if qtversion >= 5:
		qtmodules += ['QtWidgets']
	for mod in qtmodules:
		env.AppendUnique( CPPPATH = [which(mod,
										   dir = True,
										   path = expand(PFM_CPPPATH, mod),
										   err = 'Cannot locate ' + mod)] )

	###
	### Libraries
	###
	if PFM == 'linux':
		env.Append( LIBPATH = expand(['/usr/share/qt$VERSION/lib']) )

	for mod in qtmodules:
		if qthome:
			addlib( env, mod, path = [qthome + '/lib'] )
		else:
			addlib( env, mod )

	env.Replace( QT_LIB = None )

	###
	### Executables Path
	###
	if qthome:
		env.Replace( QT_BINPATH = [qthome + '/bin'] )
	else:
		env.Replace( QT_BINPATH = [which('moc',
										 dir = True,
										 err = "Cannot locate Qt executables")] )

	###
	### Misc
	###
	env.Replace( QTDIR = '/usr' ) # dummy. not needed since we explictly configure.
	env.Tool('qt')

	return env

################################################################################
###
### FUNCTION export_dynamic
###
################################################################################
def export_dynamic( env ):
	if PFM == 'linux':
		env.Append( LINKFLAGS = ['-rdynamic'] )
	elif PFM == 'mac':
		env.Append( LINKFLAGS = ['-undefined', 'dynamic_lookup'] )
	else:
		assert( False )

################################################################################
###
### FUNCTION find
###
################################################################################
def find(topdir = '.',
		 type = None,
		 name = None,
		 regex = None,
		 ignore = ['CVS'] ):

	assert((name == None and regex == None)
		   or (name != None) ^ (regex != None))
	if type:
		assert(type in ['file', 'dir'])

	result = []

	if(name):
		# convert glob syntax to regex
		pattern = name.replace('.','\\.').replace('*','.*') + '$'
	elif(regex == None):
		pattern = '.*'
	else:
		pattern = regex

	cregex = re.compile(pattern)

	for dirinfo in os.walk(topdir):
		dirname = dirinfo[0]
		if type == None:
			children = dirinfo[1] + dirinfo[2]
		elif type == 'dir':
			children = dirinfo[1]
		elif type == 'file':
			children = dirinfo[2]
		else:
			assert(false)

		for filename in children:
			if filename in ignore:
				continue

			path = os.path.join(dirname, filename)

			if cregex.match(filename):
			   result.append(path)

			if os.path.isdir(path) and os.path.islink(path):
				result += find(path,
							   type,
							   name,
							   regex)

	return result

################################################################################
###
### FUNCTION which
###
################################################################################
def which(name,
		  dir = False,
		  path = os.environ['PATH'].split(':'),
		  err = None):

	for a_path in path:
		p = os.path.join(a_path, name)
		if os.path.exists(p):
			abspath = os.path.abspath(p)
			if dir:
				return os.path.dirname(abspath)
			else:
				return abspath
	if err:
		print err
		sys.exit(1)
	else:
		return None

################################################################################
###
### FUNCTION exclude
###
################################################################################
def exclude(dirs, exclude):
	def test(dir):
		for x in exclude:
			if dir.startswith(x):
				return False
		return True

	return filter(test, dirs)

################################################################################
###
### FUNCTION relocate_paths
###
################################################################################
def relocate_paths(sources, oldtop, newtop):
	n = len(str(oldtop))

	return map(lambda path: str(newtop) + '/' + path[n:],
			   sources)

################################################################################
###
### FUNCTION libsuffixes
###
################################################################################
def libsuffixes():
	if PFM == 'linux':
		return ['.so', '.a']
	elif PFM == 'mac':
		return ['.dylib', '.so', '.a']
	else:
		assert(false)

################################################################################
###
### FUNCTION libprefix
###
################################################################################
def libprefix():
	if PFM == 'linux' or PFM == 'mac':
		return 'lib'
	else:
		assert(false)

################################################################################
###
### FUNCTION addlib
###
################################################################################
def addlib(env, libname, path = None):
	if not path:
		path = env['LIBPATH'] + env['FRAMEWORKPATH'] + env['DEFAULTFRAMEWORKPATH']

	for dir in path:
		if os.path.exists( os.path.join(dir, libname + '.framework') ):
				env.Append( FRAMEWORKS = [libname] )
				if dir not in env['DEFAULTFRAMEWORKPATH']:
					env.AppendUnique( FRAMEWORKPATH = [dir] )
				return
		else:
			for libsuffix in libsuffixes():
				lib = libprefix() + libname + libsuffix

				if os.path.exists( os.path.join(dir, lib) ):
					env.Append( LIBS = [libname] )
					env.AppendUnique( LIBPATH = [dir])
					return

	print 'Failed locating library', libname
	sys.exit(1)

################################################################################
###
### FUNCTION uname
###
################################################################################
def uname():
	type = os.popen('uname').readlines()[0].strip()

	if type.startswith('Darwin'):
		return 'mac'
	elif type.startswith('Linux'):
		return 'linux'
	else:
		assert(false)

PFM = uname()
