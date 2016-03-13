#include <iostream>
#include <stdlib.h>
#include <string>
#include <time.h>

#include "agent/agent.h"
#include "brain/Brain.h"
#include "brain/NervousSystem.h"
#include "brain/NeuronModel.h"
#include "brain/RqNervousSystem.h"
#include "genome/Genome.h"
#include "genome/GenomeSchema.h"
#include "genome/GenomeUtil.h"
#include "proplib/builder.h"
#include "proplib/dom.h"
#include "proplib/interpreter.h"
#include "proplib/schema.h"
#include "sim/globals.h"
#include "utils/AbstractFile.h"
#include "utils/datalib.h"
#include "utils/misc.h"

using namespace genome;

struct Args {
    std::string run;
    int steps;
};

bool tryParseArgs(int, char**, Args&);
void printUsage(int, char**);
void initialize(const std::string&);
Genome* loadGenome(const std::string&, int);
RqNervousSystem* loadNervousSystem(Genome*, const std::string&, int);

int main(int argc, char** argv) {
    Args args;
    if (!tryParseArgs(argc, argv, args)) {
        printUsage(argc, argv);
        return 1;
    }
    initialize(args.run);
    DataLibReader lifeSpans((args.run + "/lifespans.txt").c_str());
    lifeSpans.seekTable("LifeSpans");
    while (lifeSpans.nextRow()) {
        int agent = lifeSpans.col("Agent");
        Genome* genome = loadGenome(args.run, agent);
        RqNervousSystem* cns = loadNervousSystem(genome, args.run, agent);
        delete genome;
        NeuronModel::Dimensions dims = cns->getBrain()->getDimensions();
        double* activations = new double[dims.numNeurons];
        std::cout << "# BEGIN";
        std::cout << " agent=" << agent;
        std::cout << " numNeurons=" << dims.numNeurons;
        std::cout << " numInputNeurons=" << dims.numInputNeurons;
        std::cout << " numOutputNeurons=" << dims.numOutputNeurons;
        std::cout << std::endl;
        for (int step = 1; step <= args.steps; step++) {
            cns->update(false);
            cns->getBrain()->getActivations(activations, 0, dims.numNeurons);
            for (int neuron = 0; neuron < dims.numNeurons; neuron++) {
                if (neuron > 0) {
                    std::cout << " ";
                }
                std::cout << activations[neuron];
            }
            std::cout << std::endl;
        }
        std::cout << "# END" << std::endl;
        delete[] activations;
        delete cns;
    }
    return 0;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    if (argc != 3) {
        return false;
    }
    try {
        args.run = std::string(argv[1]);
        if (!exists(args.run + "/endStep.txt")) {
            return false;
        }
        args.steps = atoi(argv[2]);
        if (args.steps < 1) {
            return false;
        }
    } catch (...) {
        return false;
    }
    return true;
}

void printUsage(int argc, char** argv) {
    std::cerr << "Usage: " << argv[0] << " RUN STEPS" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Generates brain function files using random inputs." << std::endl;
    std::cerr << std::endl;
    std::cerr << "  RUN    Run directory" << std::endl;
    std::cerr << "  STEPS  Number of timesteps" << std::endl;
}

void initialize(const std::string& run) {
    proplib::Interpreter::init();
    proplib::DocumentBuilder builder;
    proplib::SchemaDocument* schema;
    proplib::Document* worldfile;
    schema = builder.buildSchemaDocument(run + "/original.wfs");
    worldfile = builder.buildWorldfileDocument(schema, run + "/original.wf");
    schema->apply(worldfile);
    globals::recordFileType = (bool)worldfile->get("CompressFiles") ? AbstractFile::TYPE_GZIP_FILE : AbstractFile::TYPE_FILE;
    agent::processWorldfile(*worldfile);
    genome::GenomeSchema::processWorldfile(*worldfile);
    Brain::processWorldfile(*worldfile);
    proplib::Interpreter::dispose();
    delete worldfile;
    delete schema;
    Brain::init();
    genome::GenomeUtil::createSchema();
    srand48(time(NULL));
}

Genome* loadGenome(const std::string& run, int agent) {
    std::string path = run + "/genome/agents/genome_" + std::to_string(agent) + ".txt";
    AbstractFile* file = AbstractFile::open(globals::recordFileType, path.c_str(), "r");
    Genome* genome = GenomeUtil::createGenome();
    genome->load(file);
    delete file;
    return genome;
}

RqNervousSystem* loadNervousSystem(Genome* genome, const std::string& run, int agent) {
    RqNervousSystem* cns = new RqNervousSystem();
    cns->grow(genome);
    std::string path = run + "/brain/synapses/synapses_" + std::to_string(agent) + ".txt";
    AbstractFile* file = AbstractFile::open(globals::recordFileType, path.c_str(), "r");
    cns->getBrain()->loadSynapses(file);
    delete file;
    cns->prebirth();
    cns->getBrain()->freeze();
    return cns;
}
