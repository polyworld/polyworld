#!/usr/bin/env python

import __builtin__
import getopt
import gzip
import os
import re
import sys
import tempfile

from common_functions import err

################################################################################
###
### FUNCTION main()
###
### entry point when called from shell    
###
################################################################################
def main():
    argv = sys.argv[1:]

    if len(argv) < 1:
        show_usage( "Must specify mode" )

    mode = argv[0]

    mode_argv = argv[1:]

    if mode == 'ls':
        exitval = shell_ls( mode_argv )
    elif mode == 'ln':
        exitval = shell_ln( mode_argv )
    elif mode == 'tail':
        exitval = shell_tail( mode_argv )
    else:
        show_usage( 'Invalid mode (%s)' % mode )

    if exitval == None:
        exitval = 0
    elif exitval != 0:
        exitval = 1
    
    return exitval


################################################################################
###
### FUNCTION open()
###
### Given an abstract path, open a concrete file.
###
### RESULT: file object for I/O
###
################################################################################
def open( apath, mode = 'r' ):
    afile = resolve_apath( apath )
    if afile == None:
        err( "Cannot locate abstract file %s" % apath )
    if afile.isAmbiguous():
        err( "ambiguous abstract file %s" % apath )

    if afile.type == 'gzip':
        return gzip.GzipFile( afile.cpath, mode )
    else:
        return __builtin__.open( afile.cpath, mode )


################################################################################
###
### FUNCTION exists()
###
### Given an abstract path, check for existence of concrete file.
###
### RESULT: bool
###
################################################################################
def exists( apath ):
    return resolve_apath( apath ) != None


################################################################################
###
### FUNCTION ls()
###
### Return list of paths that match one or more queries.
###
### PARAM queries: list of strings, where each string is either a path
###   or a wildcard. e.g.: ['run/foo/*.txt', 'run/foo/bar.plt']. queries should
###   be in form of abstract paths.
###
### PARAM returnConcrete: Whether result should be abstract or concrete paths.
###
### RESULT: List of path strings
###
################################################################################
def ls( queries, returnConcrete = True ):
    for query in queries:
        if os.path.isdir( query ):
            query = os.path.join( query, '*' )

        if '*' in query:
            afiles = aglob( query )
        else:
            afile = resolve_apath(query)
            if afile == None:
                err( "no such file: %s" % query )
            afiles = [afile]

        for afile in afiles:
            if afile.isAmbiguous():
                err( "multiple concrete files for %s" % afile.apath )

        return map( lambda x: x.cpath if returnConcrete else x.apath,
                    afiles )


################################################################################
###
### FUNCTION shell_ls()
###
################################################################################
def shell_ls( argv ):
    class Opts:
        printConcrete = True

    shortopts = 'a'
    longopts = []

    try:
        opts, args = getopt.getopt(argv, shortopts, longopts)
    except getopt.GetoptError, e:
        show_usage( e )

    for opt, value in opts:
        opt = opt.strip('-')

        if opt == 'a':
           Opts.printConcrete = False

    queries = args

    paths = ls( queries, Opts.printConcrete )

    for path in paths:
        print path

################################################################################
###
### FUNCTION shell_ln()
###
################################################################################
def shell_ln( argv ):
    queries = argv

    if len(argv) != 2:
        err( 'ln requires 2 args' )

    srcapath = argv[0]
    dstapath = argv[1]

    src = resolve_apath( srcapath )

    if src == None:
        err( 'cannot locate src file %s' % srcapath )
    if src.isAmbiguous():
        err( 'ambigous src file %s' % src.apath )

    if os.path.isdir( dstapath ):
        dstapath = os.path.join( dstapath, src.basename() )

    dst = resolve_apath( dstapath )

    if dst != None:
        err( 'dst file already exists: %s' % dst.apath )

    dst = AbstractFile( src.type, dstapath )

    os.link( src.cpath, dst.cpath )


################################################################################
###
### FUNCTION shell_tail()
###
################################################################################
def shell_tail( argv ):
    if len(argv) < 1:
        err( "tail requires a file arg." )

    opts = argv[:-1]
    apath = argv[-1]

    afile = resolve_apath( apath )

    if afile == None:
        err( "cannot locate file %s" % apath )

    if afile.type == 'gzip':
        # we want to do zcat | tail, but I don't know of a shell-agnostic
        # way to get the exit value of the zcat. bash has PIPESTATUS...
        tmp = '%s/abstractfile.tail.%s' % (tempfile.gettempdir(), os.getpid())
        cmd = 'zcat "%s" > "%s"' % (afile.cpath, tmp)
        exitval = os.system( cmd )
        if exitval == 0:
            cmd = 'tail %s "%s"' % (' '.join(opts), tmp)
            exitval = os.system( cmd )
            os.remove( tmp )
    else:
        cmd = 'tail %s "%s"' % (' '.join(opts), afile.cpath)
        exitval = os.system( cmd )

    return exitval

################################################################################
###
### FUNCTION aglob()
###
### "Abstract Glob" - Evaluate wildcard query, where query is in terms of
### abstract paths.
###
### RESULT: list of AbstractFile
###
################################################################################
def aglob( query ):
    dir = os.path.dirname( query )
    wildcard_pattern = os.path.basename( query )

    if '*' in dir:
        err( 'Wildcards in directories not yet supported' )

    regex_pattern = ('^' + re.escape(wildcard_pattern) + '$').replace('\\*', '.*' )

    afiles = []

    for cpath in os.listdir( dir ):
        afile = resolve_cpath( os.path.join(dir, cpath) )

        if re.match( regex_pattern, afile.basename() ):
            afiles.append( afile )

    return afiles

################################################################################
###
### FUNCTION resolve_apath()
###
### Given an abstract path, create an AbstractFile
###
### RESULT: AbstractFile. None if no concrete file exists.
###
################################################################################
def resolve_apath( apath ):
    if os.path.exists( apath ):
        return AbstractFile( 'file', apath )
    else:
        pathgz = apath + '.gz'
        if os.path.exists( pathgz ):
            return AbstractFile( 'gzip', apath )

    return None

################################################################################
###
### FUNCTION resolve_cpath()
###
### Given a concrete path, create an AbstractFile
###
### RESULT: AbstractFile. None if no concrete file exists.
###
################################################################################
def resolve_cpath( cpath ):
    if not os.path.exists( cpath ):
        return None

    if cpath.endswith( '.gz' ):
        return AbstractFile( 'gzip', cpath[:-3] )
    else:
        return AbstractFile( 'file', cpath );

################################################################################
###
### CLASS AbstractFile
###
### Represents a file. Contains info about type, abstract path, concrete path...
###
################################################################################
class AbstractFile:
    def __init__( self,
                  type,
                  apath ):
        self.type = type
        self.apath = apath

        if type == 'gzip':
            self.cpath = apath + '.gz'
        else:
            self.cpath = apath

    def exists( self ):
        return os.path.exists( self.cpath )

    def basename( self ):
        return os.path.basename( self.apath )

    def isAmbiguous( self ):
        n = 0

        if os.path.exists( self.apath ):
            n += 1
        if os.path.exists( self.apath + '.gz' ):
            n += 1

        return n > 1

    def __str__( self ):
        return '[%s, %s]' % (self.type, self.apath)

####################################################################################
###
### FUNCTION show_usage()
###
####################################################################################
def show_usage(msg = None):
################################################################################
	print"""\
USAGE

  %s <mode> [<mode_opts>]... <mode_args>...

MODES
  
  ls [<opts>]... <file>...

     -a   Output abstract names.

  ln <src_file> <dst_file_or_dir>

  tail [<standard tail opts>]... <file>
""" % ( os.path.basename(sys.argv[0]) )
        if msg:
            print "--------------------------------------------------------------------------------"
            print
            print 'Error!', str(msg)
	sys.exit(1)


################################################################################
###
### Invoke main() if called from shell
###
if __name__ == "__main__":
    exitval = main()

    sys.exit( exitval )
