from common_functions import warn
import itertools
from math import sqrt
from scipy import stats
# following special treatment of isnan() is to allow scripts to work with Python 2.5 and earlier
try:
	from math import isnan
except ImportError:
	def isnan(x):
		return x != x


import common_functions
import datalib
import iterators

####################################################################################
###
### FUNCTION median()
###
### In addition to returning the median, also returns the upper and lower half of
### the list.
###
### IMPORTANT: This function ASSUMES THE LIST IS ALREADY SORTED.
###
####################################################################################
def median( listofnumbers ):

	length=len(listofnumbers)

	if length == 1:
		return listofnumbers[0], listofnumbers, listofnumbers
	
	lenover2=int(length / 2)

	middle1=listofnumbers[ lenover2 ]			# this number is the answer if the length is ODD, and half of the answer if the length is EVEN
	lowerhalf=listofnumbers[: lenover2 ]	# first half of the numbers

		
	if length % 2 == 0:			# if the length of the list is an EVEN number
		upperhalf=listofnumbers[ lenover2 :]
		middle2=listofnumbers[ (lenover2 - 1) ]
		median = (middle1 + middle2) / 2.0
	else:						# the length of the list is an ODD number, so simply return the middle number.
		upperhalf=listofnumbers[(lenover2+1) :]	# second half of the numbers
		median = middle1
	
	return median, lowerhalf, upperhalf

####################################################################################
###
### FUNCTION sample_mean()
###
####################################################################################
def sample_mean( list ):
        N = float(len(list))
        mean = sum(list) / N
        SSE=0
        for item in list:
                SSE += (item - mean) * (item - mean)

        try: variance = SSE / (N-1)
        except: variance = 0

	stddev = sqrt(variance)

        stderr = ( variance / N ) ** 0.5 # stderr = stddev / sqrt(N)

        return mean, stddev, stderr

####################################################################################
###
### FUNCTION diff_mean()
###
### Calculates mean difference between two sets of data represented as iterators
###
####################################################################################
def diff_mean( ita, itb ):
	n = 0
	sum = 0.0
	n_inf = 0
	inf = float('inf')
	for a, b in iterators.IteratorUnion( ita, itb ):
		if a == inf or b == inf:
			n_inf += 1
		else:
			n += 1
			sum += a - b

	if n_inf > 0:
		print 'diff_mean() ignoring', n_inf, 'inf value(s)'
	
	if n < 1:
		return 0.0
	else:
		return sum / n

####################################################################################
###
### FUNCTION diff_stddev()
###
####################################################################################
def diff_stddev( diffmean, ita, itb ):
	n = 0
	sum = 0.0
	n_inf = 0
	inf = float('inf')
	for a, b in iterators.IteratorUnion( ita, itb ):
		if a == inf or b == inf:
			n_inf += 1
		else:
			n += 1
			delta = a - b - diffmean
			sum += delta ** 2

	if n_inf > 0:
		print 'diff_stddev() ignoring', n_inf, 'inf value(s)'
	
	if n < 1:
		return 0.0
	else:
		return sqrt( sum / (n - 1) )

####################################################################################
###
### FUNCTION tval()
###
####################################################################################
def tval( mean, stddev, n ):
	if stddev:
		return (abs( mean ) / stddev) * sqrt( n - 1 )
	else:
		return 0.0

####################################################################################
###
### FUNCTION pval()
###
####################################################################################
def pval( tval, n ):
	return stats.t.cdf(abs( tval ), n - 1)

####################################################################################
###
### FUNCTION avr()
###
### Computes avr over data, where data is a list of floats SORTED IN ASCENDING ORDER.
###
### Returns min, max, mean, mean_stderr, q1, q3, median
###
####################################################################################
def avr(data):
    try:
        minimum = data[0]
        maximum = data[-1]

	if isnan(minimum) or isnan(maximum):
	    nan = float('nan')
	    warn('algorithms.avr() encountered NaN!')
            return nan, nan, nan, nan, nan, nan, nan

        # sanity check (data should already be sorted)
        assert(minimum <= maximum)

        mean, stddev, stderr = sample_mean(data)

        med, lowerhalf, upperhalf = median(data)

        q1 = median( lowerhalf )[0]
        q3 = median( upperhalf )[0]
    except IndexError:  # no data points
        minimum, maximum, mean, stderr, q1, q3, med = (0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

    return minimum, maximum, mean, stderr, q1, q3, med

####################################################################################
###
### FUNCTION avr_table()
###
### TODO: This is a ridiculous interface. Code should be converted to use
###       avr_table_from_tables(), which can then be renamed to avr_table()
###
### Computes avr over DATA (see structure of DATA below), and places result in
### constructed table.
###
###
### DATA: If DATA takes the form of a dict of a dict, where the outer dict maps from
###	timestep to a dict that maps from complexity region to a list of complexity
###	data, then func_get_regiondata may be None. Put another way, DATA must be
###	structured like:
###
###	{
###		int timestep,
###		{
###			str complexity_region,
###			float[] complexity
###		}
###	}
###
###	In this case the complexity data is assumed to be sorted in ascending order.
###
###	If DATA is not in this representation, then func_get_regiondata must be
###	provided, and it must provide an iterator of the complexity_region data.
###	func_get_regiondata must be of the form:
###
###		Iterator xxx( void *DATA, string region, int t )
###
###	See avr_meta for an example of using func_get_regiondata
###
###	Note that the iterator returned by func_get_regiondata may return the
###	data in any order.
###
####################################################################################
def avr_table( DATA, regions, timesteps, func_get_regiondata = None ):
	colnames = ['Timestep', 'min', 'q1', 'median', 'q3', 'max', 'mean', 'mean_stderr', 'sampsize']
	coltypes = ['int', 'float', 'float', 'float', 'float', 'float', 'float', 'float', 'int']

	tables = {}

	for region in regions:
		table = datalib.Table(region, colnames, coltypes)
		tables[region] = table

		for t in timesteps:
			if func_get_regiondata:
				data = func_get_regiondata( DATA, region, t )
				regiondata = []
				for x in data:
					regiondata.append( x )
				regiondata.sort()
			else:
				regiondata = DATA[t][region]
                        
                        minimum, maximum, mean, mean_stderr, q1, q3, median = avr(regiondata)

			row = table.createRow()
			row.set('Timestep', t)
			row.set('min', minimum)
			row.set('max', maximum)
			row.set('mean', mean)
			row.set('mean_stderr', mean_stderr)
			row.set('median', median)
			row.set('q1', q1)
			row.set('q3', q3)
			row.set('sampsize', len(regiondata))

	return tables

####################################################################################
###
### FUNCTION avr_table_from_tables()
###
####################################################################################
def avr_table_from_tables( avr_table_name, tables, xcolname, ycolname ):
	colnames = [xcolname, 'min', 'q1', 'median', 'q3', 'max', 'mean', 'mean_stderr', 'sampsize']
	xcoltype = tables[0].coltypes[ tables[0].colnames.index(xcolname) ]
	coltypes = [xcoltype, 'float', 'float', 'float', 'float', 'float', 'float', 'float', 'int']

	result = datalib.Table(avr_table_name, colnames, coltypes)

	xvalues = common_functions.get_timesteps( tables, xcolname )

	for x in xvalues:
		ydata = []
		for table in tables:
			ydata.append( table[x][ycolname] )
		ydata.sort()
                        
                minimum, maximum, mean, mean_stderr, q1, q3, median = avr(ydata)

		row = result.createRow()
		row.set(xcolname, x)
		row.set('min', minimum)
		row.set('max', maximum)
		row.set('mean', mean)
		row.set('mean_stderr', mean_stderr)
		row.set('median', median)
		row.set('q1', q1)
		row.set('q3', q3)
		row.set('sampsize', len(ydata))

	return result

####################################################################################
###
### FUNCTION ttest()
###
####################################################################################
def ttest( data1, data2 ):
	assert( len(data1) == len(data2) )
	n = len(data1)
	if n < 2:
		return 0.0, 0.0, 0.0, 0.0

	mean = diff_mean( iter(data1),
			  iter(data2) )

	dev = diff_stddev( mean,
			   iter(data1),
			   iter(data2) )

	_tval = tval( mean, dev, n )
	_pval = pval( _tval, n )

	return mean, dev, _tval, _pval

####################################################################################
###
### FUNCTION ttest_table()
###
####################################################################################
def ttest_table( ttest_table_name,
		 tcrit,
		 tables1,
		 tables2,
		 xcolname,
		 ycolname ):

    assert( len(tables1) == len(tables2) )

    colnames = [xcolname, 'Mean', 'StdDev', 'Sig', 'tval', 'pval'] 
    xcoltype = tables1[0].coltypes[ tables1[0].colnames.index(xcolname) ]
    coltypes = [xcoltype, 'float', 'float', 'int', 'float', 'float']

    xvalues = common_functions.get_timesteps( tables1 + tables2, xcolname )

    result = datalib.Table( ttest_table_name, colnames, coltypes )

    for x in xvalues:
	    data1 = [ table[x][ycolname] for table in tables1 ]
	    data2 = [ table[x][ycolname] for table in tables2 ]

	    diffMean, stdDev, tval, pval = ttest( data1, data2 )

	    row = result.createRow()

	    row[xcolname] = x
	    row["Mean"] = diffMean
	    row["StdDev"] = stdDev
	    if tval >= tcrit:
                    row["Sig"] = 1
	    else:
                    row["Sig"] = 0
	    row["tval"] = tval
	    row["pval"] = pval

    return result


####################################################################################
###
### FUNCTION make_epochs()
###
####################################################################################
def make_epochs( table, xcolname, ycolname, epoch_len = 1000, endStep = None ):
    timeCol = table.getColumn( xcolname ).data
    if endStep == None:
        endStep = timeCol[-1]

    colnames = [xcolname, ycolname]
    coltypes = ['int', 'float']
    table_epoch = datalib.Table( table.name + "-epoch",
                                 colnames,
                                 coltypes,
                                 keycolname = xcolname )

    time_index = 0
    row = table_epoch.createRow()
    row[xcolname] = 0
    row[ycolname] = table.rows()[time_index][ycolname]

    time_index = 1

    for epoch_start in range( 1, endStep, epoch_len ):
        epoch_end = min( epoch_start + epoch_len - 1, endStep )

        values = []
        while time_index < len(timeCol) and timeCol[time_index] <= epoch_end:
            values.append( table.rows()[time_index][ycolname] )
            time_index += 1

        row = table_epoch.createRow()
        row[xcolname] = epoch_end
        if len(values):
            row[ycolname] = float( sum(values) ) / len(values)
        else:
            # Use value from previous epoch
            row[ycolname] = table_epoch.rows()[-2][ycolname]

    return table_epoch


####################################################################################
###
### FUNCTION copy_matrix()
###
####################################################################################
def copy_matrix(m):
	n = []
	for i in range(len(m)):
		row = []
		for j in range(len(m[i])):
			row.append(m[i][j])
		n.append(row)
	
	return n
	

####################################################################################
###
### FUNCTION wd_to_bu()  # Convert weighted, directed matrix to binary, undirected
###                      # Assumes all positive (or zero) weights
###
####################################################################################
def wd_to_bu(m, threshold=0.0):
	m_wu = wd_to_wu(m)
	m_bu = copy_matrix(m_wu)
	for i in range(len(m)):
		for j in range(len(m[i])):
			if m_wu[i][j] > threshold:
				m_bu[i][j] = 1
			else:
				m_bu[i][j] = 0
	
	return m_bu
			

####################################################################################
###
### FUNCTION wd_to_bd()  # Convert weighted, directed matrix to binary, directed
###                      # Assumes all positive (or zero) weights
###
####################################################################################
def wd_to_bd(m, threshold=0.0):
	m_bd = copy_matrix(m)
	for i in range(len(m)):
		for j in range(len(m[i])):
			if m[i][j] > threshold:
				m_bd[i][j] = 1
			else:
				m_bd[i][j] = 0

	return m_bd


####################################################################################
###
### FUNCTION wd_to_wu()  # Convert weighted, directed matrix to weighted, undirected
###                      # Assumes all positive (or zero) weights
###
####################################################################################
def wd_to_wu(m):
	m_wu = copy_matrix(m)
	for i in range(len(m)):
		for j in range(i):
			m_wu[i][j] = m_wu[j][i] = (m[i][j] + m[j][i]) * 0.5

	return m_wu


####################################################################################
###
### FUNCTION count_edges()  # counts edges in a graph matrix
###                         # Assumes all positive (or zero) weights, and that
###							# the diagonal is all zeroes (no self-connections).
###							# (If any diagonal elements are non-zero, they will
###							# be counted in the directed=True case, but not the
###							# directed=False case.)
###							# If directed is False, it is assumed that the
###							# graph is symmetric, and only the lower triangle
###							# is counted.
###
####################################################################################
def count_edges(m, directed=True, threshold=0.0):
	count = 0
	if directed:
		for i in range(len(m)):
			for j in range(len(m[i])):
				if m[i][j] > threshold:
					count += 1
	else:
		for i in range(len(m)):
			for j in range(i):
				if m[i][j] > threshold:
					count += 1
	
	return count


####################################################################################
###
### FUNCTION w_to_d()  # Converts matrix weights to distances (by inversion),
###					   # leaving zeroes as zeroes, and
###					   # optionally zeroing weights smaller than min_weight.
###					   # Assures the diagonal elements are zero.
###					   # Assumes a square matrix.
###
####################################################################################
def w_to_d(mw, min_weight=0.0):
	n = len(mw)
	md = []
	for i in range(n):
		md.append([])
		for j in range(n):
			w = mw[i][j]
			if i == j or w <= min_weight:
				d = 0.0
			else:
				d = 1.0 / w
			md[i].append(d)

	return md

