#pragma once

#include <string>

#include "brain/RqNervousSystem.h"
#include "genome/Genome.h"
#include "utils/AbstractFile.h"

namespace analysis {
    void initialize(const std::string&);
    int getMaxAgent(const std::string&);
    genome::Genome* getGenome(const std::string&, int);
    AbstractFile* getSynapses(const std::string&, int, const std::string&);
    RqNervousSystem* getNervousSystem(genome::Genome*, AbstractFile*);
    RqNervousSystem* getNervousSystem(const std::string&, int, const std::string&);
}
