#!/usr/bin/env python

import common_genome

def main():
    test__SeparationCache()

def test__SeparationCache():
    cache = common_genome.SeparationCache('run_test')
    
    print cache.separation(944, 965)
    print cache.separation(965, 944)

    print cache.separation(21, 201)
    print cache.separation(201, 21)
    

main()
