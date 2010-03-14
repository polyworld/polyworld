####################################################################################
###
### FUNCTION product
###
### see standard itertools.product(). This is here because we need to support
### Python 2.5
###
####################################################################################
def product( ita, itb ):
    return [(x,y) for x in ita for y in itb]

####################################################################################
###
### FUNCTION concat
###
### concatenate one or more iterators
###
####################################################################################
def concat(*iterators):
    for i in iterators:
        for x in i:
            yield x

####################################################################################
###
### CLASS IteratorUnion
###
### Iterator that combines one or more iterators in such a way that each iteration
### contains a tuple consisting of an iteration result from each iterator.
###
### Example:
###
###     it = IteratorUnion(iter(['a','b']), iter(['x','y']))
###     for x in it:
###             print x
###
###     YIELDS
###
###     ('a','x')
###     ('b','y')
###
####################################################################################
class IteratorUnion:
    def __init__( self, *iterables ):
        self.iterables = iterables
        self.n = len( iterables )
        self.l = []

    def __iter__( self ):
        for a in self.iterables[0]:
            try:
                self.l[0] = a
            except IndexError:
                self.l.append(a)

            for i in range(1, self.n):
                try:
                    b = self.iterables[i].next()
                except StopIteration:
                    raise Exception('len(iterables[%d]) < len(iterables[0])')

                try:
                    self.l[i] = b
                except IndexError:
                    self.l.append(b)

            yield tuple(self.l)

        for i in range(1, self.n):
            try:
                self.iterables[i].next()
                raise Exception('len(iterables[%d] > len(iterables[0])' % i)
            except StopIteration:
                pass # normal case

####################################################################################
###
### CLASS MatrixIterator
###
### Iterates over subset of an n-dimensional matrix. Range must be provided for
### each dimension. Note that the matrix may take the form of dictionaries or
### datalib Tables, in which case the range may take the form of a list of keys.
###
### Example 1:
###
###     M = [['a','b'],
###          ['x','y']]
###     it = MatrixIterator(M, range(2), range(1,2))
###     for x in it:
###             print x
###
###     YIELDS:
###
###     b
###     y
###
### Example 2:
###
###     table = datalib.Table(name='Example 2',
###                           colnames=['Time','A','B'],
###                           coltypes=['int','float','float'],
###                           keycolname='Time')
###     row = table.createRow()
###     row['Time'] = 1
###     row['A'] = 100.0
###     row['B'] = 101.0
###     row = table.createRow()
###     row['Time'] = 2
###     row['A'] = 200.0
###     row['B'] = 201.0
###     
###     it = iterators.MatrixIterator(table, range(1,3), ['B'])
###     for b in it:
###         print b
###
###     YIELDS:
###
###     101.0
###     201.0
###
####################################################################################
class MatrixIterator:
    def __init__(self, matrix, *ranges):
        self.ranges = ranges
        self.index = [0 for dim in ranges]
        self.n = len(ranges)

        self.submatrix = []
        for i in range(self.n):
            self.submatrix.append(None)
        self.submatrix.append(matrix)


    def __iter__(self):
        while True:
            yield self.next()

    def next(self):
        return self.__next(0)

    def __next(self, dim):
        i = self.index[dim]
        lastdim = dim == self.n - 1

        if i == len(self.ranges[dim]):
            self.index[dim] = 0
            self.submatrix[dim] = None
            raise StopIteration()

        imatrix = self.ranges[dim][i]

        if (not lastdim) and (self.submatrix[dim] == None):
            self.submatrix[dim] = self.submatrix[dim - 1][imatrix]

        if lastdim:
            self.index[dim] += 1
            return self.submatrix[dim - 1][imatrix]
        else:
            try:
                return self.__next(dim + 1)
            except StopIteration:
                self.submatrix[dim] = None
                self.index[dim] += 1
                return self.__next(dim)
