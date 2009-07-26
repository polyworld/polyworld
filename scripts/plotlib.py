import os

import common_functions

COLORS = ['red', 'green', 'blue', 'orange', 'pink', 'magenta', 'black', 'gray', 'cyan', 'yellow']

####################################################################################
###
### CLASS StyleGroup
###
####################################################################################
class StyleGroup:
    DOTTED = 0
    SOLID = 1
    DASHED = 2
    DASHDOT = 5
    LINE_TYPES = [SOLID, DASHED, DOTTED, DASHDOT]

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
            self.colors.remove(color)
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
        if self.group.plot.doc.nocolor:
            color = 'gray'
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
        self.table = col_x.table
        self.col_x = col_x
        self.col_y = col_y
        self.style = style
        self.smooth = smooth
        self.points = False

        self.title = title

    def getSpec(self):
        spec = "'%s' index %d using %s with %s ls %d" %(self.table.path,
                                                        self.table.index,
                                                        self._spec_using(),
                                                        self._spec_with(),
                                                        self.style.id)
        if self.title:
            spec += ' title "%s"' % self.title
        else:
            spec += ' notitle'

        if self.smooth:
            spec += ' smooth csplines'

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

        self.table = col_x.table
        self.cols = [col_x, col_opening, col_low, col_high, col_closing]
        self.whiskers = whiskers
        self.style = style
        self.smooth = False
        self.points = False

        self.title = title

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
        self.table = col_x.table
        self.col_x = col_x
        self.col_y = col_y
        self.col_err = col_err
        self.style = style
        self.smooth = False
        self.points = False

        self.title = title

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

        self.title = name
        self.xlabel = None
        self.ylabel = None
        self.grid = True
        self.boxwidth = 0.5
        self.fill_opacity = 0.3
        self.legend = True

        self.stylegroup = None
        self.stylegroups = {}
        self.linetypes = list(StyleGroup.LINE_TYPES)
        self.reserved_colors = []

    def createStyleGroup(self,
                         name = None,
                         linetype = None,
                         linewidth = StyleGroup.THIN):
        if not linetype:
            linetype = self.linetypes.pop(0)

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
            spec += 'set xlabel "%s" font "Times,12"\n' % self.xlabel
        else:
            spec += 'unset xlabel\n'
        if self.ylabel:
            spec += 'set ylabel "%s" font "Times,12"\n' % self.ylabel
        else:
            spec += 'unset ylabel\n'

        #if self.points:
        #    spec += 'set style data points\n'
        #else:
        #    spec += 'set style data lines\n'
        
        spec += 'set style fill solid %f\n' % self.fill_opacity
        spec += 'set boxwidth %f relative\n' % self.boxwidth

        for i in range(len(self.curvelist)):
            spec += self.curvelist[i].style.getSpec()

        if self.title:
            spec += 'set title "%s"\n' % self.title
            spec += 'show title\n'

        spec += 'plot '

        for i in range(len(self.curvelist)):
            if i > 0:
                spec += ', '
            spec += self.curvelist[i].getSpec()

        spec += '\n'

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
        os.system('%s %s 2>/dev/null' % (gnuplot, path_script))

        return path_doc, path_script
