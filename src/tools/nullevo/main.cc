#include <assert.h>
#include <iostream>
#include <iterator>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "genome/Genome.h"
#include "genome/GenomeUtil.h"
#include "sim/globals.h"
#include "utils/AbstractFile.h"
#include "utils/analysis.h"
#include "utils/misc.h"

#define BATCH 100

struct Args {
    int groupSize;
    std::string run;
    int agents;
    int steps;
    std::string path;
};

void printUsage(int, char**);
bool tryParseArgs(int, char**, Args&);
void update(std::vector<std::vector<int> >&, genome::Genome*, int, int);
double getDistance(const std::vector<std::vector<int> >&, int, int);
void logGenome(const std::string&, genome::Genome*);

int main(int argc, char** argv) {
    Args args;
    if (!tryParseArgs(argc, argv, args)) {
        printUsage(argc, argv);
        return 1;
    }
    analysis::initialize(args.run);
    std::vector<genome::Genome*> genomes;
    int geneCount = genome::GenomeUtil::schema->getMutableSize();
    std::vector<std::vector<int> > distribution(geneCount, std::vector<int>(256 >> args.groupSize));
    genome::Genome* seed = analysis::getGenome(args.run, 1);
    for (int agent = 1; agent <= args.agents; agent++) {
        genome::Genome* genome = genome::GenomeUtil::createGenome();
        genome->copyFrom(seed);
        genomes.push_back(genome);
    }
    update(distribution, seed, args.agents, args.groupSize);
    std::cout << 0 << " " << getDistance(distribution, args.agents, args.groupSize) << std::endl;
    for (int step = 1; step <= args.steps; step++) {
        int parent1 = (int)rrand(0, genomes.size());
        int parent2;
        do {
            parent2 = (int)rrand(0, genomes.size());
        } while (parent2 == parent1);
        genome::Genome* birth = genome::GenomeUtil::createGenome();
        birth->crossover(genomes[parent1], genomes[parent2], true);
        int child = (int)rrand(0, genomes.size());
        genome::Genome* death = genomes[child];
        genomes[child] = birth;
        update(distribution, birth, 1, args.groupSize);
        update(distribution, death, -1, args.groupSize);
        delete death;
        if (step % BATCH == 0) {
            std::cout << step << " " << getDistance(distribution, args.agents, args.groupSize) << std::endl;
        }
    }
    makeDirs(args.path);
    for (int agent = 1; agent <= args.agents; agent++) {
        genome::Genome* genome = genomes[agent - 1];
        logGenome(args.path + "/genome_" + std::to_string(agent) + ".txt", genomes[agent - 1]);
        delete genome;
    }
    return 0;
}

void printUsage(int argc, char** argv) {
    std::cerr << "Usage: " << argv[0] << " GROUP_SIZE RUN AGENTS STEPS PATH" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Generates selection-agnostic agents assuming steady-state conditions." << std::endl;
    std::cerr << std::endl;
    std::cerr << "  GROUP_SIZE  Size of allele groups in bits" << std::endl;
    std::cerr << "  RUN         Run directory" << std::endl;
    std::cerr << "  AGENTS      Number of agents in the population" << std::endl;
    std::cerr << "  STEPS       Number of steps to simulate" << std::endl;
    std::cerr << "  PATH        Logged genome directory" << std::endl;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    if (argc != 6) {
        return false;
    }
    int groupSize;
    std::string run;
    int agents;
    int steps;
    std::string path;
    try {
        int argi = 1;
        groupSize = atoi(argv[argi++]);
        if (groupSize < 0 || groupSize > 7) {
            return false;
        }
        run = std::string(argv[argi++]);
        if (!exists(run + "/endStep.txt")) {
            return false;
        }
        agents = atoi(argv[argi++]);
        if (agents < 2) {
            return false;
        }
        int groupCount = 256 >> groupSize;
        agents = groupCount * (int)ceil((double)agents / groupCount);
        steps = atoi(argv[argi++]);
        if (steps < 0) {
            return false;
        }
        path = std::string(argv[argi++]);
        if (exists(path)) {
            return false;
        }
    } catch (...) {
        return false;
    }
    args.groupSize = groupSize;
    args.run = run;
    args.agents = agents;
    args.steps = steps;
    args.path = path;
    return true;
}

void update(std::vector<std::vector<int> >& distribution, genome::Genome* genome, int increment, int groupSize) {
    for (unsigned gene = 0; gene < distribution.size(); gene++) {
        distribution[gene][genome->get_raw_uint(gene) >> groupSize] += increment;
    }
}

double getDistance(const std::vector<std::vector<int> >& distribution, int population, int groupSize) {
    long distance = 0;
    int groupCount = 256 >> groupSize;
    int expected = population / groupCount;
    for (unsigned gene = 0; gene < distribution.size(); gene++) {
        for (int allele = 0; allele < groupCount; allele++) {
            distance += abs(distribution[gene][allele] - expected);
        }
    }
    return (double)distance / (expected * (groupCount - 2) + population) / distribution.size();
}

void logGenome(const std::string& path, genome::Genome* genome) {
    AbstractFile* file = AbstractFile::open(globals::recordFileType, path.c_str(), "w");
    genome->dump(file);
    delete file;
}
