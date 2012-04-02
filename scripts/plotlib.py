import os
import re
import sys

import common_functions

####################################################################################
###
### CLASS CurveStyle
###
####################################################################################
class CurveStyle:
    COLORS = ['red', 'green', 'blue', 'orange', 'pink', 'magenta', 'gray40', 'cyan', 'gold', 'purple']

    GRAY = 'gray40'
    BLACK = 'black'
    GRAYSCALE = [GRAY, BLACK]

    DOTTED = 0
    SOLID = 1
    DASHED = 2
    HASHED = 3
    DASHDOT = 5
    LINE_TYPES = [SOLID, DASHED, DASHDOT, HASHED, DOTTED]

    THIN = 1
    MEDIUM = 2
    THICK = 3.5

    def __init__(self, doc, id, linetype, linewidth, color):
        self.doc = doc
        self.id = id
        self.linetype = linetype
        self.linewidth = linewidth
        self.color = color

    def getSpec(self):
        if self.doc.nocolor and not self.color in CurveStyle.GRAYSCALE:
            color = CurveStyle.GRAY
        else:
            color = self.color

        spec = 'set style line %d lt %d lw %f lc rgb "%s"\n' % (self.id,
                                                                self.linetype,
                                                                self.linewidth,
                                                                color)
        return spec

####################################################################################
###
### CLASS Curve
###
####################################################################################
class Curve:
    def __init__(self,
                 title,
                 col_x,
                 col_y,
                 style,
                 smooth = False,
                 points = False):
        assert(col_x.table == col_y.table)
        self.col_x = col_x
        self.col_y = col_y

        self.init(title, col_x.table, style, smooth, points)

    def init(self, title, table, style, smooth = False, points = False):
        self.title = title
        self.table = table
        self.style = style
        self.smooth = smooth
        self.points = points

        self.axes = [1,1]

    def getSpec(self):
        # make sure table is written to a file
        assert(self.table.path)

        spec = "'%s' index %d using %s with %s ls %d" %(self.table.path,
                                                        self.table.index,
                                                        self._spec_using(),
                                                        self._spec_with(),
                                                        self.style.id)
        if self.title:
            spec += " title '%s'" % psencode(self.title)
        else:
            spec += ' notitle'

        if self.smooth:
            spec += ' smooth csplines'

        spec += ' axes x%dy%d' % (self.axes[0], self.axes[1])

        return spec

    def _spec_using(self):
        return '%d:%d' % (self.col_x.index + 1, self.col_y.index + 1)

    def _spec_with(self):
        if self.points:
            return 'linespoints'
        else:
            return 'lines'

####################################################################################
###
### CLASS Tick
###
####################################################################################
class Tick:
    def __init__(self,
                 index,
                 y,
                 label,
                 line,
                 style):
        self.index = index
        self.y = y
        self.label = label
        self.line = line
        self.style = style

    def getSpec(self):
        spec = 'set y2tics '

        if self.index == 0:
            spec += 'out '
        else:
            spec += 'add '

        if self.label == None:
            spec += '("%g" ' %  self.y
        else:
            spec += "('%s' " % psencode(self.label)

        spec += '%g)' % self.y

        return spec

    def getLineSpec(self):
        assert(self.line)

        return '%g axes x1y2 notitle with lines ls %d' % (self.y, self.style.id)
                 
####################################################################################
###
### CLASS CandlestickCurve
###
####################################################################################
class CandlestickCurve(Curve):
    WHISKERS = True

    def __init__(self,
                 title, 
                 col_x,
                 col_opening,
                 col_low,
                 col_high,
                 col_closing,
                 style,
                 whiskers):

        self.cols = [col_x, col_opening, col_low, col_high, col_closing]
        self.whiskers = whiskers

        self.init(title, col_x.table, style)

    def _spec_using(self):
        indices = map(lambda col: str(col.index + 1), self.cols)

        return ':'.join(indices)

    def _spec_with(self):
        if self.whiskers:
            return 'candlesticks whiskerbars'
        else:
            return 'candlesticks'

####################################################################################
###
### CLASS ErrorbarCurve
###
####################################################################################
class ErrorbarCurve(Curve):
    def __init__(self,
                 title,
                 col_x,
                 col_y,
                 col_err,
                 style):
        self.col_x = col_x
        self.col_y = col_y
        self.col_err = col_err

        self.init(title, col_x.table, style)

    def _spec_using(self):
        return '%d:%d:%d' % (self.col_x.index + 1,
                             self.col_y.index + 1,
                             self.col_err.index + 1)

    def _spec_with(self):
        return 'yerrorbars'

####################################################################################
###
### CLASS Plot
###
####################################################################################
class Plot:
    def __init__(self, doc, name):
        self.doc = doc

        self.curves = {}
        self.curvelist = []

        self.ticks = []

        self.title = name
        self.xlabel = None
        self.y1label = None
        self.y2label = None
        self.xrange = None
        self.y1range = None
        self.y2range = None
        self.y2ticks = False
        self.grid = True
        self.boxwidth = 0.5
        self.fill_opacity = 0.3
        self.legend = True

        self.rmargin = 0

        self.curve_styles = []

    def createCurveStyle( self, linetype, linewidth, color ):
        style = CurveStyle( self.doc, len(self.curve_styles) + 1, linetype, linewidth, color )
        self.curve_styles.append( style )
        return style

    def createTick(self,
                   y,
                   label = None,
                   line = False,
                   linetype = CurveStyle.DASHED ):

        tick = Tick(len(self.ticks),
                    y,
                    label,
                    line,
                    self.createCurveStyle( linetype,
                                           CurveStyle.THIN,
                                           CurveStyle.GRAY ) )

        self.ticks.append(tick)

        return tick

    def createCurve( self,
                     table,
                     title,
                     name_col_x,
                     name_col_y,
                     style,
                     smooth = False,
                     points = False ):
        return self.__createCurve(Curve(title,
                                        table.getColumn(name_col_x),
                                        table.getColumn(name_col_y),
                                        style,
                                        smooth,
                                        points))

    def createCandlestickCurve( self,
                                table,
                                title,
                                name_col_x,
                                name_col_opening,
                                name_col_low,
                                name_col_high,
                                name_col_closing,
                                style,
                                whiskers = False  ):
        return self.__createCurve(CandlestickCurve( title,
                                                    table.getColumn(name_col_x),
                                                    table.getColumn(name_col_opening),
                                                    table.getColumn(name_col_low),
                                                    table.getColumn(name_col_high),
                                                    table.getColumn(name_col_closing),
                                                    style,
                                                    whiskers ))

    def createErrorbarCurve(self,
                            table,
                            title,
                            name_col_x,
                            name_col_y,
                            name_col_err,
                            style):
        return self.__createCurve(ErrorbarCurve(title,
                                                table.getColumn(name_col_x),
                                                table.getColumn(name_col_y),
                                                table.getColumn(name_col_err),
                                                style))

    def __createCurve(self, curve):
        self.curvelist.append(curve)
        return curve

    def getSpec(self):
        spec = ''

        if self.grid:
            spec += 'set grid\n'
        else:
            spec += 'unset grid\n'

        if self.legend:
            spec += 'set key outside bottom center horizontal\n'
        else:
            spec += 'unset key\n'

        if self.xlabel:
            spec += "set xlabel '%s' font 'Times,14'\n" % psencode(self.xlabel)
        else:
            spec += 'unset xlabel\n'
        if self.y1label:
            spec += "set ylabel '%s' font 'Times,14'\n" % psencode(self.y1label)
        else:
            spec += 'unset ylabel\n'
        if self.y2label:
            spec += "set y2label \"%s\" font 'Times,14'\n" % psencode(self.y2label)
        else:
            spec += 'unset y2label\n'

        if self.y2ticks:
            spec += 'set y2tics border\n'

        if self.xrange:
            spec += 'set xrange [%f:%f]\n' % (self.xrange[0], self.xrange[1])

        if self.y1range:
            spec += 'set yrange [%f:%f]\n' % (self.y1range[0], self.y1range[1])

        if self.y2range:
            spec += 'set y2range [%f:%f]\n' % (self.y2range[0], self.y2range[1])
        else:
            spec += 'unset y2range\n'
        
        if self.rmargin:
            spec += 'set rmargin %d\n' % self.rmargin
        else:
            spec += 'unset rmargin\n'

        for tick in self.ticks:
            spec += tick.getSpec() + '\n'

        #if self.points:
        #    spec += 'set style data points\n'
        #else:
        #    spec += 'set style data lines\n'
        
        spec += 'set style fill solid %f\n' % self.fill_opacity
        spec += 'set boxwidth %f relative\n' % self.boxwidth

        for style in self.curve_styles:
            spec += style.getSpec()

        if self.title:
            spec += "set title '%s'\n" % psencode(self.title)
            spec += 'show title\n'

        spec += 'plot '

        for i in range(len(self.curvelist)):
            if i > 0:
                spec += ', '
            spec += self.curvelist[i].getSpec()

        if len(self.ticks):
            comma = len(self.curvelist) > 0
            for tick in self.ticks:
                if tick.line:
                    if comma:
                        spec += ', '
                    spec += tick.getLineSpec()
                    comma = True

        return spec;

####################################################################################
###
### CLASS Document
###
####################################################################################
class Document:
    def __init__(self):
        self.layout = None

        self.plots = {}
        self.plotlist = []

        self.nstyles = 0

    def createPlot(self, title):
        plot = Plot(self, title)
        self.plots[title] = plot
        self.plotlist.append(plot) # for layout ordering
        return plot

    def save(self, path_doc):
        path_script = '/tmp/plot_%d.gnuplot' % (os.getpid())
        path_err = path_script + '.err'
        
        f = open(path_script, 'w')

        f.write('set term postscript eps enhanced color\n')
        f.write('set output "%s"\n' % path_doc)
        
        multiplot = len(self.plotlist) > 1

        if multiplot:
            layout = self.layout
            if not layout:
                layout = (len(self.plotlist), 1)
            f.write('set multiplot layout %d,%d\n' % (layout[0], layout[1]))
        
        for plot in self.plotlist:
            f.write('%s\n' % plot.getSpec())

        if multiplot:
            f.write('unset multiplot\n')

        f.close()

        gnuplot = common_functions.pw_env('gnuplot')
            
        # redirect stderr because it's verbose even on no errors
        rc = os.system(gnuplot + " '" +  path_script + "' 2>'" + path_err + "'" )
        if rc != 0:
            os.system( 'cat ' + path_err )
            sys.exit(1)

        os.remove( path_script )
        os.remove( path_err )

####################################################################################
###
### FUNCTION psencode()
###
####################################################################################
def psencode(text):
    # escape '_' so it doesn't make a subscript
    text = text.replace( '_', '\\_' )
    # escape '{'  and '}'
    text = text.replace( '{', '\\{' ).replace( '}', '\\}' )

    return text
