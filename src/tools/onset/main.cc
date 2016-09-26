#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "brain/RqNervousSystem.h"
#include "genome/Genome.h"
#include "utils/AbstractFile.h"
#include "utils/analysis.h"

struct Args {
    std::string run;
    std::string stage;
    float wmaxMin;
    float wmaxMax;
    float wmaxInc;
    double perturbation;
    int repeats;
    int random;
    int quiescent;
    int steps;
    double threshold;
    int start;
};

bool tryParseArgs(int, char**, Args&);
void printArgs(const Args&);

int main(int argc, char** argv) {
    Args args;
    if (!tryParseArgs(argc, argv, args)) {
        std::cerr << "Usage: " << argv[0] << " RUN STAGE WMAX_MIN WMAX_MAX WMAX_INC PERTURBATION REPEATS RANDOM QUIESCENT STEPS THRESHOLD [START]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Calculates onset of criticality." << std::endl;
        std::cerr << std::endl;
        std::cerr << "  RUN           Run directory" << std::endl;
        std::cerr << "  STAGE         Life stage (incept, birth, or death)" << std::endl;
        std::cerr << "  WMAX_MIN      Minimum value of maximum synaptic weight" << std::endl;
        std::cerr << "  WMAX_MAX      Maximum value of maximum synaptic weight" << std::endl;
        std::cerr << "  WMAX_INC      Increment for maximum synaptic weight" << std::endl;
        std::cerr << "  PERTURBATION  Magnitude of perturbation" << std::endl;
        std::cerr << "  REPEATS       Number of calculation attempts per agent" << std::endl;
        std::cerr << "  RANDOM        Number of random timesteps" << std::endl;
        std::cerr << "  QUIESCENT     Number of quiescent timesteps" << std::endl;
        std::cerr << "  STEPS         Number of calculation timesteps" << std::endl;
        std::cerr << "  THRESHOLD     Threshold phase space expansion" << std::endl;
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
        bool success = false;
        for (float wmax = args.wmaxMin; wmax <= args.wmaxMax && !success; wmax += args.wmaxInc) {
            synapses->seek(0, SEEK_SET);
            analysis::setMaxWeight(cns, synapses, wmax);
            double expansion = analysis::getExpansion(genome, cns, args.perturbation, args.repeats, args.random, args.quiescent, args.steps);
            if (expansion >= args.threshold) {
                success = true;
                std::cout << agent << " " << wmax << std::endl;
            }
        }
        if (!success) {
            std::cout << "# " << agent << " -" << std::endl;
        }
        delete cns;
        delete genome;
        delete synapses;
    }
    return 0;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    if (argc < 12 || argc > 13) {
        return false;
    }
    std::string run;
    std::string stage;
    float wmaxMin;
    float wmaxMax;
    float wmaxInc;
    double perturbation;
    int repeats;
    int random;
    int quiescent;
    int steps;
    double threshold;
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
        wmaxMin = atof(argv[3]);
        if (wmaxMin <= 0.0) {
            return false;
        }
        wmaxMax = atof(argv[4]);
        if (wmaxMax <= wmaxMin) {
            return false;
        }
        wmaxInc = atof(argv[5]);
        if (wmaxInc <= 0.0) {
            return false;
        }
        perturbation = atof(argv[6]);
        if (perturbation <= 0.0) {
            return false;
        }
        repeats = atoi(argv[7]);
        if (repeats < 1) {
            return false;
        }
        random = atoi(argv[8]);
        if (random < 0) {
            return false;
        }
        quiescent = atoi(argv[9]);
        if (quiescent < 0) {
            return false;
        }
        steps = atoi(argv[10]);
        if (steps < 1) {
            return false;
        }
        threshold = atof(argv[11]);
        if (argc == 13) {
            start = atoi(argv[12]);
            if (start < 1) {
                return false;
            }
        }
    } catch (...) {
        return false;
    }
    args.run = run;
    args.stage = stage;
    args.wmaxMin = wmaxMin;
    args.wmaxMax = wmaxMax;
    args.wmaxInc = wmaxInc;
    args.perturbation = perturbation;
    args.repeats = repeats;
    args.random = random;
    args.quiescent = quiescent;
    args.steps = steps;
    args.threshold = threshold;
    args.start = start;
    return true;
}

void printArgs(const Args& args) {
    std::cout << "# stage = " << args.stage << std::endl;
    std::cout << "# wmax_min = " << args.wmaxMin << std::endl;
    std::cout << "# wmax_max = " << args.wmaxMax << std::endl;
    std::cout << "# wmax_inc = " << args.wmaxInc << std::endl;
    std::cout << "# perturbation = " << args.perturbation << std::endl;
    std::cout << "# repeats = " << args.repeats << std::endl;
    std::cout << "# random = " << args.random << std::endl;
    std::cout << "# quiescent = " << args.quiescent << std::endl;
    std::cout << "# steps = " << args.steps << std::endl;
    std::cout << "# threshold = " << args.threshold << std::endl;
}
