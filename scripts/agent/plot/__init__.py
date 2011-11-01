import population
import movie
import barplot

from argparse import ArgumentParser as _Parser
Parser = _Parser(add_help=False)

Parser.add_argument('-r', '--run', dest='run_dir', action='store', 
    default='../run/', help="set the run directory (default: '../run/')")
Parser.add_argument('-c', '--cluster', dest='cluster_file', action='store',
    help="set the cluster data file")
