#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "brain/RqNervousSystem.h"
#include "genome/Genome.h"
#include "utils/AbstractFile.h"
#include "utils/analysis.h"

struct Args {
    std::string mode;
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
    int maxAgent = args.mode == "single" ? args.agent : analysis::getMaxAgent(args.run);
    for (int agent = args.agent; agent <= maxAgent; agent++) {
        AbstractFile* synapses = analysis::getSynapses(args.run, agent, args.stage);
        if (synapses == NULL) {
            continue;
        }
        genome::Genome* genome = analysis::getGenome(args.run, agent);
        RqNervousSystem* cns = analysis::getNervousSystem(genome, synapses);
        if (args.mode == "all") {
            double expansion = analysis::getExpansion(genome, cns, args.perturbation, args.repeats, args.random, args.quiescent, args.steps);
            std::cout << agent << " " << expansion << std::endl;
        } else if (args.mode == "single") {
            for (float wmax = args.wmaxMin; wmax <= args.wmaxMax; wmax += args.wmaxInc) {
                synapses->seek(0, SEEK_SET);
                analysis::setMaxWeight(cns, synapses, wmax);
                double expansion = analysis::getExpansion(genome, cns, args.perturbation, args.repeats, args.random, args.quiescent, args.steps);
                std::cout << wmax << " " << expansion << std::endl;
            }
        } else if (args.mode == "onset") {
            float wmaxInc = args.wmaxInc;
            int stage = 1;
            bool done = false;
            for (float wmax = args.wmaxMin; wmax <= args.wmaxMax && !done; wmax += wmaxInc) {
                synapses->seek(0, SEEK_SET);
                analysis::setMaxWeight(cns, synapses, wmax);
                double expansion = analysis::getExpansion(genome, cns, args.perturbation, args.repeats, args.random, args.quiescent, args.steps);
                if (expansion >= args.threshold) {
                    if (stage == 2) {
                        wmax -= wmaxInc;
                        wmaxInc = args.wmaxInc;
                        stage = 3;
                    } else {
                        std::cout << agent << " " << wmax << std::endl;
                        done = true;
                    }
                } else {
                    if (stage == 1 && wmax >= args.wmaxMax / 10.0f) {
                        wmaxInc = args.wmaxInc * 10.0f;
                        stage = 2;
                    }
                }
            }
            if (!done) {
                std::cout << agent << " -" << std::endl;
            }
        }
        delete cns;
        delete genome;
        delete synapses;
    }
    return 0;
}

void printUsage(int argc, char** argv) {
    std::cerr << "Usage:" << std::endl;
    std::cerr << "  " << argv[0] << " all RUN STAGE PERTURBATION REPEATS RANDOM QUIESCENT STEPS [AGENT]" << std::endl;
    std::cerr << "  " << argv[0] << " single RUN STAGE WMAX_MIN WMAX_MAX WMAX_INC PERTURBATION REPEATS RANDOM QUIESCENT STEPS AGENT" << std::endl;
    std::cerr << "  " << argv[0] << " onset RUN STAGE PERTURBATION REPEATS RANDOM QUIESCENT STEPS THRESHOLD [AGENT]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Calculates phase space expansion." << std::endl;
    std::cerr << std::endl;
    std::cerr << "  RUN           Run directory" << std::endl;
    std::cerr << "  STAGE         Life stage (incept, birth, or death)" << std::endl;
    std::cerr << "  WMAX_MIN      Minimum value of maximum synaptic weight" << std::endl;
    std::cerr << "  WMAX_MAX      Maximum value of maximum synaptic weight" << std::endl;
    std::cerr << "  WMAX_INC      Increment for maximum synaptic weight" << std::endl;
    std::cerr << "  PERTURBATION  Magnitude of perturbation" << std::endl;
    std::cerr << "  REPEATS       Number of attempts per calculation" << std::endl;
    std::cerr << "  RANDOM        Number of random timesteps" << std::endl;
    std::cerr << "  QUIESCENT     Number of quiescent timesteps" << std::endl;
    std::cerr << "  STEPS         Number of calculation timesteps" << std::endl;
    std::cerr << "  THRESHOLD     Threshold phase space expansion" << std::endl;
    std::cerr << "  AGENT         [Starting] agent index" << std::endl;
}

bool tryParseArgs(int argc, char** argv, Args& args) {
    std::string mode;
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
    int agent;
    try {
        if (argc < 2) {
            return false;
        }
        int argi = 1;
        mode = std::string(argv[argi++]);
        if (mode == "all") {
            if (argc < 9 || argc > 10) {
                return false;
            }
        } else if (mode == "single") {
            if (argc != 13) {
                return false;
            }
        } else if (mode == "onset") {
            if (argc < 10 || argc > 11) {
                return false;
            }
        }
        run = std::string(argv[argi++]);
        if (!exists(run + "/endStep.txt")) {
            return false;
        }
        stage = std::string(argv[argi++]);
        if (stage != "incept" && stage != "birth" && stage != "death") {
            return false;
        }
        if (mode == "single") {
            wmaxMin = atof(argv[argi++]);
            if (wmaxMin <= 0.0f) {
                return false;
            }
            wmaxMax = atof(argv[argi++]);
            if (wmaxMax <= wmaxMin) {
                return false;
            }
            wmaxInc = atof(argv[argi++]);
            if (wmaxInc <= 0.0f) {
                return false;
            }
        } else if (mode == "onset") {
            wmaxMin = 1.0f;
            wmaxMax = 1000.0f;
            wmaxInc = 1.0f;
        } else {
            wmaxMin = 0.0f;
            wmaxMax = 0.0f;
            wmaxInc = 0.0f;
        }
        perturbation = atof(argv[argi++]);
        if (perturbation <= 0.0) {
            return false;
        }
        repeats = atoi(argv[argi++]);
        if (repeats < 1) {
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
        if (mode == "onset") {
            threshold = atof(argv[argi++]);
        } else {
            threshold = 0.0;
        }
        if (mode == "single" || argc > argi) {
            agent = atoi(argv[argi++]);
            if (agent < 1) {
                return false;
            }
        } else {
            agent = 1;
        }
    } catch (...) {
        return false;
    }
    args.mode = mode;
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
    args.agent = agent;
    return true;
}

void printArgs(const Args& args) {
    std::cout << "# stage = " << args.stage << std::endl;
    if (args.mode == "onset") {
        std::cout << "# wmax_min = " << args.wmaxMin << std::endl;
        std::cout << "# wmax_max = " << args.wmaxMax << std::endl;
        std::cout << "# wmax_inc = " << args.wmaxInc << std::endl;
    }
    std::cout << "# perturbation = " << args.perturbation << std::endl;
    std::cout << "# repeats = " << args.repeats << std::endl;
    std::cout << "# random = " << args.random << std::endl;
    std::cout << "# quiescent = " << args.quiescent << std::endl;
    std::cout << "# steps = " << args.steps << std::endl;
    if (args.mode == "onset") {
        std::cout << "# threshold = " << args.threshold << std::endl;
    }
}
