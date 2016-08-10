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
    std::string stage;
    int count;
    int transient;
    int steps;
};

bool tryParseArgs(int, char**, Args&);
void printArgs(const Args&);
void initialize(const std::string&);
AbstractFile* getSynapses(const std::string&, const std::string&, int);
Genome* loadGenome(const std::string&, int);
RqNervousSystem* loadNervousSystem(Genome*, AbstractFile*);
void printHeader(int, NervousSystem*);
void printSynapses(NervousSystem*);
void printTimeSeries(NervousSystem*, int, int);

int main(int argc, char** argv) {
    Args args;
    if (!tryParseArgs(argc, argv, args)) {
        std::cerr << "Usage: " << argv[0] << " RUN STAGE COUNT TRANSIENT STEPS" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Generates neural activation time series using random inputs." << std::endl;
        std::cerr << std::endl;
        std::cerr << "  RUN        Run directory" << std::endl;
        std::cerr << "  STAGE      Life stage (incept, birth, or death)" << std::endl;
        std::cerr << "  COUNT      Number of time series per agent" << std::endl;
        std::cerr << "  TRANSIENT  Initial number of timesteps to ignore" << std::endl;
        std::cerr << "  STEPS      Number of timesteps to display" << std::endl;
        return 1;
    }
    printArgs(args);
    initialize(args.run);
    DataLibReader lifeSpans((args.run + "/lifespans.txt").c_str());
    lifeSpans.seekTable("LifeSpans");
    while (lifeSpans.nextRow()) {
        int agent = lifeSpans.col("Agent");
        AbstractFile* synapses = getSynapses(args.run, args.stage, agent);
        if (synapses != NULL) {
            Genome* genome = loadGenome(args.run, agent);
            RqNervousSystem* cns = loadNervousSystem(genome, synapses);
            delete genome;
            printHeader(agent, cns);
            printSynapses(cns);
            std::cout << "# BEGIN ENSEMBLE" << std::endl;
            for (int index = 0; index < args.count; index++) {
                printTimeSeries(cns, args.transient, args.steps);
            }
            std::cout << "# END ENSEMBLE" << std::endl;
            delete cns;
        }
        delete synapses;
    }
    return 0;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    if (argc != 6) {
        return false;
    }
    std::string run;
    std::string stage;
    int count;
    int transient;
    int steps;
    try {
        run = std::string(argv[1]);
        if (!exists(run + "/endStep.txt")) {
            return false;
        }
        stage = std::string(argv[2]);
        if (stage != "incept" && stage != "birth" && stage != "death") {
            return false;
        }
        count = atoi(argv[3]);
        if (count < 1) {
            return false;
        }
        transient = atoi(argv[4]);
        if (transient < 0) {
            return false;
        }
        steps = atoi(argv[5]);
        if (steps < 1) {
            return false;
        }
    } catch (...) {
        return false;
    }
    args.run = run;
    args.stage = stage;
    args.count = count;
    args.transient = transient;
    args.steps = steps;
    return true;
}

void printArgs(const Args& args) {
    std::cout << "# BEGIN ARGUMENTS" << std::endl;
    std::cout << "stage = " << args.stage << std::endl;
    std::cout << "count = " << args.count << std::endl;
    std::cout << "transient = " << args.transient << std::endl;
    std::cout << "steps = " << args.steps << std::endl;
    std::cout << "# END ARGUMENTS" << std::endl;
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

AbstractFile* getSynapses(const std::string& run, const std::string& stage, int agent) {
    std::string path = run + "/brain/synapses/synapses_" + std::to_string(agent) + "_" + stage + ".txt";
    if (AbstractFile::exists(path.c_str())) {
        return AbstractFile::open(globals::recordFileType, path.c_str(), "r");
    } else {
        return NULL;
    }
}

Genome* loadGenome(const std::string& run, int agent) {
    std::string path = run + "/genome/agents/genome_" + std::to_string(agent) + ".txt";
    AbstractFile* file = AbstractFile::open(globals::recordFileType, path.c_str(), "r");
    Genome* genome = GenomeUtil::createGenome();
    genome->load(file);
    delete file;
    return genome;
}

RqNervousSystem* loadNervousSystem(Genome* genome, AbstractFile* synapses) {
    RqNervousSystem* cns = new RqNervousSystem();
    cns->grow(genome);
    cns->getBrain()->loadSynapses(synapses);
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

void printSynapses(NervousSystem* cns) {
    std::map<short, std::set<short> > synapses;
    NeuronModel::Dimensions dims = cns->getBrain()->getDimensions();
    NeuronModel* model = cns->getBrain()->getNeuronModel();
    for (int synapse = 0; synapse < dims.numSynapses; synapse++) {
        short neuron1;
        short neuron2;
        float weight;
        float learningRate;
        model->get_synapse(synapse, neuron1, neuron2, weight, learningRate);
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
    cns->getBrain()->randomizeActivations();
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
