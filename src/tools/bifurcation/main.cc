#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "brain/Brain.h"
#include "brain/NeuronModel.h"
#include "brain/RqNervousSystem.h"
#include "genome/Genome.h"
#include "utils/AbstractFile.h"
#include "utils/analysis.h"
#include "utils/misc.h"

struct Args {
    std::string run;
    std::string stage;
    float wmaxMin;
    float wmaxMax;
    int wmaxCount;
    int random;
    int quiescent;
    int steps;
    int agent;
};

void printUsage(int, char**);
bool tryParseArgs(int, char**, Args&);
void printArgs(const Args&);

int main(int argc, char** argv) {
    Args args;
    if (!tryParseArgs(argc, argv, args)) {
        printUsage(argc, argv);
        return 1;
    }
    printArgs(args);
    analysis::initialize(args.run);
    AbstractFile* synapses = analysis::getSynapses(args.run, args.agent, args.stage);
    if (synapses == NULL) {
        return 0;
    }
    genome::Genome* genome = analysis::getGenome(args.run, args.agent);
    RqNervousSystem* cns = analysis::getNervousSystem(genome, synapses);
    delete genome;
    cns->getBrain()->freeze();
    NeuronModel::Dimensions dims = cns->getBrain()->getDimensions();
    double* activations = new double[dims.numOutputNeurons];
    for (int index = 0; index < args.wmaxCount; index++) {
        float wmax = interp((float)index / (args.wmaxCount - 1), args.wmaxMin, args.wmaxMax);
        synapses->seek(0, SEEK_SET);
        analysis::setMaxWeight(cns, synapses, wmax);
        cns->getBrain()->randomizeActivations();
        cns->setMode(RqNervousSystem::RANDOM);
        for (int step = 1; step <= args.random; step++) {
            cns->update(false);
        }
        cns->setMode(RqNervousSystem::QUIESCENT);
        for (int step = 1; step <= args.quiescent; step++) {
            cns->update(false);
        }
        for (int step = 1; step <= args.steps; step++) {
            cns->update(false);
            cns->getBrain()->getActivations(activations, dims.getFirstOutputNeuron(), dims.numOutputNeurons);
            std::cout << wmax;
            for (int neuron = 0; neuron < dims.numOutputNeurons; neuron++) {
                std::cout << " " << activations[neuron];
            }
            std::cout << std::endl;
        }
    }
    delete[] activations;
    delete cns;
    delete synapses;
    return 0;
}

void printUsage(int argc, char** argv) {
    std::cerr << "Usage: " << argv[0] << " RUN STAGE WMAX_MIN WMAX_MAX WMAX_COUNT RANDOM QUIESCENT STEPS AGENT" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Generates data for bifurcation diagrams." << std::endl;
    std::cerr << std::endl;
    std::cerr << "  RUN         Run directory" << std::endl;
    std::cerr << "  STAGE       Life stage (incept, birth, or death)" << std::endl;
    std::cerr << "  WMAX_MIN    Minimum value of maximum synaptic weight" << std::endl;
    std::cerr << "  WMAX_MAX    Maximum value of maximum synaptic weight" << std::endl;
    std::cerr << "  WMAX_COUNT  Number of maximum synaptic weight values" << std::endl;
    std::cerr << "  RANDOM      Number of random timesteps" << std::endl;
    std::cerr << "  QUIESCENT   Number of quiescent timesteps" << std::endl;
    std::cerr << "  STEPS       Number of output timesteps" << std::endl;
    std::cerr << "  AGENT       Agent index" << std::endl;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    if (argc != 10) {
        return false;
    }
    std::string run;
    std::string stage;
    float wmaxMin;
    float wmaxMax;
    int wmaxCount;
    int random;
    int quiescent;
    int steps;
    int agent;
    try {
        int argi = 1;
        run = std::string(argv[argi++]);
        if (!exists(run + "/endStep.txt")) {
            return false;
        }
        stage = std::string(argv[argi++]);
        if (stage != "incept" && stage != "birth" && stage != "death") {
            return false;
        }
        wmaxMin = atof(argv[argi++]);
        if (wmaxMin < 0.0f) {
            return false;
        }
        wmaxMax = atof(argv[argi++]);
        if (wmaxMax <= wmaxMin) {
            return false;
        }
        wmaxCount = atoi(argv[argi++]);
        if (wmaxCount < 1) {
            return false;
        }
        random = atoi(argv[argi++]);
        if (random < 0) {
            return false;
        }
        quiescent = atoi(argv[argi++]);
        if (quiescent < 0) {
            return false;
        }
        steps = atoi(argv[argi++]);
        if (steps < 1) {
            return false;
        }
        agent = atoi(argv[argi++]);
        if (agent < 1) {
            return false;
        }
    } catch (...) {
        return false;
    }
    args.run = run;
    args.stage = stage;
    args.wmaxMin = wmaxMin;
    args.wmaxMax = wmaxMax;
    args.wmaxCount = wmaxCount;
    args.random = random;
    args.quiescent = quiescent;
    args.steps = steps;
    args.agent = agent;
    return true;
}

void printArgs(const Args& args) {
    std::cout << "# stage = " << args.stage << std::endl;
    std::cout << "# random = " << args.random << std::endl;
    std::cout << "# quiescent = " << args.quiescent << std::endl;
    std::cout << "# steps = " << args.steps << std::endl;
}
