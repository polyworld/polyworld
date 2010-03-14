import os
import re

import common_functions

COLORS = ['red', 'green', 'blue', 'orange', 'pink', 'magenta', 'black', 'gray40', 'cyan', 'yellow']
GRAYSCALE = ['gray40', 'black']

####################################################################################
###
### CLASS StyleGroup
###
####################################################################################
class StyleGroup:
    DOTTED = 0
    SOLID = 1
    DASHED = 2
    HASHED = 3
    DASHDOT = 5
    LINE_TYPES = [SOLID, DASHED, DOTTED, DASHDOT, HASHED] * 4

    THIN = 1
    THICK = 3.5
    
    def __init__(self, plot, linetype, linewidth):
        self.plot = plot
        self.linetype = linetype
        self.linewidth = linewidth
        self.colors = list(COLORS)

    def createStyle(self, color = None):
        self.plot.doc.nstyles += 1
        if color == None:
            for color in self.colors:
                if not color in self.plot.reserved_colors:
                    self.colors.remove(color)
                    break
        else:
            try:
                self.colors.remove(color)
            except ValueError:
                # duplicate color in group, must be due to user intervention
                pass

        style = Style(self, self.plot.doc.nstyles, color)
        return style

####################################################################################
###
### CLASS Style
###
####################################################################################
class Style:
    def __init__(self, group, id, color):
        self.group = group
        self.id = id
        self.color = color

    def getSpec(self):
        if self.group.plot.doc.nocolor and not self.color in GRAYSCALE:
            color = 'gray40'
        else:
            color = self.color

        spec = 'set style line %d lt %d lw %f lc rgb "%s"\n' % (self.id,
                                                                self.group.linetype,
                                                                self.group.linewidth,
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
                 smooth):
        assert(col_x.table == col_y.table)
        self.col_x = col_x
        self.col_y = col_y

        self.init(title, col_x.table, style, smooth)

    def init(self, title, table, style, smooth):
        self.title = title
        self.table = table
        self.style = style
        self.smooth = smooth

        self.points = False
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
            spec += ' title "%s"' % psencode(self.title)
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
            spec += '("%s" ' % psencode(self.label)

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
                 whiskers,
                 style):

        self.cols = [col_x, col_opening, col_low, col_high, col_closing]
        self.whiskers = whiskers

        self.init(title, col_x.table, style, False)

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

        self.init(title, col_x.table, style, False)

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
        self.ylabel = None
        self.y2label = None
        self.y2range = None
        self.grid = True
        self.boxwidth = 0.5
        self.fill_opacity = 0.3
        self.legend = True

        self.rmargin = 0

        self.style_tick = None
        self.stylegroup = None
        self.stylegroups = {}
        self.linetypes = list(StyleGroup.LINE_TYPES)
        self.reserved_colors = []

    def createStyleGroup(self,
                         name = None,
                         linetype = None,
                         linewidth = StyleGroup.THIN,
                         make_context = True):
        if not linetype:
            linetype = self.linetypes.pop(0)
        else:
            self.linetypes.remove(linetype)

        self.stylegroup = StyleGroup(self, linetype, linewidth)
        if name:
            self.stylegroups[name] = self.stylegroup

        return self.stylegroup

    def setStyleGroup(self,
                      name):
        self.stylegroup = self.stylegroups[name]

    def getStyleGroup(self,
                      name):
        return self.stylegroups[name]

    def reserveColor(self, color):
        self.reserved_colors.append(color)

    def createTick(self,
                   y,
                   label = None,
                   line = False):

        if self.style_tick == None:
            group = self.createStyleGroup('__tick',
                                          StyleGroup.DASHED,
                                          StyleGroup.THIN,
                                          make_context = False)
            self.style_tick = group.createStyle('gray40')
        
        tick = Tick(len(self.ticks),
                    y,
                    label,
                    line,
                    self.style_tick)

        self.ticks.append(tick)

        return tick

    def createCurve(self,
                    table,
                    title,
                    name_col_x,
                    name_col_y,
                    style = None,
                    smooth = False):
        return self.__createCurve(Curve(title,
                                        table.getColumn(name_col_x),
                                        table.getColumn(name_col_y),
                                        self.__style(style),
                                        smooth))

    def createCandlestickCurve(self,
                               table,
                               title,
                               name_col_x,
                               name_col_opening,
                               name_col_low,
                               name_col_high,
                               name_col_closing,
                               whiskers = False,
                               style = None):
        return self.__createCurve(CandlestickCurve(title,
                                                   table.getColumn(name_col_x),
                                                   table.getColumn(name_col_opening),
                                                   table.getColumn(name_col_low),
                                                   table.getColumn(name_col_high),
                                                   table.getColumn(name_col_closing),
                                                   whiskers,
                                                   self.__style(style)))

    def createErrorbarCurve(self,
                            table,
                            title,
                            name_col_x,
                            name_col_y,
                            name_col_err,
                            style = None):
        return self.__createCurve(ErrorbarCurve(title,
                                                table.getColumn(name_col_x),
                                                table.getColumn(name_col_y),
                                                table.getColumn(name_col_err),
                                                self.__style(style)))

    def __style(self, style):
        if not style:
            if not self.stylegroup:
                self.stylegroup = self.createStyleGroup()
            style = self.stylegroup.createStyle()
        return style

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
            spec += 'set xlabel "%s" font "Times,12"\n' % psencode(self.xlabel)
        else:
            spec += 'unset xlabel\n'
        if self.ylabel:
            spec += 'set ylabel "%s" font "Times,12"\n' % psencode(self.ylabel)
        else:
            spec += 'unset ylabel\n'
        if self.y2label:
            spec += 'set y2label "%s" font "Times,12"\n' % psencode(self.y2label)
        else:
            spec += 'unset y2label\n'

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

        if self.style_tick:
            spec += self.style_tick.getSpec()

        for i in range(len(self.curvelist)):
            spec += self.curvelist[i].style.getSpec()

        if self.title:
            spec += 'set title "%s"\n' % psencode(self.title)
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
            
        # redirect stderr because it's verbose even on no errors (why?!)
        os.system('%s %s 2>%s.out' % (gnuplot, path_script, path_script))

        return path_doc, path_script

####################################################################################
###
### FUNCTION psencode()
###
####################################################################################
def psencode(text):
    # escape '_' so it doesn't make a subscript, so long as the '_'
    # proceeded by a letter.
    text = re.subn( r'_([a-zA-Z])',
                    r'\\\\_\1',
                    text )[0]

    # convert _num into _{num} for subscript
    text = re.subn( r'_([0-9]+)([^a-zA-Z])',
                    r'_{\1}\2',
                    text )[0]
    return text

####################################################################################
###
### CLASS Options
###
### Used for command-line options. Not as general-purpose as preceding content of
### this module.
###
####################################################################################
class Options:
    def __init__(self, flags, args):
        self.flags = flags
        self.args = args

        self.settings = dict([(name, False) for name in self.flags.values()])
        self.nset = 0
    
    def __str__(self):
        return """plotlib.Options:
        flags = %s,
        args = %s,
        settings = %s,
        nset = %d\n""" % (
        str(self.flags),
        str(self.args),
        str(self.settings),
        self.nset )

    def get(self, setting_name, default = None):
        try:
            return self.settings[setting_name]
        except KeyError, e:
            if default != None:
                return default
            else:
                raise e

    def set(self, setting_name, value, count = True):
        self.settings[setting_name] = value
        if count:
            self.nset += 1

    def process_opt(self, opt, value):
        if opt in self.flags.keys():
            self.set(self.flags[opt], True)
            return True
        elif opt in self.args.keys():
            self.set(self.args[opt], value)
            return True
        else:
            return False

####################################################################################
###
### FUNCTION getopts_encoding()
###
####################################################################################
def getopts_encoding(options):
    short = ""
    long = []

    for opts in options:
        for flag in opts.flags.keys():
            if len(flag) == 1:
                short += flag
            else:
                long.append(flag)

        for arg in opts.args.keys():
            if len(arg) == 1:
                short += arg + ":"
            else:
                long.append(arg + "=")

    return short, long

####################################################################################
###
### FUNCTION process_options()
###
####################################################################################
def process_options(options, getopts_opts):
    for opt, value in getopts_opts:
        opt = opt.strip('-')

        processed = False
        for flag in options.values():
            processed = flag.process_opt(opt, value)
            if processed:
                break

        if not processed:
            assert(False)
