#include <iostream>
#include <stdlib.h>
#include <string>

#include "brain/Brain.h"
#include "brain/Nerve.h"
#include "brain/NeuronModel.h"
#include "brain/RqNervousSystem.h"
#include "utils/analysis.h"
#include "utils/misc.h"

struct Args {
    std::string run;
};

void printUsage(int, char**);
bool tryParseArgs(int, char**, Args&);

int main(int argc, char** argv) {
    Args args;
    if (!tryParseArgs(argc, argv, args)) {
        printUsage(argc, argv);
        return 1;
    }
    analysis::initialize(args.run);
    int maxAgent = analysis::getMaxAgent(args.run);
    for (int agent = 1; agent <= maxAgent; agent++) {
        RqNervousSystem* cns = analysis::getNervousSystem(args.run, agent, "birth");
        const NervousSystem::NerveList& nerves = cns->getNerves();
        citfor(NervousSystem::NerveList, nerves, it) {
            std::cout << agent << " " << (*it)->name << " " << (*it)->getNeuronCount() << std::endl;
        }
        NeuronModel::Dimensions dimensions = cns->getBrain()->getDimensions();
        std::cout << agent << " Input " << dimensions.numInputNeurons << std::endl;
        std::cout << agent << " Output " << dimensions.numOutputNeurons << std::endl;
        std::cout << agent << " Internal " << dimensions.numNeurons - dimensions.numInputNeurons - dimensions.numOutputNeurons << std::endl;
        std::cout << agent << " Processing " << dimensions.numNeurons - dimensions.numInputNeurons << std::endl;
        std::cout << agent << " Total " << dimensions.numNeurons << std::endl;
        delete cns;
    }
    return 0;
}

void printUsage(int argc, char** argv) {
    std::cerr << "Usage: " << argv[0] << " RUN" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Prints neuron counts by type." << std::endl;
    std::cerr << std::endl;
    std::cerr << "  RUN  Run directory" << std::endl;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    if (argc != 2) {
        return false;
    }
    std::string run;
    try {
        int argi = 1;
        run = std::string(argv[argi++]);
        if (!exists(run + "/endStep.txt")) {
            return false;
        }
    } catch (...) {
        return false;
    }
    args.run = run;
    return true;
}
