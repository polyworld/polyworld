#include <assert.h>
#include <iostream>
#include <iterator>
#include <map>
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

#define COMMA ,

struct Args {
    std::string run;
    int agents;
    int births;
    std::string path;
};

void printUsage(int, char**);
bool tryParseArgs(int, char**, Args&);
std::map<int, genome::Genome*>::iterator pick(std::map<int, genome::Genome*>&);
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
    std::map<int, genome::Genome*> genomes;
    int geneCount = genome::GenomeUtil::schema->getMutableSize();
    std::vector<std::vector<int> > distribution(geneCount, std::vector<int>(256));
    for (int agent = 1; agent <= args.agents; agent++) {
        genome::Genome* genome = analysis::getGenome(args.run, 1);
        genomes[agent] = genome;
        update(distribution, genome, 1);
    }
    std::cout << 0 << " " << getDistance(distribution, args.agents) << std::endl;
    for (int birth = 1; birth <= args.births; birth++) {
        int agent = args.agents + birth;
        std::map<int, genome::Genome*>::iterator parent1 = pick(genomes);
        std::map<int, genome::Genome*>::iterator parent2;
        do {
            parent2 = pick(genomes);
        } while (parent2->first == parent1->first);
        genome::Genome* genome = genome::GenomeUtil::createGenome();
        genome->crossover(parent1->second, parent2->second, true);
        genomes[agent] = genome;
        update(distribution, genome, 1);
        std::map<int, genome::Genome*>::iterator genomesIter = pick(genomes);
        genomes.erase(genomesIter->first);
        update(distribution, genomesIter->second, -1);
        delete genomesIter->second;
        std::cout << birth << " " << getDistance(distribution, args.agents) << std::endl;
    }
    makeDirs(args.path);
    itfor(std::map<int COMMA genome::Genome*>, genomes, genomesIter) {
        logGenome(args.path + "/genome_" + std::to_string(genomesIter->first) + ".txt", genomesIter->second);
    }
    return 0;
}

void printUsage(int argc, char** argv) {
    std::cerr << "Usage: " << argv[0] << " RUN AGENTS BIRTHS PATH" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Generates selection-agnostic agents assuming steady-state conditions." << std::endl;
    std::cerr << std::endl;
    std::cerr << "  RUN     Run directory" << std::endl;
    std::cerr << "  AGENTS  Number of agents in the population" << std::endl;
    std::cerr << "  BIRTHS  Number of births to simulate" << std::endl;
    std::cerr << "  PATH    Logged genome directory" << std::endl;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    if (argc != 5) {
        return false;
    }
    std::string run;
    int agents;
    int births;
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
        births = atoi(argv[argi++]);
        if (births < 0) {
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
    args.births = births;
    args.path = path;
    return true;
}

std::map<int, genome::Genome*>::iterator pick(std::map<int, genome::Genome*>& genomes) {
    std::map<int, genome::Genome*>::iterator genome = genomes.begin();
    std::advance(genome, (int)rrand(0, genomes.size()));
    return genome;
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
