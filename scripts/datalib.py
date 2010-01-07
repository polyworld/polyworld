import re
import common_functions

REQUIRED = True

SIGNATURE = '#datalib\n'
CURRENT_VERSION = 2
COLUMN_TYPE_MARKER = "#@T"
COLUMN_LABEL_MARKER = "#@L"

####################################################################################
###
### CLASS Table
###
####################################################################################
class Table:
    def __init__(self,
                 name,
                 colnames,
                 coltypes,
                 path = None,
                 index = None,
                 keycolname = None):
        assert(len(colnames) == len(coltypes))

        self.name = name
        self.colnames = colnames
        self.coltypes = coltypes
        self.path = path
        self.index = index
        self.keycolname = keycolname

        self.coldata = [[] for x in range(len(colnames))]
        self.collist = [Column(self, x) for x in range(len(colnames))]
        self.coldict = dict([(colnames[x], self.collist[x]) for x in range(len(colnames))])

        self.rowlist = []

        self.keymap = {}

    # override [] operator
    def __getitem__(self, key):
        return self.rowlist[self.keymap[key]]

    def rows(self):
        return self.rowlist

    def columns(self):
        return self.collist

    def getColumn(self, colname):
        return self.coldict[colname]

    def createRow(self):
        irow = len(self.coldata[0])

        for coldata in self.coldata: coldata.append(None)

        row = Row(self, irow)
        self.rowlist.append(row)
        
        return row

####################################################################################
###
### CLASS Row
###
####################################################################################
class Row:
    def __init__(self, table, index):
        self.table = table
        self.index = index

    def __iter__(self):
        for col in self.table.columns():
            yield col.get(self.index)

    # override [] operator
    def __getitem__(self, colname):
        return self.get(colname)

    # override [] operator
    def __setitem__(self, colname, value):
        return self.set(colname, value)

    def get(self, colname):
        return self.table.coldict[colname].get(self.index)

    def set(self, colname, value):
        return self.table.coldict[colname].set(self.index, value)

    def mutate(self, colname, func):
        return self.table.coldict[colname].mutate(self.index, func)

####################################################################################
###
### CLASS Column
###
####################################################################################
class Column:
    def __init__(self, table, index):
        self.table = table
        self.index = index
        self.name = table.colnames[index]
        self.data = table.coldata[index]

        self.iskey = (self.name == table.keycolname)

        type = table.coltypes[index]
        if type == 'int':
            self.convert = int
        elif type == 'float':
            self.convert = float
        else:
            raise ValueError('illegal data type ('+type+')')

    def __iter__(self):
        for d in self.data:
            yield d

    def get(self, i):
        return self.data[i]

    def set(self, i, value):
        if self.iskey:
            self.__clear_keymap(i)

        value = self.convert(value)
        self.data[i] = value

        if self.iskey:
            self.__update_keymap(i)

        return value

    def mutate(self, i, func):
        if self.iskey:
            self.__clear_keymap(i)

        value = self.data[i] = func(self.data[i])

        if self.iskey:
            self.__update_keymap(i)

        return value

    def __clear_keymap(self, i):
        value = self.get(i)
        try:
            del self.table.keymap[value]
        except KeyError:
            pass

    def __update_keymap(self, i):
        value = self.get(i)

        try:
            self.table.keymap[value]
            raise DuplicateKeyError(str(value))
        except KeyError:
            pass

        self.table.keymap[value] = i

        return value
        
####################################################################################
###
### CLASS DuplicateKeyError
###
####################################################################################
class DuplicateKeyError(Exception):
    pass

####################################################################################
###
### CLASS InvalidFileError
###
####################################################################################
class InvalidFileError(Exception):
    pass

####################################################################################
###
### CLASS MissingTableError
###
####################################################################################
class MissingTableError(Exception):
    pass

####################################################################################
###
### FUNCTION write()
###
####################################################################################
def write(path, tables):
    try:
        # if it's a dict, get the values
        tables = tables.values()
    except AttributeError:
        pass

    try:
        # if it's not a list, put it in one
        len(tables)
    except AttributeError:
        tables = [tables]

    class TableDims:
        def __init__( self, table ):
            self.name = table.name
            self.nrows = len( table.rows() )
            self.rowlen = -1
    tabledims = []


    f = open(path, 'w')

    __write_header(f)

    table_index = 0
    for table in tables:
        dims = TableDims( table )
        tabledims.append( dims )

        table.path = path
        
        table.index = table_index
        table_index += 1
        
        col_labels = __create_col_metadata(COLUMN_LABEL_MARKER,
                                           table.colnames)
        col_types = __create_col_metadata(COLUMN_TYPE_MARKER,
                                           table.coltypes)


        dims.offset = f.tell()

        __start_table(f,
                      table.name,
                      col_labels,
                      col_types)

        dims.data = f.tell()
        
        for row in table.rows():
            rowstart = f.tell()

            linelist = ['   ']

            for data in row:
                linelist.append('%-18s' % data)

            linelist.append('\n')
            f.write(' '.join(linelist))

            rowlen = f.tell() - rowstart

            if dims.rowlen == -1:
                dims.rowlen = rowlen
            else:
                assert dims.rowlen == rowlen

        __end_table(f,
                    table.name)

    __write_footer( f, tabledims )

    f.close()

####################################################################################
###
### FUNCTION parse_all()
###
####################################################################################
def parse_all(paths, tablenames = None, required = not REQUIRED, keycolname = None):
    tables = {}

    for path in paths:
        tables[path] = parse(path, tablenames, required, keycolname)

    return tables

####################################################################################
###
### FUNCTION parse()
###
####################################################################################
def parse(path, tablenames = None, required = not REQUIRED, keycolname = None):
    f = open(path, 'r')

    if tablenames:
        tablenames_found = dict([(tablename, False) for tablename in tablenames])

    if not __is_datalib_file(f):
        raise InvalidFileError(path)

    version = common_functions.get_version(f.readline())
    if version > CURRENT_VERSION:
        raise InvalidFileError('invalid version (%s)' % version)

    tables = {}
    table_index = -1

    while True:
        tablename = __seek_next_tag(f)
        if not tablename: break

        table_index += 1

        if tablenames:
            if not tablename in tablenames:
                __seek_end_tag(f, tablename)
                continue
            else:
                tablenames_found[tablename] = True
            

        # --- read column names
        colnames = f.readline().split()
        if len(colnames) == 0:
            raise InvalidFileError('expecting column labels')
        elif colnames[0] != COLUMN_LABEL_MARKER:
            raise InvalidFileError('unexpected token (%s)' % colnames[0])
        colnames.pop(0) # remove marker

        f.readline() # skip blank line

        # --- read column types
        coltypes = f.readline().split()
        if len(coltypes) == 0:
            raise InvalidFileError('expecting column types')
        elif coltypes[0] != COLUMN_TYPE_MARKER:
            raise InvalidFileError('unexpected token (%s)' % coltypes[0])
        coltypes.pop(0) # remove marker

        f.readline() # skip blank line


        # --- construct table object
        table = Table(tablename,
                      colnames,
                      coltypes,
                      path,
                      table_index,
                      keycolname = keycolname)
        tables[tablename] = table

        found_end_tag = False
        
        # --- parse data until we reach </name>
        while True:
            line = f.readline()
            tag = __get_end_tag(line)

            if tag:
                assert(tag == tablename)
                found_end_tag = True
                break

            data = line.split()
            if len(data) == 0:
                raise InvalidFileError("Missing end tag for %s" % tablename)
            elif len(data) != len(colnames):
                raise InvalidFileError("Missing data for %s" % tablename)

            row = table.createRow()
            for col in table.columns():
                col.set(row.index, data[col.index])

    if tablenames and required:
        for name, found in tablenames_found.items():
            if not found:
                raise MissingTableError('Failed to find %s in %s' % (name, path))

    return tables

####################################################################################
###
### FUNCTION __write_header()
###
####################################################################################
def __write_header(f):
    f.write(SIGNATURE)
    f.write('#version=%d\n' % CURRENT_VERSION)

####################################################################################
###
### FUNCTION __write_footer()
###
####################################################################################
def __write_footer( f, tabledims ):
    start = f.tell()

    f.write( '#TABLES %d\n' % len(tabledims) )

    for dims in tabledims:
        f.write( '# %s %d %d %d %d\n' % (dims.name,
                                         dims.offset,
                                         dims.data,
                                         dims.nrows,
                                         dims.rowlen) )

    end = f.tell()

    f.write( '#START %d\n' % start )
    f.write( '#SIZE %d' % (end - start) )

####################################################################################
###
### FUNCTION __start_table_()
###
####################################################################################
def __start_table(f,
                  tablename,
                  col_labels,
                  col_types):
    f.write('\n#<%s>\n' % tablename)
    f.write(col_labels)
    f.write('#\n')
    f.write(col_types)
    f.write('#\n')

####################################################################################
###
### FUNCTION __end_table()
###
####################################################################################
def __end_table(f, tablename):
    f.write('#</%s>\n\n' % tablename)

####################################################################################
###
### FUNCTION __create_col_metadata()
###
####################################################################################
def __create_col_metadata(marker, meta):
    linelist = [marker]
    for colmeta in meta:
        linelist.append('%-18s' % colmeta)
    linelist.append('\n')

    return ' '.join(linelist)

####################################################################################
###
### FUNCTION __is_datalib_file()
###
####################################################################################
def __is_datalib_file(file):
    return file.readline() == SIGNATURE

####################################################################################
###
### FUNCTION __seek_next_tag()
###
####################################################################################
def __seek_next_tag(file):
    regex = '^#\\<([a-zA-Z0-9_ \-]+)\\>'

    while True:
        line = file.readline();
        if not line: break

        result = re.search(regex, line)
        if result: return result.group(1)

    return None

####################################################################################
###
### FUNCTION __get_end_tag()
###
####################################################################################
def __get_end_tag(line):
    regex = '^#\\</([a-zA-Z0-9_ \-]+)\\>'

    result = re.search(regex, line)
    if result:
        return result.group(1)
    else:
        return None

####################################################################################
###
### FUNCTION __seek_end_tag()
###
####################################################################################
def __seek_end_tag(file, tag):
    while True:
        line = file.readline();
        if not line: break

        endtag = __get_end_tag(line)

        if endtag:
            if tag != endtag:
                raise InvalidFileError('unbalanced tag '+tag+' (found '+endtag+')')
            else:
                return

    raise InvalidFileError('unbalanced tag '+tag+' (no terminating tag)')

####################################################################################
###
### FUNCTION test()
###
####################################################################################
def test():
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
