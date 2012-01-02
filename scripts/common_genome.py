import os
import sys

import abstractfile
import datalib

####################################################################################
###
### CLASS SeparationCache
###
####################################################################################
class SeparationCache:
    def __init__( self, path_run ):
        path_log = os.path.join(path_run, 'genome/separations.txt')

        class state:
            tables = {}

        def __beginTable( tablename, colnames, coltypes, path, table_index, keycolname ):
            agentNumber = int( tablename )
            state.currTable = {}
            state.tables[ agentNumber ] = state.currTable

        def __row( row ):
            state.currTable[ row['Agent'] ] = row[ 'Separation' ]


        datalib.parse( path_log, stream_beginTable = __beginTable, stream_row = __row )

        self.tables = state.tables

    def separation( self, agentNumber1, agentNumber2 ):
        if agentNumber1 < agentNumber2:
            x = agentNumber1
            y = agentNumber2
        else:
            x = agentNumber2
            y = agentNumber1

        return self.tables[x][y]

    # returns min_separation, max_separation
    def getBounds( self ):
        lo = sys.maxint
        hi = -lo

        for table in self.tables.values():
            for separation in table.values():
                lo = min(lo, separation)
                hi = max(hi, separation)

        return lo, hi

####################################################################################
###
### CLASS GeneRange
###
### Obtain via class GenomeSchema
###
####################################################################################
class GeneRange:
    def __init__( self, rounding, minval, maxval ):
        self.rounding = rounding
        self.minval = minval
        self.maxval = maxval

    def interpolate( self, raw ):
        ratio = float(raw) * (1.0 / 255)

        def __interp(x,ylo,yhi):
            return ((ylo)+(x)*((yhi)-(ylo)))

        def __nint(a):
            return int( a + (0.499999999 if a > 0.0 else -0.499999999) )

        if type(self.minval) is float:
            return __interp( ratio, self.minval, self.maxval )
        elif type(self.minval) is int:
            if self.rounding == "IntFloor":
                return int( __interp( ratio, int(self.minval), int(self.maxval) ) )
            elif self.rounding == "IntNearest":
                return __nint( __interp( ratio, int(self.minval), int(self.maxval) ) )
            elif self.rounding == "IntBin":
                return min( int(__interp( ratio, int(self.minval), int(self.maxval) + 1 )),
                            int(self.maxval) );

            else:
                assert( False )
        else:
            assert( False )



####################################################################################
###
### CLASS Genome
###
### Current implementation assumes you're going to look at just one or two genes
### for thousands of genomes... and it therefore opens and seeks in the genome file
### for each gene value you look up. This could cause performance problems. If you
### need to look at a lot of genes, you should maybe enhance this to cache file
### contents. 
###
####################################################################################
class Genome:
    def __init__( self, schema, agentNumber ):
        self.schema = schema
        self.agentNumber = agentNumber

    def getGeneValue( self, geneName ):
        range = self.schema.getRange( geneName )
        raw = self.getRawValue( geneName )

        return range.interpolate( raw )

    def getRawValue( self, geneName ):
        index = self.schema.getIndex( geneName )
        
        genomePath = os.path.join( self.schema.path_run, 'genome/agents/genome_%d.txt' % self.agentNumber )
        f = abstractfile.open( genomePath )

        for i in range(index+1):
            line = f.readline()

        f.close()

        return int( line )
        
####################################################################################
###
### CLASS GenomeSubset
###
####################################################################################
class GenomeSubset:
    def __init__( self, path_run ):
        self.schema = GenomeSchema( path_run )
        self.subsets = datalib.parse( os.path.join(path_run, 'genome/subset.log'), keycolname = 'Agent' )['GenomeSubset']

    def getGeneNames( self ):
        return self.subsets.colnames[1:]

    def getGeneValue( self, agentNumber, geneName ):
        range = self.schema.getRange( geneName )
        raw = self.getRawValue( agentNumber, geneName )

        return range.interpolate( raw )

    def getRawValue( self, agentNumber, geneName ):
        return self.subsets[agentNumber][geneName]


####################################################################################
###
### CLASS GenomeSchema
###
####################################################################################
class GenomeSchema:
    def __init__( self, path_run ):
        self.path_run = path_run
        self.indexes = None
        self.ranges = None

    def getIndex( self, geneName ):
        if self.indexes == None:
            self.indexes = {}
            for line in open( os.path.join( self.path_run, 'genome/meta/geneindex.txt' ) ):
                fields = line.split()
                index = fields[0]
                name = fields[1]

                self.indexes[name] = int(index)

        return self.indexes[geneName]


    def getRange( self, geneName ):
        if self.ranges == None:
            self.ranges = {}
            for line in open( os.path.join( self.path_run, 'genome/meta/generange.txt' ) ):
                fields = line.split()

                def __parseBound( index ):
                    if fields[index] == 'INT':
                        return int( fields[index + 1] )
                    elif fields[index] == 'FLOAT':
                        return float( fields[index + 1] )
                    else:
                        assert( False )

                rounding = fields[0]
                minval = __parseBound( 1 )
                maxval = __parseBound( 3 )
                name = fields[5]

                self.ranges[ name ] = GeneRange( rounding, minval, maxval )

        return self.ranges[geneName]
        

####################################################################################
###
### FUNCTION get_seed_run_chain()
###
### Using information from genomeSeeds.txt in a run directory, construct a list
### of runs from oldest to newest.
###
####################################################################################
def get_seed_run_chain( path_newest ):
    path_curr = path_newest
    paths = []

    while True:
        paths.insert( 0, path_curr )

        path_seeds = os.path.join( path_curr, 'genome/genomeSeeds.txt' )
        if not os.path.exists( path_seeds ):
            break;

        f = open( path_seeds )
        genomepath = f.readline()
        if '/genome/agents/' in genomepath:
            path_curr = os.path.dirname( os.path.dirname(os.path.dirname( genomepath )) )
        else:
            path_curr = os.path.dirname( os.path.dirname(genomepath) )
        
        f.close()

    return paths
        
