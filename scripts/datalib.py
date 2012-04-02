import re
import common_functions
import iterators
import os

REQUIRED = True

SIGNATURE = '#datalib\n'
CURRENT_VERSION = 3
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

		for x in self.coldata: x.append( None )
		row = Row(self, irow)
		self.rowlist.append(row)
		
		return row

	def sortrows( self, cmp ):
		sortedrows = sorted( self.rowlist, cmp = cmp )
		self.rowlist = sortedrows

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
		elif type == 'string':
			self.convert = lambda x: x
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
def write( path,
		   tables,
		   append = False,
		   replace = True,
		   randomAccess = True,
		   singleSchema = False ):

	colformat = 'fixed' if randomAccess else 'none'
	schema = 'single' if singleSchema else 'table'

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
	
	if append:
		try:
			f = open(path, 'r')
		except IOError:
			pass
		else:
			existing_tables = parse(path).values()
			remove_existing = []
			remove_new = []
			for et in existing_tables:
				for t in tables:
					if et.name == t.name:
						if replace:
							remove_existing.append(et)
						else:
							print 'WARNING: %s table found in both existing and new data, but not allowed to replace; discarding new data and retaining old' % et.name
							remove_new.append(t)
			f.close()
			for et in remove_existing:
				existing_tables.remove(et)
			for t in remove_new:
				tables.remove(t)
			tables = existing_tables + tables

	class TableDims:
		def __init__( self, table ):
			self.name = table.name
			self.nrows = 0
			self.rowlen = -1
	tabledims = []

	f = open(path, 'w')

	__write_header(f, schema, colformat)

	if schema == 'single':
		table = tables[0]
		col_widths = __calc_col_widths(table, colformat)
		col_labels = __create_col_metadata(COLUMN_LABEL_MARKER, table.colnames, col_widths)
		col_types = __create_col_metadata(COLUMN_TYPE_MARKER, table.coltypes, col_widths)
		f.write(col_labels)
		f.write(col_types)
		f.write('\n')

	table_index = 0
	for table in tables:
		dims = TableDims( table )
		tabledims.append( dims )

		table.path = path
		
		table.index = table_index
		table_index += 1
		
		if schema == 'single':
			assert( table.colnames == tables[0].colnames )
			assert( table.coltypes == tables[0].coltypes )
		else:
			col_widths = __calc_col_widths(table, colformat)

			col_labels = __create_col_metadata(COLUMN_LABEL_MARKER, table.colnames, col_widths)
			col_types = __create_col_metadata(COLUMN_TYPE_MARKER, table.coltypes, col_widths)


		dims.offset = f.tell()

		__start_table(f,
					  schema,
					  table.name,
					  col_labels,
					  col_types)

		dims.data = f.tell()
		
		for row in table.rows():
			dims.nrows += 1

			rowstart = f.tell()

			if colformat == 'fixed':
				linelist = ['   ']

				ncols = len(col_widths)
				for icol in range(ncols):
					width = col_widths[icol]
					data = table.columns()[icol].get( row.index )
					format = '%-' + str(width) + 's'
					linelist.append(format % data)
		
				linelist.append('\n')
				f.write('\t'.join(linelist))
		
				rowlen = f.tell() - rowstart
			elif colformat == 'none':
				f.write('\t'.join(map(str,row)))
				f.write('\n')
				rowlen = -1

			if dims.rowlen == -1:
				dims.rowlen = rowlen
			else:
				if dims.rowlen != rowlen:
					print 'dims.rowlen =', dims.rowlen
					print 'rowlen =', rowlen
					print 'table =', table.name
					print 'linelist =', linelist
					print 'col_widths =', col_widths
					print 'data =', ','.join(linelist)
					assert(false)

		__end_table(f, table.name)

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
def parse( path,
		   tablenames = None,
		   required = not REQUIRED,
		   keycolname = None,
		   tablename2key = lambda x: x,
		   stream_beginTable = None,
		   stream_row = None):

	f = open(path, 'r')

	if tablenames:
		tablenames_found = dict([(tablename, False) for tablename in tablenames])

	if not __is_datalib_file(f):
		raise InvalidFileError(path)

	version = common_functions.get_version(f.readline())
	if version > CURRENT_VERSION:
		raise InvalidFileError('invalid version (%s)' % version)

	if version < 3:
		schema = 'table'
		colformat = 'fixed'
	else:
		schema = common_functions.get_equals_decl(f.readline(), 'schema')
		colformat = common_functions.get_equals_decl(f.readline(), 'colformat')

	assert( schema in ['table', 'single'] )
	assert( colformat in ['fixed', 'none'] )

	class TableResult:
		def __init__(self):
			self.tables = {}

		def beginTable( self,
						tablename,
						colnames,
						coltypes,
						path,
						table_index,
						keycolname):

			self.table = Table(tablename,
							   colnames,
							   coltypes,
							   path,
							   table_index,
							   keycolname = keycolname)
			self.tables[tablename2key(tablename)] = self.table

		def row(self, data):
			row = self.table.createRow()
			for col in self.table.columns():
				col.set(row.index, data[col.index])

		def retval(self):
			return self.tables

	class StreamResult:
		def __init__(self):
			pass

		def beginTable(self,
					   tablename,
					   colnames,
					   coltypes,
					   path,
					   table_index,
					   keycolname):
			if stream_beginTable:
				stream_beginTable(tablename,
								  colnames,
								  coltypes,
								  path,
								  table_index,
								  keycolname)

			# a private table used for a row context in callback
			self.table = Table(tablename,
							   colnames,
							   coltypes,
							   path,
							   table_index,
							   keycolname = keycolname)
			self.__row = self.table.createRow()

		def row(self, data):
			if stream_row:
				for col in self.table.columns():
					col.set(self.__row.index, data[col.index])

				stream_row(self.__row)

		def retval(self):
			return None

	if stream_beginTable == None and stream_row == None:
		result = TableResult()
	else:
		result = StreamResult()

	table_index = -1

	if schema == 'single':
		__seek_meta(f, 'L')
		colnames = __parse_colnames( f )
		coltypes = __parse_coltypes( f )

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
			
		if schema == 'table':
			colnames = __parse_colnames( f )
			f.readline() # skip blank line
			coltypes = __parse_coltypes( f )
			f.readline() # skip blank line


		# --- begin table
		result.beginTable(tablename,
						  colnames,
						  coltypes,
						  path,
						  table_index,
						  keycolname)

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

			result.row(data)

	if tablenames and required:
		for name, found in tablenames_found.items():
			if not found:
				raise MissingTableError('Failed to find %s in %s' % (name, path))

	return result.retval();


####################################################################################
###
### FUNCTION parse_digest()
###
####################################################################################
def parse_digest( path ):
	f = open( path )
	
	def int_field(line):
		return int( line.split()[1] )
	
	#
	# Parse the digest start/size from end of file
	#
	f.seek( -64, os.SEEK_END )
	
	start = -1
	size = -1
	
	for line in f.readlines():
		if line.startswith( '#START' ):
			start = int_field( line )
		elif line.startswith('#SIZE'):
			size = int_field( line )
	
	assert( start > -1 )
	assert( size > -1 )
	
	#
	# Parse the digest
	#
	f.seek( start, os.SEEK_SET )
	
	line = f.readline()
	assert( line.startswith('#TABLES') )
	
	ntables = int_field( line )
	
	tables = {}
	
	for i in range(ntables):
		fields = f.readline().split()
	
		table = { 'name': fields[1],
				  'offset': int( fields[2] ),
				  'data': int( fields[3] ),
				  'nrows': int( fields[4] ),
				  'rowlen': int( fields[5] ) }
	
		tables[ table['name'] ] = table
	
	return {'tables': tables}

####################################################################################
###
### FUNCTION __write_header()
###
####################################################################################
def __write_header(f,schema,colformat):
	f.write(SIGNATURE)
	f.write('#version=%d\n' % CURRENT_VERSION)
	f.write('#schema=%s\n' % schema)
	f.write('#colformat=%s\n' % colformat)
	f.write('\n')

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
				  schema,
				  tablename,
				  col_labels,
				  col_types):
	f.write('#<%s>\n' % tablename)
	if schema != 'single':
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
	f.write('#</%s>\n\n\n' % tablename)

####################################################################################
###
### FUNCTION __calc_col_widths()
###
####################################################################################
def __calc_col_widths(table, colformat):
	col_widths = []

	for name, type, data in iterators.IteratorUnion(iter(table.colnames), iter(table.coltypes), iter(table.coldata)):
		# add 2 to name length to account for quotes
		width = max(len(str(name)) + 2, len(type))
		if colformat == 'fixed':
			for row in data:
				width = max(width, len(str(row)))

		col_widths.append(width)

	return col_widths

####################################################################################
###
### FUNCTION __create_col_metadata()
###
####################################################################################
def __create_col_metadata(marker, meta, colwidths):
	linelist = [marker]
	for colmeta, colwidth in iterators.IteratorUnion(iter(meta), iter(colwidths)):
		format = '%-' + str(colwidth) + 's'
		if marker == COLUMN_LABEL_MARKER:
			# add quotes
			colmeta = '"%s"' % colmeta
			
		linelist.append(format % colmeta)
	linelist.append('\n')

	return '\t'.join(linelist)

####################################################################################
###
### FUNCTION __parse_colnames()
###
####################################################################################
def __parse_colnames( f ):
	line = f.readline()
	tokens = line.split()
	if tokens[0] != COLUMN_LABEL_MARKER:
		raise InvalidFileError('unexpected token (%s)' % tokens[0])

	if '"' in line:
		# parse colnames enclosed in quotes
		colnames = re.findall(r'"([^"]+)"', line)
	else:
		colnames = tokens[1:]

	if len(colnames) == 0:
		raise InvalidFileError('expecting column labels')

	return colnames

####################################################################################
###
### FUNCTION __parse_coltypes()
###
####################################################################################
def __parse_coltypes( f ):
	coltypes = f.readline().split()
	if len(coltypes) == 0:
		raise InvalidFileError('expecting column types')
	elif coltypes[0] != COLUMN_TYPE_MARKER:
		raise InvalidFileError('unexpected token (%s)' % coltypes[0])
	coltypes.pop(0) # remove marker

	return coltypes

####################################################################################
###
### FUNCTION __is_datalib_file()
###
####################################################################################
def __is_datalib_file(file):
	return file.readline() == SIGNATURE

####################################################################################
###
### FUNCTION __seek_meta()
###
####################################################################################
def __seek_meta(file, meta):
	regex = '^#@'+meta+'\s'

	while True:
		pos = file.tell()
		line = file.readline()
		if not line: break

		result = re.search(regex, line)
		if result:
			file.seek(pos, os.SEEK_SET)
			return

	raise InvalidFileError('missing meta '+meta)

####################################################################################
###
### FUNCTION __seek_next_tag()
###
####################################################################################
def __seek_next_tag(file):
	regex = r'^#\<([^\>]+)\>'

	while True:
		line = file.readline()
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
	regex = r'^#\</([^\>]+)\>'

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
