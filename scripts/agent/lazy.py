''' Lazy decorator '''
''' create lazy-default dict:
http://docs.python.org/library/collections.html#collections.defaultdict
'''
class Lazy(object):
    def __init__(self, calculate_function):
        self._calculate = calculate_function

    def __get__(self, obj, _=None):
        if obj is None:
            return self
        #print "calculating", obj, self._calculate.func_name
        value = self._calculate(obj)
        setattr(obj, self._calculate.func_name, value)
        return value
