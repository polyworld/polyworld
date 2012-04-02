#!/usr/bin/env python

import getopt
import os
import shutil
import sys

from common_functions import err, get_cmd_stdout, isrundir, pw_env

DEBUG = False


################################################################################
################################################################################
###
### FUNCTION main()
###
################################################################################
################################################################################
def main():
    
    argv = sys.argv[1:]

    if len(argv) == 0:
        usage()

    mode = argv[0]

    if mode == "conv":
        shell_conv( argv[1:] )
    elif mode == "edit":
        shell_edit( argv[1:] )
    else:
        err( "invalid mode (%s)" % mode )

####################################################################################
###
### FUNCTION get_overlay_parameter_names()
###
####################################################################################
def get_overlay_parameter_names( path_worldfile_or_run, assume_run_dir = True ):
    if assume_run_dir:
        # do automatic conversion
        path_proplib = ensure_proplib_worldfile( path_worldfile_or_run )
    else:
        path_proplib = path_worldfile_or_run

    path_overlay = os.path.join( os.path.dirname(path_proplib),
                                 'parms.wfo' )

    if not os.path.exists(path_overlay):
        return set()

    lines = proputil( 'scalarnames', path_overlay, 3 ).split()
    return set( lines )

####################################################################################
###
### FUNCTION get_parameter()
###
####################################################################################
def get_parameter( path_worldfile_or_run, parameter, assume_run_dir = True ):
    if assume_run_dir:
        # do automatic conversion
        path_proplib = ensure_proplib_worldfile( path_worldfile_or_run )
    else:
        path_proplib = path_worldfile_or_run

    return proputil( '-w', 'get', '-s', path_schema(), path_proplib, parameter )

####################################################################################
###
### FUNCTION get_parameter_len()
###
####################################################################################
def get_parameter_len( path_worldfile_or_run, parameter, assume_run_dir = True ):
    if assume_run_dir:
        # do automatic conversion
        path_proplib = ensure_proplib_worldfile( path_worldfile_or_run )
    else:
        path_proplib = path_worldfile_or_run

    return int(proputil( '-w', 'len', '-s', path_schema(), path_proplib, parameter ))

####################################################################################
###
### FUNCTION edit()
###
####################################################################################
def edit( path, edit_specs, path_new = None ):
    result = proputil( '-w', 'set', '-s', path_schema(), path, *edit_specs )

    if path_new == None:
        path_new = path

    __tofile( path_new, result )
 
####################################################################################
###
### FUNCTION ensure_proplib_worldfile()
###
####################################################################################
def ensure_proplib_worldfile( path_worldfile_or_run ):
    if not os.path.isdir( path_worldfile_or_run ):
        path_run = os.path.dirname( path_worldfile_or_run )
    else:
        path_run = path_worldfile_or_run

    if not isrundir( path_worldfile_or_run ):
        err( "ensure_proplib_worldfile can only operate within a run directory" )

    path_proplib = path_worldfile(path_run)

    if not os.path.exists( path_proplib ):
        path_legacy = path_worldfile( path_run, legacy = True )

        if not os.path.exists( path_legacy ):
            err( "Cannot locate proplib or legacy worldfile in rundir " + path_run )

        exitval, stdout = get_cmd_stdout( ['wfutil.py', 'conv', '-l', path_legacy] )
        if exitval != 0:
            err( "Cannot convert worldfile in rundir " + path_run )

        __tofile( '/tmp/converted.wf', stdout )

        if not os.path.exists( path_schema() ):
            err( "Cannot find schema. Expected at '%s'" % path_schema() )

        __tofile( path_proplib, proputil('-w', 'apply', path_schema(), '/tmp/converted.wf') )

        shutil.copy( path_schema(), path_run )

    return path_proplib

################################################################################
################################################################################
###
### FUNCTION path_schema()
###
################################################################################
################################################################################
def path_schema():
    return os.path.join( pw_env('home'), 'etc', 'worldfile.wfs' )

####################################################################################
###
### FUNCTION path_worldfile()
###
####################################################################################
def path_worldfile(path_run, legacy = False):
    if legacy:
	return os.path.join(path_run, "worldfile")
    else:
	return os.path.join(path_run, "normalized.wf")

####################################################################################
###
### FUNCTION path_default_worldfile()
###
####################################################################################
def path_default_worldfile():
    return os.path.join(pw_env('home'), 'current.wf')

################################################################################
################################################################################
###
### FUNCTION shell_conv()
###
################################################################################
################################################################################
def shell_conv( argv ):
    legacyMode = False

    shortopts = 'l'
    longopts = []
    try:
        opts, args = getopt.getopt(argv, shortopts, longopts)
    except getopt.GetoptError, e:
        usage( e )

    for opt, value in opts:
        opt = opt.strip('-')

        if opt == 'l':
            legacyMode = True

    if len(args) < 1:
        usage( "missing worldfile argument" )
        

    if len(args) == 1:
        worldfile = parsePreProplib( args[0] )
        normalizePreProplib( worldfile, legacyMode )
        printProplibFormat( worldfile )
    else:
        usage()

################################################################################
################################################################################
###
### FUNCTION shell_edit()
###
################################################################################
################################################################################
def shell_edit( args ):
    if len(args) < 2:
        usage()

    path_worldfile = args[0]
    edit_specs = args[1:]

    result = edit( path_worldfile, edit_specs )

################################################################################
################################################################################
###
### FUNCTION proputil()
###
### Execute the proputil application.
###
################################################################################
################################################################################
def proputil( *args ):
    proputil = pw_env( 'proputil' )

    cmd = [proputil] + list(args)

    exitval, stdout = get_cmd_stdout( cmd )
    
    if exitval != 0:
        err( "Failed executing '%s'. exit=%s" % (cmd,exitval) )

    return stdout.strip()

################################################################################
################################################################################
###
### FUNCTION __tofile()
###
### write a string to a file
###
################################################################################
################################################################################
def __tofile( path, content ):
    f = open( path, 'w' )
    f.write( content )
    f.close()

################################################################################
################################################################################
###
### CLASS Property
###
################################################################################
################################################################################
class Property:
    def __init__( self, name, type, value ):
        self.name = name
        self.type = type
        self.value = value

################################################################################
################################################################################
###
### CLASS Container
###
################################################################################
################################################################################
class Container:
    def __init__( self ):
        self.parent = None
        self.props = {}
        self.order = []

    def add( self, name, type, value ):
        if DEBUG:
            print 'parsed', name, ' as ', value

        prop = Property( name, type, value )

        self.props[name] = prop
        self.order.append( prop )
        if prop.type == 'container':
            value.parent = self
            value.name = name

    def remove( self, name ):
        prop = self.props[name]
        del self.props[name]
        self.order.remove( prop )

    def get( self, name ):
        return self.props[ name ]

    def fullname( self, name ):
        if self.parent == None:
            return name

        ancestors = self.parent.fullname( self.name )
        if isinstance( name, int ):
            return "%s[%d]" % (ancestors, name)
        else:
            return "%s.%s" % (ancestors, name)

    def isarray( self ):
        return all( map(lambda x: isinstance(x, int), self.props) )

    def printOldFormat( self ):
        for prop in self.order:
            value = prop.value
            name = prop.name
            if prop.type == 'container':
                value.printOldFormat()
            else:
                if isinstance( value, bool ):
                    value = '1' if value else '0'
                elif isinstance( value, list ):
                    value = ' '.join( map(str,value) )
                print "%-30s %s" % ( value, self.fullname(name) )

    def printProplibFormat( self, indent = "" ):
        def __print( name, data ):
            print "%s%-25s %s" % (indent, name, data)

        if self.isarray() and len(self.order) == 0:
            return

        lastprop = self.order[-1]

        for prop in self.order:
            value = prop.value

            if prop.type == 'container':
                if not self.isarray():
                    if value.isarray():
                        open = '['
                        close = ']'
                    else:
                        open = '{'
                        close = '}'
                    #__print( prop.name + " " + open, "" )
                    __print( prop.name +" "+open, "" )

                value.printProplibFormat(indent + "  " )

                if not self.isarray():
                    #__print( close, "" )
                    __print( close, "" )
                else:
                    if prop != lastprop:
                        __print( ",", "" )
            elif prop.type == 'barrierkeyframe':
                __print( "Time", value[0] )
                __print( "X1", value[1] )
                __print( "Z1", value[2] )
                __print( "X2", value[3] )
                __print( "Z2", value[4] )
            elif prop.type == 'color':
                __print( prop.name + " {", "" )
                indent += "  "
                __print( "R", value[0] )
                __print( "G", value[1] )
                __print( "B", value[2] )
                indent = indent[:-2]
                __print( "}", "" )
            else:
                __print( prop.name, prop.value )


################################################################################
################################################################################
###
### FUNCTION printProplibFormat()
###
################################################################################
################################################################################
def printProplibFormat( container ):
    container.printProplibFormat()

################################################################################
################################################################################
###
### FUNCTION normalizePreProplib()
###
################################################################################
################################################################################
def normalizePreProplib( container, legacyMode ):
    wfversion = container.get( 'Version' ).value

    edges = container.get( 'Edges' ).value
    container.remove( 'Edges' )
    wraparound = container.get( 'WrapAround' ).value
    container.remove( 'WrapAround' )
    if edges and not wraparound:
        container.add( 'Edges', 'enum', 'B' )
    else:
        print 'implement edges conversion'
        assert( false )
    
    container.remove( 'NumBarriers' )
    container.remove( 'NumDomains' )

    if wfversion < 47:
        container.add( 'RatioBarrierPositions', 'bool', False )

    barriers = container.get( 'Barriers' )
    for barrier in barriers.value.props.values():
        barrier.value.remove( 'NumKeyFrames' )

    def __fraction( container, worldPropName, subcontainer, subPropName = None, newPropName = None ):
        if not subPropName:
            subPropName = worldPropName
        if not newPropName:
            newPropName = subPropName + 'Fraction'

        worldValue = container.get( worldPropName ).value
        subValue = subcontainer.value.get( subPropName ).value
        fractionValue = float(subValue) / worldValue
        subcontainer.value.remove( subPropName )
        subcontainer.value.add( newPropName, 'float', fractionValue )

    def __move( container, propName, subcontainer, subPropName = None ):
        if subPropName == None:
            subPropName = propName

        prop = container.value.get( propName )
        subcontainer.add( subPropName, prop.type, prop.value )
        container.value.remove( propName )


    domains = container.get( 'Domains' )
    for domain in domains.value.props.values():
        domain.value.remove( 'NumFoodPatches' )
        domain.value.remove( 'NumBrickPatches' )

        foodpatches = domain.value.get( 'FoodPatches' )
        for foodpatch in foodpatches.value.props.values():
            onCondition = Container()
            foodpatch.value.add( 'OnCondition', 'container', onCondition )
            onCondition.add( 'Mode', 'enum', 'Time' )
            time = Container()
            onCondition.add( 'Time', 'container', time )

            __move( foodpatch, 'Period', time )
            __move( foodpatch, 'Phase', time )
            __move( foodpatch, 'OnFraction', time )


            if wfversion < 32:
                def __foodfraction( propName, domainPropName ):
                    value = foodpatch.value.get( propName ).value
                    if value < 0:
                        value *= foodpatch.value.get( 'FoodFraction' ).value
                    __fraction( domain.value, domainPropName, foodpatch, propName, propName + 'Fraction' )

                __foodfraction( 'InitFood', 'InitFood' )
                __foodfraction( 'MinFood', 'MinFood' )
                __foodfraction( 'MaxFood', 'MaxFood' )
                __foodfraction( 'MaxFoodGrown', 'MaxFoodGrownCount' )


        if wfversion < 32:
            __fraction( container, 'InitFood', domain )
            __fraction( container, 'MinFood', domain )
            __fraction( container, 'MaxFood', domain )
            __fraction( container, 'MaxFoodGrown', domain, 'MaxFoodGrownCount', 'MaxFoodGrownFraction' )
            __fraction( container, 'InitAgents', domain )
            __fraction( container, 'MinAgents', domain )
            __fraction( container, 'MaxAgents', domain )
            __fraction( container, 'SeedAgents', domain, 'SeedAgents', 'InitSeedsFraction' )
            


    container.remove( 'Version' )

    container.add( 'LegacyMode', 'bool', legacyMode )

################################################################################
################################################################################
###
### FUNCTION parsePreProplib()
###
################################################################################
################################################################################
def parsePreProplib( path ):
    f = open( path, 'r' )

    def __line():
        while True:
            l = f.readline()

            tokens = l.split()
            n = len(tokens)
            if n == 1:
                err( "unexpected format: '%s'" % l )

            if DEBUG:
                print 'READ LINE:', l

            if n > 0:
                return tokens[-1], tokens[:-1]

    class local:
        container = Container()

    def push( name, childContainer ):
        local.container.add( name, 'container', childContainer )
        local.container = childContainer
        return childContainer

    def pop( name ):
        assert( name == local.container.name )
        local.container = local.container.parent

    wfversion = -1

    def __scalar( name, version, default, type, func_parse ):
        if version == None or version <= wfversion:
            label, val = __line()
            if len(val) != 1:
                err( "expecting scalar for %s, found (%s,%s)" % (name, label, val) )
            val = func_parse( val[0] )

            local.container.add( name, type, val )
        else:
            val = default

        return val

    def __tuple( name, count, version, default, type, func_parse ):
        if version == None or version <= wfversion:
            label, val = __line()
            if len(val) != count:
                err( "expecting tuple with %d elements for %s, found (%s,%s)" % (count, name, label, val) )
            val = map( func_parse, val )

            local.container.add( name, type, val )
        else:
            val = default


        return val

    def pint( name, version = None, default = None ):
        return __scalar(name, version, default, 'int', int)

    def pfloat( name, version = None, default = None ):
        return __scalar(name, version, default, 'float', float)

    def pbool( name, version = None, default = None ):
        return __scalar(name, version, default, 'bool', lambda x: x != '0' )

    def penum( name, values, version = None, default = None, values_new = None ):
        if values_new == None:
            values_new = values
        type = 'enum(' + ','.join(values_new) + ')'
        val =  __scalar(name, version, default, type, lambda x: values_new[values.index(x)] )
        assert( val in values_new )
        
        return val

    def pcolor( name, version = None, default = None ):
        return __tuple( name, 3, version, default, 'color', float )

    def pbarrierkeyframe( name ):
        label, val = __line()
        if len(val) != 5:
            err( "expecting barrierkeyframe with 5 elements for %s, found (%s,%s)" % (name, label, val) )
        val = [int(val[0])] + map( float, val[1:] )

        local.container.add( name, 'barrierkeyframe', val )

        return val

    def pbarrierpos( name ):
        label, val = __line()
        if len(val) != 4:
            err( "expecting barrierkeyframe with 4 elements for %s, found (%s,%s)" % (name, label, val) )
        val = map( float, val )

        local.container.add( name, 'barrierpos', val )

        return val

    def pignore( name ):
        __line()

            

    try:
        wfversion = pint( 'Version' )
    except:
        wfversion = 0

    if (wfversion < 1) or (wfversion > 55):
        err( "Invalid pre-proplib worldfile (didn't find version on first line)" )

    pbool( 'PassiveLockstep', 25, False )
    pint( 'MaxSteps', 18, 0 )
    pbool( 'EndOnPopulationCrash', 48, False )
    pignore( 'DoCpuWork' )
    pint( 'CheckPointFrequency' )
    pint( 'StatusFrequency' )
    pbool( 'Edges' )
    pbool( 'WrapAround' )
    pbool( 'Vision' )
    pbool( 'ShowVision' )
    pbool( 'StaticTimestepGeometry', 34, False )
    pbool( 'ParallelInitAgents', 55, False )
    pbool( 'ParallelInteract', 55, False )
    pbool( 'ParallelBrains', 55, True )
    pint( 'RetinaWidth' )
    pfloat( 'MaxVelocity' )
    pint( 'MinAgents' )
    pint( 'MaxAgents' )
    pint( 'InitAgents' )
    pint( 'SeedAgents', 9, 0 )
    pfloat( 'SeedMutationProbability', 9, 0.0 )
    pbool( 'SeedGenomeFromRun', 45, False )
    pbool( 'SeedPositionFromRun', 49, False )
    pint( 'MiscegenationDelay' )

    if wfversion >= 32:
        pint( 'InitFood' )
        pint( 'MinFood' )
        pint( 'MaxFood' )
        pint( 'MaxFoodGrown' )
        pfloat( 'FoodGrowthRate' )

    pfloat( 'FoodRemoveEnergy', 46, 0.0 )
    pint( 'PositionSeed' )
    pint( 'InitSeed' )
    pint( 'SimulationSeed', 44, 0 )
    
    if wfversion < 32:
        pint( 'MinFood' )
        pint( 'MaxFood' )
        pint( 'MaxFoodGrown' )
        pint( 'InitFood' )
        pfloat( 'FoodGrowthRate' )

    penum( 'AgentsAreFood', ['0','1','F'], values_new = ['False','True', 'Fight'] )
    pint( 'EliteFrequency' )
    pint( 'PairFrequency' )
    pint( 'NumberFittest' )
    pint( 'NumberRecentFittest', 14, 10 )
    pfloat( 'FitnessWeightEating' )
    pfloat( 'FitnessWeightMating' )
    pfloat( 'FitnessWeightMoving' )
    pfloat( 'FitnessWeightEnergyAtDeath' )
    pfloat( 'FitnessWeightLongevity' )
    pfloat( 'MinFoodEnergy' )
    pfloat( 'MaxFoodEnergy' )
    pfloat( 'FoodEnergySizeScale' )
    pfloat( 'FoodConsumptionRate' )
    penum( 'GenomeLayout', ['L','N'], 53, 'L' )
    pbool( 'EnableMateWaitFeedback', 48, False )
    pbool( 'EnableSpeedFeedback', 38, False )
    pbool( 'EnableGive', 38, False )
    pbool( 'EnableCarry', 39, False )
    pint( 'MaxCarries', 39, 0 )
    pbool( 'CarryAgents', 41, True )
    pbool( 'CarryFood', 41, True )
    pbool( 'CarryBricks', 41, True )
    pbool( 'ShieldAgents', 41, False )
    pbool( 'ShieldFood', 41, False )
    pbool( 'ShieldBricks', 41, False )
    pfloat( 'CarryPreventsEat', 41, 0.0 )
    pfloat( 'CarryPreventsFight', 41, 0.0 )
    pfloat( 'CarryPreventsGive', 41, 0.0 )
    pfloat( 'CarryPreventsMate', 41, 0.0 )
    pignore( 'MinInternalNeurons' )
    pignore( 'MaxInternalNeurons' )
    pint( 'MinVisionNeuronsPerGroup' )
    pint( 'MaxVisionNeuronsPerGroup' )
    pfloat( 'MinMutationRate' )
    pfloat( 'MaxMutationRate' )
    pint( 'MinCrossoverPoints' )
    pint( 'MaxCrossoverPoints' )
    pint( 'MinLifeSpan' )
    pint( 'MaxLifeSpan' )
    pint( 'MateWait' )
    pint( 'InitMateWait' )
    pint( 'EatMateWait', 29, 0 )
    pfloat( 'MinAgentStrength' )
    pfloat( 'MaxAgentStrength' )
    pfloat( 'MinAgentSize' )
    pfloat( 'MaxAgentSize' )
    pfloat( 'MinAgentMaxEnergy' )
    pfloat( 'MaxAgentMaxEnergy' )
    pfloat( 'MinEnergyFractionToOffspring' )
    pfloat( 'MaxEnergyFractionToOffspring' )
    pfloat( 'MinMateEnergyFraction' )
    pfloat( 'MinAgentMaxSpeed' )
    pfloat( 'MaxAgentMaxSpeed' )
    pfloat( 'MotionRate' )
    pfloat( 'YawRate' )
    pfloat( 'MinLearningRate' )
    pfloat( 'MaxLearningRate' )
    pfloat( 'MinHorizontalFieldOfView' )
    pfloat( 'MaxHorizontalFieldOfView' )
    pfloat( 'VerticalFieldOfView' )
    pfloat( 'MaxSizeFightAdvantage' )
    penum( 'BodyGreenChannel', ['I','L','$float'], 38, 'I' )
    penum( 'NoseColor', ['L','$float'], 38, 'L' )
    pfloat( 'DamageRate' )
    pfloat( 'EnergyUseEat' )
    pfloat( 'EnergyUseMate' )
    pfloat( 'EnergyUseFight' )
    pfloat( 'MinSizeEnergyPenalty', 47, 0 )
    pfloat( 'MaxSizeEnergyPenalty' )
    pfloat( 'EnergyUseMove' )
    pfloat( 'EnergyUseTurn' )
    pfloat( 'EnergyUseLight' )
    pfloat( 'EnergyUseFocus' )
    pfloat( 'EnergyUsePickup', 39, 0.5 )
    pfloat( 'EnergyUseDrop', 39, 0.001 )
    pfloat( 'EnergyUseCarryAgent', 39, 0.05 )
    pfloat( 'EnergyUseCarryAgentSize', 39, 0.05 )
    pfloat( 'EnergyUseCarryFood', 39, 0.01 )
    pfloat( 'EnergyUseCarryBrick', 39, 0.01 )
    pfloat( 'EnergyUseSynapses' )
    pfloat( 'EnergyUseFixed' )
    pfloat( 'SynapseWeightDecayRate' )
    pfloat( 'AgentHealingRate', 18, 0.0 )
    pfloat( 'EatThreshold' )
    pfloat( 'MateThreshold' )
    pfloat( 'FightThreshold' )
    pfloat( 'FightMultiplier', 43, 1.0 )
    pfloat( 'GiveThreshold', 38, 0.2 )
    pfloat( 'GiveFraction', 38, 1.0 )
    pfloat( 'PickupThreshold', 39, 0.5 )
    pfloat( 'DropThreshold', 39, 0.5 )
    pfloat( 'MiscegenationFunctionBias' )
    pfloat( 'MiscegenationFunctionInverseSlope' )
    pfloat( 'LogisticSlope' )
    pfloat( 'MaxSynapseWeight' )
    pbool( 'EnableInitWeightRngSeed', 52, False )
    pint( 'MinInitWeightRngSeed', 52, 0 )
    pint( 'MaxInitWeightRngSeed', 52, 0 )
    pfloat( 'MaxSynapseWeightInitial' )
    pfloat( 'MinInitialBitProb' )
    pfloat( 'MaxInitialBitProb' )
    pbool( 'SolidAgents', 31, False )
    pbool( 'SolidFood', 31, False )
    pbool( 'SolidBricks', 31, True )
    pfloat( 'AgentHeight' )
    pfloat( 'FoodHeight' )
    pcolor( 'FoodColor' )
    pfloat( 'BrickHeight', 47 )
    pcolor( 'BrickColor', 47, [0.6,0.2,0.2] )
    pfloat( 'BarrierHeight' )
    pcolor( 'BarrierColor' )
    pcolor( 'GroundColor' )
    pfloat( 'GroundClearance' )
    pcolor( 'CameraColor' )
    pfloat( 'CameraRadius' )
    pfloat( 'CameraHeight' )
    pfloat( 'CameraRotationRate' )
    pfloat( 'CameraAngleStart' )
    pfloat( 'CameraFieldOfView' )
    pint( 'MonitorAgentRank' )
    pignore( 'MonitorCritWinWidth' )
    pignore( 'MonitorCritWinHeight' )
    pint( 'BrainMonitorFrequency' )
    pfloat( 'WorldSize' )

    ###
    ### Barriers
    ###
    numBarriers = pint( 'NumBarriers' )
    pbool( 'RatioBarrierPositions', 47, False )

    barriers = push( 'Barriers', Container() )
    for i in range(numBarriers):
        barrier = push( i, Container() )

        if( wfversion >= 27 ):
            numKeyFrames = pint( 'NumKeyFrames' )
            keyFrames = push( 'KeyFrames', Container() )
            for j in range(numKeyFrames):
                push( j, Container() )
                pbarrierkeyframe( j )
                pop( j )
            pop( 'KeyFrames' )
        else:
            pbarrierpos( j )

        pop( i )
    pop( 'Barriers' )

    if wfversion < 2:
        return local.container

    pbool( 'MonitorGeneSeparation' )
    pbool( 'RecordGeneSeparation' )

    if wfversion < 3:
        return local.container

    pbool( 'ChartBorn' )
    pbool( 'ChartFitness' )
    pbool( 'ChartFoodEnergy' )
    pbool( 'ChartPopulation' )
    
    ###
    ### Domains
    ###
    numDomains = pint( 'NumDomains' )

    push( 'Domains', Container() )

    for i in range(numDomains):
        domain = push( i, Container() )

        if wfversion < 32:
            pint( 'MinAgents' )
            pint( 'MaxAgents' )
            pint( 'InitAgents' )
            pint( 'SeedAgents', 9, 0 )
            pfloat( 'ProbabilityOfMutatingSeeds', 9, 0.0 )

        if wfversion >= 19:
            pfloat( 'CenterX' )
            pfloat( 'CenterZ' )
            pfloat( 'SizeX' )
            pfloat( 'SizeZ' )
        
        if wfversion >= 32:
            pfloat( 'MinAgentsFraction' )
            pfloat( 'MaxAgentsFraction' )
            pfloat( 'InitAgentsFraction' )
            pfloat( 'InitSeedsFraction' )
            pfloat( 'ProbabilityOfMutatingSeeds' )
            pfloat( 'InitFoodFraction' )
            pfloat( 'MinFoodFraction' )
            pfloat( 'MaxFoodFraction' )
            pfloat( 'MaxFoodGrownFraction' )
            pfloat( 'FoodRate' )

            numFoodPatches = pint( 'NumFoodPatches' )
            numBrickPatches = pint( 'NumBrickPatches' )
        else:
            numFoodPatches = pint( 'NumFoodPatches' )
            numBrickPatches = pint( 'NumBrickPatches' )
            pfloat( 'FoodRate' )
            pint( 'InitFood' )
            pint( 'MinFood' )
            pint( 'MaxFood' )
            pint( 'MaxFoodGrownCount' )

        foodPatches = push( 'FoodPatches', Container() )

        for j in range( numFoodPatches ):
            push( j , Container() )

            pfloat( 'FoodFraction' )

            if wfversion >= 32:
                pfloat( 'InitFoodFraction' )
                pfloat( 'MinFoodFraction' )
                pfloat( 'MaxFoodFraction' )
                pfloat( 'MaxFoodGrownFraction' )
                pfloat( 'FoodRate' )
            else:
                pfloat( 'FoodRate' )
                pint( 'InitFood' )
                pint( 'MinFood' )
                pint( 'MaxFood' )
                pint( 'MaxFoodGrown' )
            
            pfloat( 'CenterX' )
            pfloat( 'CenterZ' )
            pfloat( 'SizeX' )
            pfloat( 'SizeZ' )
            penum( 'Shape', ['0','1'], values_new = ['R','E'] )
            penum( 'Distribution', ['0','1','2'], values_new = ['U','L','G'] )
            pfloat( 'NeighborhoodSize' )
            
            pint( 'Period', 26, 0 )
            pfloat( 'OnFraction', 26, 0.0 )
            pfloat( 'Phase', 26, 0.0 )
            pbool( 'RemoveFood', 26, False )

            pop( j )

        pop( 'FoodPatches' )
                

        brickPatches = push( 'BrickPatches', Container() )

        for j in range( numBrickPatches ):
            push( j, Container() )

            pfloat( 'CenterX' )
            pfloat( 'CenterZ' )
            pfloat( 'SizeX' )
            pfloat( 'SizeZ' )
            pint( 'BrickCount' )
            penum( 'Shape', ['0','1'], values_new = ['R','E'] )
            penum( 'Distribution', ['0','1','2'], values_new = ['U','L','G'] )
            pfloat( 'NeighborhoodSize' )

            pop( j )

        pop( 'BrickPatches' )
        
        pop( i )
    pop( 'Domains' )

    pbool( 'ProbabilisticFoodPatches', 17, False )

    if wfversion < 5:
        return local.container

    pbool( 'ChartGeneSeparation' )

    if wfversion < 6:
        return local.container

    penum( 'NeuronModel', ['F','S','T'], 36, 'F' )

    pfloat( 'TauMin', 50, 0.01 )
    pfloat( 'TauMax', 50, 1.0 )
    pfloat( 'TauSeed', 50, 1.0 )

    pint( 'MinInternalNeuralGroups' )
    pint( 'MaxInternalNeuralGroups' )
    pint( 'MinExcitatoryNeuronsPerGroup' )
    pint( 'MaxExcitatoryNeuronsPerGroup' )
    pint( 'MinInhibitoryNeuronsPerGroup' )
    pint( 'MaxInhibitoryNeuronsPerGroup' )
    pignore( 'MinBias' )
    pfloat( 'MaxBiasWeight' )
    pfloat( 'MinBiasLrate' )
    pfloat( 'MaxBiasLrate' )
    pfloat( 'MinConnectionDensity' )
    pfloat( 'MaxConnectionDensity' )
    pfloat( 'MinTopologicalDistortion' )
    pfloat( 'MaxTopologicalDistortion' )
    pbool( 'EnableTopologicalDistortionRngSeed', 50, False )
    pint( 'MinTopologicalDistortionRngSeed', 50, 0 )
    pint( 'MaxTopologicalDistortionRngSeed', 50, 255 )
    pfloat( 'EnergyUseNeurons' )
    pint( 'PreBirthCycles' )
    pfloat( 'SeedFightBias', 40, 0.5 )
    pfloat( 'SeedFightExcitation', 40, 1.0 )
    pfloat( 'SeedGiveBias', 40, 0.0 )

    if wfversion >= 39:
        pfloat( 'SeedPickupBias' )
        pfloat( 'SeedDropBias' )
        pfloat( 'SeedPickupExcitation' )
        pfloat( 'SeedDropExcitation' )

    pint( 'OverHeadRank' )
    pbool( 'AgentTracking' )
    pfloat( 'MinFoodEnergyAtDeath' )
    pbool( 'GrayCoding' )

    if wfversion < 7:
        return local.container

    penum( 'SmiteMode', ['L','R','O'], 21 )
    pfloat( 'SmiteFrac' )
    pfloat( 'SmiteAgeFrac' )

    pint( 'NumDepletionSteps', 23, 0 )
    pbool( 'ApplyLowPopulationAdvantage', 24, False )

    if wfversion < 8:
        return local.container

    pbool( 'RecordBirthsDeaths', 22, False )
    pbool( 'RecordPosition', 33, False )
    pbool( 'RecordContacts', 38, False )
    pbool( 'RecordCollisions', 38, False )
    pbool( 'RecordCarry', 42, False )

    pbool( 'BrainAnatomyRecordAll', 11, False )
    pbool( 'BrainFunctionRecordAll', 11, False )
    pbool( 'BrainAnatomyRecordSeeds', 12, False )
    pbool( 'BrainFunctionRecordSeeds', 12, False )
    pint( 'BestSoFarBrainAnatomyRecordFrequency', 11, False )
    pint( 'BestSoFarBrainFunctionRecordFrequency', 11, False )
    pint( 'BestRecentBrainAnatomyRecordFrequency', 14, False )
    pint( 'BestRecentBrainFunctionRecordFrequency', 14, False )
    pbool( 'RecordGeneStats', 12, False )
    pbool( 'RecordPerformanceStats', 35, False )
    pbool( 'RecordFoodPatchStats', 15, False )
    
    if wfversion >= 18:
        pbool( 'RecordComplexity' )
        if wfversion < 28:
            # this property is a bit odd in that it dramatically changed rules between versions.
            # this should probably be moved into the normalize function
            __scalar( 'ComplexityType', None, None, 'enum', lambda x: 'O' if x == '0' else 'P' )
        else:
            penum( 'ComplexityType',
                   ['O', 'P','A','I','B','D','Z'],
                   values_new = ['O', 'P','A','I','B','D','Z'] )
        if wfversion >= 30:
            pfloat( 'ComplexityFitnessWeight' )
            pfloat( 'HeuristicFitnessWeight' )
    
    pbool( 'RecordGenomes', 37, False )
    pbool( 'RecordSeparations', 38, False )
    pbool( 'RecordAdamiComplexity', 20, False )
    pint( 'AdamiComplexityRecordFrequency', 20, 0 )

    pbool( 'CompressFiles', 54, False )

    if wfversion >= 18:
        penum( 'FogFunction', ['E','L','O'] )
        pfloat( 'ExpFogDensity' )
        pint( 'LinearFogEnd' )

    pbool( 'RecordMovie' )
    
    return local.container

################################################################################
################################################################################
###
### FUNCTION usage()
###
################################################################################
################################################################################
def usage( msg = None ):
    print """\
usage: wfutil conv [-l] worldfile

     Converts a pre-proplib worldfile to be compatible with the current schema.

   Result is written to stdout.
 
   -l    Enable LegacyMode, which should result in a worldfile that produces
       the same results as the old worldfile. If this is not specified, then
       schema default values will be adopted for properties that were
       introduced since the original worldfile was created, which will likely
       change simulation results.

--

usage: wfutil edit worldfile (propname=value)+

     Set one or more properties.

   Result is written to original file.
"""

    if msg != None:
        print msg

    exit( 1 )

################################################################################
################################################################################
###
### INVOKE MAIN
###
################################################################################
################################################################################
if __name__ == "__main__":
    main()
