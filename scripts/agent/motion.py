"""
motion.py

Contains Positions class, which represents all the motion of an agent.
"""

from lazy import Lazy
import datalib
import os.path

class Positions: 
    def __init__(self, filename):
        self.filename = filename
        assert os.path.isfile(self.filename),\
                "Invalid motion file: %s" % self.filename

    @Lazy
    def positions(self):
        ''' Lazy loading of position data'''
        position_table = datalib.parse(self.filename,
                          keycolname='Timestep')['Positions']
        
        positions = {}
        for row in position_table.rowlist:
            positions.update({ row['Timestep'] : (row['x'], row['y'], row['z']) })
        
        return positions


    def __getitem__(self, key):
        return self.positions[key]
