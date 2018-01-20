#include <assert.h>
#include <iostream>
#include <iterator>
#include <limits.h>
#include <list>
#include <map>
#include <set>
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
#include "utils/datalib.h"
#include "utils/misc.h"

#define COMMA ,

struct Args {
    std::string driven;
    std::string passive;
    int batch;
};

struct Event {
    std::string type;
    int agent;
};

void printUsage(int, char**);
bool tryParseArgs(int, char**, Args&);
void copyDir(const std::string&, const std::string&, const std::string&);
void copyFile(const std::string&, const std::string&, const std::string&);
void copyAbstractFile(const std::string&, const std::string&, std::string);
std::map<int, std::list<Event> > getEvents(const std::string&);
std::map<int, genome::Genome*>::iterator pick(std::map<int, genome::Genome*>&);
void logGenome(const std::string&, int, genome::Genome*);
void logSynapses(const std::string&, int, const std::string&, RqNervousSystem*);

int main(int argc, char** argv) {
    Args args;
    if (!tryParseArgs(argc, argv, args)) {
        printUsage(argc, argv);
        return 1;
    }
    analysis::initialize(args.driven);
    makeDirs(args.passive);
    copyFile(args.driven, args.passive, "/endStep.txt");
    copyFile(args.driven, args.passive, "/original.wfs");
    copyFile(args.driven, args.passive, "/original.wf");
    copyFile(args.driven, args.passive, "/normalized.wf");
    makeDirs(args.passive + "/genome");
    copyDir(args.driven, args.passive, "/genome/meta");
    makeDirs(args.passive + "/genome/agents");
    makeDirs(args.passive + "/brain/synapses");
    std::map<int, int> births;
    std::set<int> drivens;
    std::map<int, genome::Genome*> passives;
    ofstream log(args.passive + "/BirthsDeaths.log");
    log << "% Timestep Event Agent# Parent1 Parent2" << std::endl;
    DataLibWriter writer((args.passive + "/lifespans.txt").c_str());
    const char* columnNames[] = { "Agent", "BirthStep", "BirthReason", "DeathStep", "DeathReason", NULL };
    const datalib::Type columnTypes[] = { datalib::INT, datalib::INT, datalib::STRING, datalib::INT, datalib::STRING };
    writer.beginTable("LifeSpans", columnNames, columnTypes);
    int initAgentCount = analysis::getInitAgentCount(args.driven);
    for (int agent = 1; agent <= initAgentCount; agent++) {
        births[agent] = 0;
        drivens.insert(agent);
        passives[agent] = analysis::getGenome(args.driven, agent);
        copyAbstractFile(args.driven, args.passive, "/genome/agents/genome_" + std::to_string(agent) + ".txt");
        if (Brain::config.learningMode != Brain::Configuration::LEARN_NONE) {
            copyAbstractFile(args.driven, args.passive, "/brain/synapses/synapses_" + std::to_string(agent) + "_incept.txt");
        }
        copyAbstractFile(args.driven, args.passive, "/brain/synapses/synapses_" + std::to_string(agent) + "_birth.txt");
    }
    std::map<int, std::list<Event> > events = getEvents(args.driven);
    int maxTimestep = analysis::getMaxTimestep(args.driven);
    for (int timestep = 1; timestep <= maxTimestep; timestep++) {
        if (timestep % 100 == 0) {
            std::cerr << timestep << std::endl;
        }
        std::map<int, std::list<Event> >::iterator eventsIter = events.find(timestep);
        if (eventsIter != events.end()) {
            itfor(std::list<Event>, eventsIter->second, timestepEventsIter) {
                Event event = *timestepEventsIter;
                if (event.type == "BIRTH") {
                    births[event.agent] = timestep;
                    drivens.insert(event.agent);
                    std::map<int, genome::Genome*>::iterator parent1 = pick(passives);
                    std::map<int, genome::Genome*>::iterator parent2;
                    do {
                        parent2 = pick(passives);
                    } while (parent2->first == parent1->first);
                    genome::Genome* child = genome::GenomeUtil::createGenome();
                    child->crossover(parent1->second, parent2->second, true);
                    passives[event.agent] = child;
                    log << timestep << " BIRTH " << event.agent << " " << parent1->first << " " << parent2->first << std::endl;
                    logGenome(args.passive, event.agent, child);
                    RqNervousSystem* cns = new RqNervousSystem();
                    cns->grow(child);
                    if (Brain::config.learningMode != Brain::Configuration::LEARN_NONE) {
                        logSynapses(args.passive, event.agent, "incept", cns);
                    }
                    cns->prebirth();
                    logSynapses(args.passive, event.agent, "birth", cns);
                    delete cns;
                } else if (event.type == "DEATH") {
                    drivens.erase(event.agent);
                    std::map<int, genome::Genome*>::iterator genome = pick(passives);
                    passives.erase(genome->first);
                    delete genome->second;
                    log << timestep << " DEATH " << genome->first << std::endl;
                    writer.addRow(genome->first, births[genome->first], "PASSIVE", timestep, "PASSIVE");
                } else {
                    std::cerr << "Unhandled event: " << event.type << std::endl;
                }
            }
        }
        if (timestep % args.batch == 0) {
            itfor(std::map<int COMMA genome::Genome*>, passives, passivesIter) {
                delete passivesIter->second;
            }
            passives.clear();
            itfor(std::set<int>, drivens, drivensIter) {
                passives[*drivensIter] = analysis::getGenome(args.driven, *drivensIter);
            }
        }
    }
    itfor(std::map<int COMMA genome::Genome*>, passives, passivesIter) {
        writer.addRow(passivesIter->first, births[passivesIter->first], "PASSIVE", maxTimestep, "PASSIVE");
        delete passivesIter->second;
    }
    log.close();
}

void printUsage(int argc, char** argv) {
    std::cerr << "Usage: " << argv[0] << " DRIVEN PASSIVE [BATCH]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Generates selection-agnostic agents paired to a given run." << std::endl;
    std::cerr << std::endl;
    std::cerr << "  DRIVEN   Driven run directory" << std::endl;
    std::cerr << "  PASSIVE  Passive run directory" << std::endl;
    std::cerr << "  BATCH    Number of timesteps per batch" << std::endl;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    if (argc < 3 || argc > 4) {
        return false;
    }
    std::string driven;
    std::string passive;
    int batch = INT_MAX;
    try {
        int argi = 1;
        driven = std::string(argv[argi++]);
        if (!exists(driven + "/endStep.txt")) {
            return false;
        }
        passive = std::string(argv[argi++]);
        if (exists(passive)) {
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
    args.driven = driven;
    args.passive = passive;
    args.batch = batch;
    return true;
}

void copyDir(const std::string& run1, const std::string& run2, const std::string& path) {
    SYSTEM(("cp -R " + (run1 + path) + " " + (run2 + path)).c_str());
}

void copyFile(const std::string& run1, const std::string& run2, const std::string& path) {
    SYSTEM(("cp " + (run1 + path) + " " + (run2 + path)).c_str());
}

void copyAbstractFile(const std::string& run1, const std::string& run2, std::string path) {
    if (globals::recordFileType == AbstractFile::TYPE_GZIP_FILE) {
        path += ".gz";
    }
    SYSTEM(("cp " + (run1 + path) + " " + (run2 + path)).c_str());
}

std::map<int, std::list<Event> > getEvents(const std::string& run) {
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

std::map<int, genome::Genome*>::iterator pick(std::map<int, genome::Genome*>& genomes) {
    std::map<int, genome::Genome*>::iterator genome = genomes.begin();
    std::advance(genome, (int)rrand(0, genomes.size()));
    return genome;
}

void logGenome(const std::string& run, int agent, genome::Genome* genome) {
    std::string path = run + "/genome/agents/genome_" + std::to_string(agent) + ".txt";
    AbstractFile* file = AbstractFile::open(globals::recordFileType, path.c_str(), "w");
    genome->dump(file);
    delete file;
}

void logSynapses(const std::string& run, int agent, const std::string& stage, RqNervousSystem* cns) {
    std::string path = run + "/brain/synapses/synapses_" + std::to_string(agent) + "_" + stage + ".txt";
    AbstractFile* file = AbstractFile::open(globals::recordFileType, path.c_str(), "w");
    cns->getBrain()->dumpSynapses(file, agent);
    delete file;
}
