import re
import common_functions

REQUIRED = True

SIGNATURE = '#datalib\n'
CURRENT_VERSION = 1
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
                 index = None):
        assert(len(colnames) == len(coltypes))

        self.name = name
        self.colnames = colnames
        self.coltypes = coltypes
        self.path = path
        self.index = index

        self.coldata = [[] for x in range(len(colnames))]
        self.collist = [Column(self, x) for x in range(len(colnames))]
        self.coldict = dict([(colnames[x], self.collist[x]) for x in range(len(colnames))])

        self.rowlist = []

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
        value = self.convert(value)
        self.data[i] = value
        return value

    def mutate(self, i, func):
        value = self.data[i] = func(self.data[i])
        return value

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

    f = open(path, 'w')

    __write_header(f)

    table_index = 0
    for table in tables:
        table.path = path
        
        table.index = table_index
        table_index += 1
        
        col_labels = __create_col_metadata(COLUMN_LABEL_MARKER,
                                           table.colnames)
        col_types = __create_col_metadata(COLUMN_TYPE_MARKER,
                                           table.coltypes)

        __start_table(f,
                      table.name,
                      col_labels,
                      col_types)
        
        for row in table.rows():
            linelist = ['   ']

            for data in row:
                linelist.append('%-18s' % data)

            linelist.append('\n')
            f.write(' '.join(linelist))

        __end_table(f,
                    table.name)

    f.close()

####################################################################################
###
### FUNCTION parse()
###
####################################################################################
def parse(path, tablenames = None, required = not REQUIRED):
    f = open(path, 'r')

    if tablenames:
        tablenames_found = dict([(tablename, False) for tablename in tablenames])

    if not __is_datalib_file(f):
        raise InvalidFileError(path)

    version = common_functions.get_version(f.readline())
    if not version == CURRENT_VERSION:
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
                      table_index)
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
    regex = '^#\\<([a-zA-Z]+)\\>'

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
    regex = '^#\\</([a-zA-Z]+)\\>'

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
