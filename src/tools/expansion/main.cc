#include <iostream>
#include <stdlib.h>
#include <string>

#include "brain/RqNervousSystem.h"
#include "genome/Genome.h"
#include "utils/AbstractFile.h"
#include "utils/analysis.h"

struct Args {
    std::string run;
    std::string stage;
    double perturbation;
    int repeats;
    int random;
    int quiescent;
    int steps;
    int start;
};

bool tryParseArgs(int, char**, Args&);
void printArgs(const Args&);

int main(int argc, char** argv) {
    Args args;
    if (!tryParseArgs(argc, argv, args)) {
        std::cerr << "Usage: " << argv[0] << " RUN STAGE PERTURBATION REPEATS RANDOM QUIESCENT STEPS [START]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Calculates phase space expansion." << std::endl;
        std::cerr << std::endl;
        std::cerr << "  RUN           Run directory" << std::endl;
        std::cerr << "  STAGE         Life stage (incept, birth, or death)" << std::endl;
        std::cerr << "  PERTURBATION  Magnitude of perturbation" << std::endl;
        std::cerr << "  REPEATS       Number of calculation attempts per agent" << std::endl;
        std::cerr << "  RANDOM        Number of random timesteps" << std::endl;
        std::cerr << "  QUIESCENT     Number of quiescent timesteps" << std::endl;
        std::cerr << "  STEPS         Number of calculation timesteps" << std::endl;
        std::cerr << "  START         Starting agent" << std::endl;
        return 1;
    }
    printArgs(args);
    analysis::initialize(args.run);
    int maxAgent = analysis::getMaxAgent(args.run);
    for (int agent = args.start; agent <= maxAgent; agent++) {
        AbstractFile* synapses = analysis::getSynapses(args.run, agent, args.stage);
        if (synapses == NULL) {
            continue;
        }
        genome::Genome* genome = analysis::getGenome(args.run, agent);
        RqNervousSystem* cns = analysis::getNervousSystem(genome, synapses);
        double expansion = analysis::getExpansion(genome, cns, args.perturbation, args.repeats, args.random, args.quiescent, args.steps);
        std::cout << agent << " " << expansion << std::endl;
        delete cns;
        delete genome;
        delete synapses;
    }
    return 0;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    if (argc < 8 || argc > 9) {
        return false;
    }
    std::string run;
    std::string stage;
    double perturbation;
    int repeats;
    int random;
    int quiescent;
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
        perturbation = atof(argv[3]);
        if (perturbation <= 0.0) {
            return false;
        }
        repeats = atoi(argv[4]);
        if (repeats < 1) {
            return false;
        }
        random = atoi(argv[5]);
        if (random < 0) {
            return false;
        }
        quiescent = atoi(argv[6]);
        if (quiescent < 0) {
            return false;
        }
        steps = atoi(argv[7]);
        if (steps < 1) {
            return false;
        }
        if (argc == 9) {
            start = atoi(argv[8]);
            if (start < 1) {
                return false;
            }
        }
    } catch (...) {
        return false;
    }
    args.run = run;
    args.stage = stage;
    args.perturbation = perturbation;
    args.repeats = repeats;
    args.random = random;
    args.quiescent = quiescent;
    args.steps = steps;
    args.start = start;
    return true;
}

void printArgs(const Args& args) {
    std::cout << "# stage = " << args.stage << std::endl;
    std::cout << "# perturbation = " << args.perturbation << std::endl;
    std::cout << "# repeats = " << args.repeats << std::endl;
    std::cout << "# random = " << args.random << std::endl;
    std::cout << "# quiescent = " << args.quiescent << std::endl;
    std::cout << "# steps = " << args.steps << std::endl;
}
