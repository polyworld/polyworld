#pragma once

#include <string>

#include "brain/RqNervousSystem.h"
#include "genome/Genome.h"
#include "utils/AbstractFile.h"

namespace analysis {
    class Vector {
    public:
        static void add(Vector&, Vector&, Vector&);
        static void subtract(Vector&, Vector&, Vector&);
        
        int size;
        double* values;
        
        Vector(int);
        ~Vector();
        void reset();
        void randomize(double, double);
        double getMagnitude();
        void scaleBy(double);
        void scaleTo(double);
    };
    
    void initialize(const std::string&);
    int getMaxTimestep(const std::string&);
    int getInitAgentCount(const std::string&);
    int getMaxAgent(const std::string&);
    genome::Genome* getGenome(const std::string&, int, bool = false);
    AbstractFile* getSynapses(const std::string&, int, const std::string&, bool = false);
    RqNervousSystem* getNervousSystem(genome::Genome*, AbstractFile*);
    RqNervousSystem* getNervousSystem(const std::string&, int, const std::string&, bool = false);
    RqNervousSystem* copyNervousSystem(genome::Genome*, RqNervousSystem*);
    void setMaxWeight(RqNervousSystem*, AbstractFile*, float);
    double getExpansion(genome::Genome*, RqNervousSystem*, double, int, int, int, int);
}
