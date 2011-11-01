#!/usr/bin/python
"""
visualize.py

Script to generate visualizations from the agent.visualization module. Operates
as a master main for the scripts in polyworld/scripts/agent/visualization/

Usage:

    ./visualize.py [MODE] [OPTIONS]

barplot         fingerprint graph (gene values and complexity by cluster)
population      population graph
complexity      plots TSE neural complexity
contacts        plots offspring per contact and offspring per contact ratio
distance        plots geographic distance at birth against genetic distance
genome          plots average gene value over time
movie           creates movie
scatter         creates a scatter plot of metrics against time (like plotgenome, but with data points)
"""
import agent.plot

if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(parents=[agent.plot.Parser],
        description="Visualizes Polyworld run data.")


    parser.add_argument('mode', help='set the mode', 
        choices=['population', 'movie', 'barplot'])

    args = parser.parse_args()

    

    if args.mode == 'population':
        agent.plot.population.main(args.run_dir, args.cluster_file)
    elif args.mode == 'movie':
        agent.plot.movie.main(args.run_dir, args.cluster_file)
    elif args.mode == 'barplot':
        agent.plot.barplot.main(args.run_dir, args.cluster_file)
    else:
        raise NotImplementedError
