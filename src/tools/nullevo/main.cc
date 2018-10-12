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
    std::string run;
    int agents;
    int steps;
    std::string path;
};

void printUsage(int, char**);
bool tryParseArgs(int, char**, Args&);
void update(std::vector<std::vector<int> >&, genome::Genome*, int);
double getDistance(const std::vector<std::vector<int> >&, int);
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
    std::vector<std::vector<int> > distribution(geneCount, std::vector<int>(256));
    genome::Genome* seed = analysis::getGenome(args.run, 1);
    for (int agent = 1; agent <= args.agents; agent++) {
        genome::Genome* genome = genome::GenomeUtil::createGenome();
        genome->copyFrom(seed);
        genomes.push_back(genome);
    }
    update(distribution, seed, args.agents);
    std::cout << 0 << " " << getDistance(distribution, args.agents) << std::endl;
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
        update(distribution, birth, 1);
        update(distribution, death, -1);
        delete death;
        if (step % BATCH == 0) {
            std::cout << step << " " << getDistance(distribution, args.agents) << std::endl;
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
    std::cerr << "Usage: " << argv[0] << " RUN AGENTS STEPS PATH" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Generates selection-agnostic agents assuming steady-state conditions." << std::endl;
    std::cerr << std::endl;
    std::cerr << "  RUN     Run directory" << std::endl;
    std::cerr << "  AGENTS  Number of agents in the population" << std::endl;
    std::cerr << "  STEPS   Number of steps to simulate" << std::endl;
    std::cerr << "  PATH    Logged genome directory" << std::endl;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    if (argc != 5) {
        return false;
    }
    std::string run;
    int agents;
    int steps;
    std::string path;
    try {
        int argi = 1;
        run = std::string(argv[argi++]);
        if (!exists(run + "/endStep.txt")) {
            return false;
        }
        agents = atoi(argv[argi++]);
        if (agents < 2) {
            return false;
        }
        agents = 256 * (int)ceil(agents / 256.0);
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
    args.run = run;
    args.agents = agents;
    args.steps = steps;
    args.path = path;
    return true;
}

void update(std::vector<std::vector<int> >& distribution, genome::Genome* genome, int increment) {
    for (unsigned gene = 0; gene < distribution.size(); gene++) {
        distribution[gene][genome->get_raw_uint(gene)] += increment;
    }
}

double getDistance(const std::vector<std::vector<int> >& distribution, int population) {
    long distance = 0;
    int expected = population / 256;
    for (unsigned gene = 0; gene < distribution.size(); gene++) {
        for (unsigned allele = 0; allele < 256; allele++) {
            distance += abs(distribution[gene][allele] - expected);
        }
    }
    return (double)distance / (expected * 254 + population) / distribution.size();
}

void logGenome(const std::string& path, genome::Genome* genome) {
    AbstractFile* file = AbstractFile::open(globals::recordFileType, path.c_str(), "w");
    genome->dump(file);
    delete file;
}
