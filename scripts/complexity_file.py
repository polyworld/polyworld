import re
import common_complexity

####################################################################################
###
### FUNCTION parse_complexities()
###
####################################################################################
def parse_complexities(path):
    retval = {}

    f = open(path, 'r')
    line = f.readline()

    retval['version'] = version = get_version(line)

    col_num = None
    col_c = 0
    critter_numbers = None

    if version == 0:
        f.seek(0, 0)
    if version == 2:
        col_num = 0
        col_c = 1
        retval['type'] = get_equals_decl(f.readline(), 'type')
        f.readline() # skip column headers
        retval['critter_numbers'] = critter_numbers = []

    retval['complexities'] = complexities = []

    for line in f:
        cols = line.split()

        if version == 2:
            critter_numbers.append(int(cols[col_num]))

        complexities.append(float(cols[col_c]))

    f.close()

    return retval

####################################################################################
###
### FUNCTION write_avr()
###
####################################################################################
def write_avr(path):
    pass

####################################################################################
###
### FUNCTION parse_avr()
###
####################################################################################
def parse_avr(path):
    f = open(path, 'r')
    
    version = get_version(f.readline())

    types = {}
    typenames = []

    if version == 2:
        while True:
            typename = seek_next_tag(f)
            if not typename: break

            typenames.append(typename)

            header = f.readline()
            fieldnames = header.split()[1:] # skip leading '#'
            
            fields = {}

            for fname in fieldnames:
                fields[fname] = []

            while True:
                line = f.readline()
                tag = get_end_tag(line)

                if tag:
                    assert(tag == typename)
                    break

                data = line.split()

                assert(len(data) == len(fieldnames))

                for i in range(len(data)):
                    fields[fieldnames[i]].append(data[i])

            types[typename] = {'fieldnames': fieldnames,
                               'fields': fields}

    elif version == 1:
        f.readline() # skip column headers

        typenames = common_complexity.DEFAULT_COMPLEXITIES
        fieldnames = common_complexity.AVR_FIELDS

        for typename in typenames:
            type = types[typename] = {}
            type['fieldnames'] = fieldnames
            type['fields'] = {}
            for field in fieldnames:
                type['fields'][field] = []

        while True:
            data = f.readline().split()
            if len(data) == 0: break

            t = data.pop(0)

            for typename in typenames:
                type = types[typename]
                fields = type['fields']

                for fieldname in fieldnames[1:]: # skip timestep
                    fields[fieldname].append(data.pop(0))
                    
    return {'typenames': typenames,
            'types': types}

        
####################################################################################
###
### FUNCTION write_plot_data()
###
####################################################################################
def write_plot_data(path, types, typenames, fieldnames):
    f = open(path, 'w')

    for typename in typenames:
        f.write('#<%s>\n' % (typename))

        f.write('#\t')
        for fieldname in fieldnames:
            f.write('%18s ' % (fieldname))
        f.write('\n')

        type = types[typename]
        fields = type['fields']
        nrecords = len(fields[fieldnames[0]]) 

        for i in range(nrecords):
            f.write('\t')
            for fieldname in fieldnames:
                field = fields[fieldname]
                value = field[i]
                f.write('%18s ' % (value))
            f.write('\n')
                

        f.write('#</%s>\n\n\n' % (typename))

    f.close()
