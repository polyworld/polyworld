#include <assert.h>
#include <iterator>
#include <limits.h>
#include <list>
#include <map>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <string.h>

#include "brain/Brain.h"
#include "brain/RqNervousSystem.h"
#include "genome/Genome.h"
#include "genome/GenomeUtil.h"
#include "sim/globals.h"
#include "utils/AbstractFile.h"
#include "utils/analysis.h"
#include "utils/misc.h"

#define COMMA ,

struct Args {
    std::string run;
    int batch;
};

struct Event {
    std::string type;
    int agent;
};

void printUsage(int, char**);
bool tryParseArgs(int, char**, Args&);
std::map<int, std::list<Event> > getEvents(std::string);
genome::Genome* getParent(std::map<int, genome::Genome*>&);
void logGenome(const std::string&, int, genome::Genome*);
void logSynapses(const std::string&, int, const std::string&, RqNervousSystem*);

int main(int argc, char** argv) {
    Args args;
    if (!tryParseArgs(argc, argv, args)) {
        printUsage(argc, argv);
        return 1;
    }
    makeDirs(args.run + "/passive");
    ofstream out(args.run + "/passive/batches.txt");
    analysis::initialize(args.run);
    makeDirs(args.run + "/passive/genome/agents");
    makeDirs(args.run + "/passive/brain/synapses");
    std::map<int, genome::Genome*> originalGenomes;
    int initAgentCount = analysis::getInitAgentCount(args.run);
    for (int agent = 1; agent <= initAgentCount; agent++) {
        std::string path;
        path = "/genome/agents/genome_" + std::to_string(agent) + ".txt";
        AbstractFile::link((args.run + path).c_str(), (args.run + "/passive" + path).c_str());
        if (Brain::config.learningMode != Brain::Configuration::LEARN_NONE) {
            path = "/brain/synapses/synapses_" + std::to_string(agent) + "_incept.txt";
            AbstractFile::link((args.run + path).c_str(), (args.run + "/passive" + path).c_str());
        }
        path = "/brain/synapses/synapses_" + std::to_string(agent) + "_birth.txt";
        AbstractFile::link((args.run + path).c_str(), (args.run + "/passive" + path).c_str());
        originalGenomes[agent] = analysis::getGenome(args.run, agent);
    }
    std::map<int, genome::Genome*> batchGenomes(originalGenomes);
    std::map<int, std::list<Event> > events = getEvents(args.run);
    int count = 0;
    int maxTimestep = analysis::getMaxTimestep(args.run);
    for (int timestep = 1; timestep <= maxTimestep; timestep++) {
        if (timestep % 100 == 0) {
            std::cerr << timestep << std::endl;
        }
        std::map<int, std::list<Event> >::iterator eventsIter = events.find(timestep);
        if (eventsIter != events.end()) {
            itfor(std::list<Event>, eventsIter->second, timestepEventsIter) {
                Event event = *timestepEventsIter;
                if (event.type == "BIRTH") {
                    genome::Genome* parent1 = getParent(batchGenomes);
                    genome::Genome* parent2;
                    do {
                        parent2 = getParent(batchGenomes);
                    } while (parent2 == parent1);
                    genome::Genome* child = genome::GenomeUtil::createGenome();
                    child->crossover(parent1, parent2, true);
                    logGenome(args.run, event.agent, child);
                    RqNervousSystem* cns = new RqNervousSystem();
                    cns->grow(child);
                    if (Brain::config.learningMode != Brain::Configuration::LEARN_NONE) {
                        logSynapses(args.run, event.agent, "incept", cns);
                    }
                    cns->prebirth();
                    logSynapses(args.run, event.agent, "birth", cns);
                    delete cns;
                    batchGenomes[event.agent] = child;
                    originalGenomes[event.agent] = analysis::getGenome(args.run, event.agent);
                    count++;
                    if (count % args.batch == 0) {
                        itfor(std::map<int COMMA genome::Genome*>, batchGenomes, batchGenomesIter) {
                            if (batchGenomesIter->second != originalGenomes[batchGenomesIter->first]) {
                                delete batchGenomesIter->second;
                            }
                        }
                        out << eventsIter->first << " " << (event.agent - args.batch + 1) << " " << event.agent << std::endl;
                        batchGenomes = originalGenomes;
                    }
                } else if (event.type == "DEATH") {
                    if (batchGenomes[event.agent] != originalGenomes[event.agent]) {
                        delete batchGenomes[event.agent];
                    }
                    delete originalGenomes[event.agent];
                    batchGenomes.erase(event.agent);
                    originalGenomes.erase(event.agent);
                } else {
                    assert(false);
                }
            }
        }
    }
    out.close();
}

void printUsage(int argc, char** argv) {
    std::cerr << "Usage: " << argv[0] << " RUN [BATCH]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Generates selection-agnostic agents paired to a given run." << std::endl;
    std::cerr << std::endl;
    std::cerr << "  RUN    Run directory" << std::endl;
    std::cerr << "  BATCH  Number of agents per batch" << std::endl;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    if (argc < 2 || argc > 3) {
        return false;
    }
    std::string run;
    int batch = INT_MAX;
    try {
        int argi = 1;
        run = std::string(argv[argi++]);
        if (!exists(run + "/endStep.txt")) {
            return false;
        }
        if (argc > argi) {
            batch = atoi(argv[argi++]);
            if (batch < 1) {
                return false;
            }
        }
    } catch (...) {
        return false;
    }
    args.run = run;
    args.batch = batch;
    return true;
}

std::map<int, std::list<Event> > getEvents(std::string run) {
    std::map<int, std::list<Event> > events;
    std::ifstream in(run + "/BirthsDeaths.log");
    std::string line;
    std::getline(in, line);
    while (std::getline(in, line)) {
        std::istringstream stream(line);
        Event event;
        int timestep;
        stream >> timestep >> event.type >> event.agent;
        if (stream) {
            events[timestep].push_back(event);
        } else {
            std::cerr << "Parse failed: " << line << std::endl;
        }
    }
    return events;
}

genome::Genome* getParent(std::map<int, genome::Genome*>& genomes) {
    std::map<int, genome::Genome*>::iterator parent = genomes.begin();
    std::advance(parent, (int)rrand(0, genomes.size()));
    return parent->second;
}

void logGenome(const std::string& run, int agent, genome::Genome* genome) {
    std::string path = run + "/passive/genome/agents/genome_" + std::to_string(agent) + ".txt";
    AbstractFile* file = AbstractFile::open(globals::recordFileType, path.c_str(), "w");
    genome->dump(file);
    delete file;
}

void logSynapses(const std::string& run, int agent, const std::string& stage, RqNervousSystem* cns) {
    std::string path = run + "/passive/brain/synapses/synapses_" + std::to_string(agent) + "_" + stage + ".txt";
    AbstractFile* file = AbstractFile::open(globals::recordFileType, path.c_str(), "w");
    cns->getBrain()->dumpSynapses(file, agent);
    delete file;
}
