#include <iostream>
#include <map>
#include <math.h>
#include <set>
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
    int count;
    int transient;
    int steps;
    float threshold;
};

bool tryParseArgs(int, char**, Args&);
void initialize(const std::string&);
Genome* loadGenome(const std::string&, int);
RqNervousSystem* loadNervousSystem(Genome*, const std::string&, int);
void printHeader(int, NervousSystem*);
void printSynapses(NervousSystem*, float);
void printTimeSeries(NervousSystem*, int, int);

int main(int argc, char** argv) {
    Args args;
    if (!tryParseArgs(argc, argv, args)) {
        std::cerr << "Usage: " << argv[0] << " RUN COUNT TRANSIENT STEPS [THRESHOLD]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Generates neural activation time series using random inputs." << std::endl;
        std::cerr << std::endl;
        std::cerr << "  RUN        Run directory" << std::endl;
        std::cerr << "  COUNT      Number of time series per agent" << std::endl;
        std::cerr << "  TRANSIENT  Initial number of timesteps to ignore" << std::endl;
        std::cerr << "  STEPS      Number of timesteps to display" << std::endl;
        std::cerr << "  THRESHOLD  Threshold synaptic weight" << std::endl;
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
        printHeader(agent, cns);
        printSynapses(cns, args.threshold);
        std::cout << "# BEGIN ENSEMBLE" << std::endl;
        for (int index = 0; index < args.count; index++) {
            printTimeSeries(cns, args.transient, args.steps);
        }
        std::cout << "# END ENSEMBLE" << std::endl;
        delete cns;
    }
    return 0;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    if (argc < 5 || argc > 6) {
        return false;
    }
    std::string run;
    int count;
    int transient;
    int steps;
    float threshold = 1e-6f;
    try {
        run = std::string(argv[1]);
        if (!exists(run + "/endStep.txt")) {
            return false;
        }
        count = atoi(argv[2]);
        if (count < 1) {
            return false;
        }
        transient = atoi(argv[3]);
        if (transient < 0) {
            return false;
        }
        steps = atoi(argv[4]);
        if (steps < 1) {
            return false;
        }
        if (argc == 6) {
            threshold = atof(argv[5]);
            if (threshold < 0.0f) {
                return false;
            }
        }
    } catch (...) {
        return false;
    }
    args.run = run;
    args.count = count;
    args.transient = transient;
    args.steps = steps;
    args.threshold = threshold;
    return true;
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

void printHeader(int agent, NervousSystem* cns) {
    std::cout << "# AGENT " << agent << std::endl;
    NeuronModel::Dimensions dims = cns->getBrain()->getDimensions();
    std::cout << "# DIMENSIONS";
    std::cout << " " << dims.numNeurons;
    std::cout << " " << dims.numInputNeurons;
    std::cout << " " << dims.numOutputNeurons;
    std::cout << std::endl;
}

void printSynapses(NervousSystem* cns, float threshold) {
    std::map<short, std::set<short> > synapses;
    NeuronModel::Dimensions dims = cns->getBrain()->getDimensions();
    NeuronModel* model = cns->getBrain()->getNeuronModel();
    for (int synapse = 0; synapse < dims.numSynapses; synapse++) {
        short neuron1;
        short neuron2;
        float weight;
        float learningRate;
        model->get_synapse(synapse, neuron1, neuron2, weight, learningRate);
        if (fabs(weight) < threshold) {
            continue;
        }
        synapses[neuron1].insert(neuron2);
    }
    std::cout << "# BEGIN SYNAPSES" << std::endl;
    for (int neuron = 0; neuron < dims.numNeurons; neuron++) {
        if (synapses[neuron].size() == 0) {
            continue;
        }
        std::cout << neuron;
        citfor(std::set<short>, synapses[neuron], it) {
            std::cout << " " << *it;
        }
        std::cout << std::endl;
    }
    std::cout << "# END SYNAPSES" << std::endl;
}

void printTimeSeries(NervousSystem* cns, int transient, int steps) {
    for (int step = 1; step <= transient; step++) {
        cns->update(false);
    }
    NeuronModel::Dimensions dims = cns->getBrain()->getDimensions();
    double* activations = new double[dims.numNeurons];
    std::cout << "# BEGIN TIME SERIES" << std::endl;
    for (int step = 1; step <= steps; step++) {
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
    std::cout << "# END TIME SERIES" << std::endl;
    delete[] activations;
}
