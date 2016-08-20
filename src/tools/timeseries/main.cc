#include <iostream>
#include <map>
#include <set>
#include <stdlib.h>
#include <string>

#include "brain/Brain.h"
#include "brain/NeuronModel.h"
#include "brain/RqNervousSystem.h"
#include "utils/analysis.h"
#include "utils/misc.h"

struct Args {
    std::string run;
    std::string stage;
    int repeats;
    int transient;
    int steps;
    int start;
};

bool tryParseArgs(int, char**, Args&);
void printArgs(const Args&);
void printHeader(int, RqNervousSystem*);
void printSynapses(RqNervousSystem*);
void printTimeSeries(RqNervousSystem*, int, int);

int main(int argc, char** argv) {
    Args args;
    if (!tryParseArgs(argc, argv, args)) {
        std::cerr << "Usage: " << argv[0] << " RUN STAGE REPEATS TRANSIENT STEPS [START]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Generates neural activation time series using random inputs." << std::endl;
        std::cerr << std::endl;
        std::cerr << "  RUN        Run directory" << std::endl;
        std::cerr << "  STAGE      Life stage (incept, birth, or death)" << std::endl;
        std::cerr << "  REPEATS    Number of time series per agent" << std::endl;
        std::cerr << "  TRANSIENT  Initial number of timesteps to ignore" << std::endl;
        std::cerr << "  STEPS      Number of timesteps to display" << std::endl;
        std::cerr << "  START      Starting agent" << std::endl;
        return 1;
    }
    printArgs(args);
    analysis::initialize(args.run);
    int maxAgent = analysis::getMaxAgent(args.run);
    for (int agent = args.start; agent <= maxAgent; agent++) {
        RqNervousSystem* cns = analysis::getNervousSystem(args.run, agent, args.stage);
        if (cns != NULL) {
            cns->getBrain()->freeze();
            printHeader(agent, cns);
            printSynapses(cns);
            std::cout << "# BEGIN ENSEMBLE" << std::endl;
            for (int index = 0; index < args.repeats; index++) {
                printTimeSeries(cns, args.transient, args.steps);
            }
            std::cout << "# END ENSEMBLE" << std::endl;
            delete cns;
        }
    }
    return 0;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    if (argc < 6 || argc > 7) {
        return false;
    }
    std::string run;
    std::string stage;
    int repeats;
    int transient;
    int steps;
    int start = 1;
    try {
        run = std::string(argv[1]);
        if (!exists(run + "/endStep.txt")) {
            return false;
        }
        stage = std::string(argv[2]);
        if (stage != "incept" && stage != "birth" && stage != "death") {
            return false;
        }
        repeats = atoi(argv[3]);
        if (repeats < 1) {
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
        if (argc == 7) {
            start = atoi(argv[6]);
            if (start < 1) {
                return false;
            }
        }
    } catch (...) {
        return false;
    }
    args.run = run;
    args.stage = stage;
    args.repeats = repeats;
    args.transient = transient;
    args.steps = steps;
    args.start = start;
    return true;
}

void printArgs(const Args& args) {
    std::cout << "# BEGIN ARGUMENTS" << std::endl;
    std::cout << "stage = " << args.stage << std::endl;
    std::cout << "repeats = " << args.repeats << std::endl;
    std::cout << "transient = " << args.transient << std::endl;
    std::cout << "steps = " << args.steps << std::endl;
    std::cout << "# END ARGUMENTS" << std::endl;
}

void printHeader(int agent, RqNervousSystem* cns) {
    std::cout << "# AGENT " << agent << std::endl;
    NeuronModel::Dimensions dims = cns->getBrain()->getDimensions();
    std::cout << "# DIMENSIONS";
    std::cout << " " << dims.numNeurons;
    std::cout << " " << dims.numInputNeurons;
    std::cout << " " << dims.numOutputNeurons;
    std::cout << std::endl;
}

void printSynapses(RqNervousSystem* cns) {
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

void printTimeSeries(RqNervousSystem* cns, int transient, int steps) {
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
