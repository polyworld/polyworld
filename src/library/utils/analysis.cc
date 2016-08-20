#include "analysis.h"

#include <limits>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <vector>

#include "agent/agent.h"
#include "brain/Brain.h"
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

const double analysis::inf = std::numeric_limits<double>::infinity();

void analysis::Vector::add(Vector& addend1, Vector& addend2, Vector& sum) {
    for (int index = 0; index < sum.size; index++) {
        sum.values[index] = addend1.values[index] + addend2.values[index];
    }
}

void analysis::Vector::subtract(Vector& minuend, Vector& subtrahend, Vector& difference) {
    for (int index = 0; index < difference.size; index++) {
        difference.values[index] = minuend.values[index] - subtrahend.values[index];
    }
}

analysis::Vector::Vector(int size) :
    size(size),
    values(new double[size]) { }

analysis::Vector::~Vector() {
    delete[] values;
}

void analysis::Vector::reset() {
    for (int index = 0; index < size; index++) {
        values[index] = 0.0;
    }
}

void analysis::Vector::randomize(double mean, double stdev) {
    for (int index = 0; index < size; index++) {
        values[index] = nrand(mean, stdev);
    }
}

double analysis::Vector::getMagnitude() {
    double sum2 = 0.0;
    for (int index = 0; index < size; index++) {
        double value = values[index];
        sum2 += value * value;
    }
    return sqrt(sum2);
}

void analysis::Vector::scaleBy(double factor) {
    for (int index = 0; index < size; index++) {
        values[index] *= factor;
    }
}

void analysis::Vector::scaleTo(double magnitude) {
    scaleBy(magnitude / getMagnitude());
}

void analysis::initialize(const std::string& run) {
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

int analysis::getMaxAgent(const std::string& run) {
    DataLibReader reader((run + "/lifespans.txt").c_str());
    reader.seekTable("LifeSpans");
    return reader.nrows();
}

genome::Genome* analysis::getGenome(const std::string& run, int agent) {
    std::string path = run + "/genome/agents/genome_" + std::to_string(agent) + ".txt";
    AbstractFile* file = AbstractFile::open(globals::recordFileType, path.c_str(), "r");
    genome::Genome* genome = genome::GenomeUtil::createGenome();
    genome->load(file);
    delete file;
    return genome;
}

AbstractFile* analysis::getSynapses(const std::string& run, int agent, const std::string& stage) {
    std::string path = run + "/brain/synapses/synapses_" + std::to_string(agent) + "_" + stage + ".txt";
    if (AbstractFile::exists(path.c_str())) {
        return AbstractFile::open(globals::recordFileType, path.c_str(), "r");
    } else {
        return NULL;
    }
}

RqNervousSystem* analysis::getNervousSystem(genome::Genome* genome, AbstractFile* synapses) {
    RqNervousSystem* cns = new RqNervousSystem();
    cns->grow(genome);
    cns->getBrain()->loadSynapses(synapses);
    return cns;
}

RqNervousSystem* analysis::getNervousSystem(const std::string& run, int agent, const std::string& stage) {
    AbstractFile* synapses = getSynapses(run, agent, stage);
    RqNervousSystem* cns;
    if (synapses == NULL) {
        cns = NULL;
    } else {
        genome::Genome* genome = getGenome(run, agent);
        cns = getNervousSystem(genome, synapses);
        delete genome;
    }
    delete synapses;
    return cns;
}

RqNervousSystem* analysis::copyNervousSystem(genome::Genome* genome, RqNervousSystem* other) {
    RqNervousSystem* cns = new RqNervousSystem();
    cns->grow(genome);
    cns->getBrain()->copySynapses(other->getBrain());
    return cns;
}

double analysis::getLyapunov(genome::Genome* genome, RqNervousSystem* cns, double perturbation, int repeats, int random, int quiescent, int steps) {
    
    // Set up nervous systems
    RqNervousSystem* cns1 = copyNervousSystem(genome, cns);  // Reference
    RqNervousSystem* cns2 = copyNervousSystem(genome, cns);  // Perturbed
    cns1->getBrain()->freeze();
    cns2->getBrain()->freeze();
    cns2->setMode(RqNervousSystem::QUIESCENT);
    
    // Set up activation vectors
    NeuronModel::Dimensions dims = cns->getBrain()->getDimensions();
    int nstart = dims.getFirstOutputNeuron();
    int ncount = dims.getNumNonInputNeurons();
    Vector activations1(ncount);  // Reference
    Vector activations2(ncount);  // Perturbed
    Vector deltas(ncount);
    
    // Perform set of calculation attempts
    std::vector<double> lyapunovs;
    for (int index = 0; index < repeats; index++) {
        
        // Iterate with random inputs
        cns1->getBrain()->randomizeActivations();
        cns1->setMode(RqNervousSystem::RANDOM);
        for (int step = 1; step <= random; step++) {
            cns1->update(false);
        }
        
        // Iterate with quiescent inputs
        cns1->setMode(RqNervousSystem::QUIESCENT);
        for (int step = 1; step <= quiescent; step++) {
            cns1->update(false);
        }
        
        // Introduce perturbation
        deltas.randomize(0.0, 1.0);
        deltas.scaleTo(perturbation);
        cns1->getBrain()->getActivations(activations1.values, nstart, ncount);
        Vector::add(activations1, deltas, activations2);
        cns2->getBrain()->setActivations(activations2.values, nstart, ncount);
        
        // Measure effect of perturbation
        double sum = 0.0;
        for (int step = 1; step <= steps; step++) {
            
            // Step both with quiescent inputs
            cns1->update(false);
            cns2->update(false);
            
            // Calculate new distance
            cns1->getBrain()->getActivations(activations1.values, nstart, ncount);
            cns2->getBrain()->getActivations(activations2.values, nstart, ncount);
            Vector::subtract(activations2, activations1, deltas);
            double distance = deltas.getMagnitude();
            
            // Check if trajectories converged
            if (distance == 0.0) {
                sum = -inf;
                break;
            }
            
            // Add to running total
            sum += log(distance);
            
            // Rescale to initial perturbation
            deltas.scaleBy(perturbation / distance);
            Vector::add(activations1, deltas, activations2);
            cns2->getBrain()->setActivations(activations2.values, nstart, ncount);
        }
        
        // Save result of this attempt
        if (sum != -inf) {
            lyapunovs.push_back(sum / steps - log(perturbation));
        }
    }
    
    // Clean up
    delete cns1;
    delete cns2;
    
    // Return average of attempts
    if (lyapunovs.empty()) {
        return -inf;
    } else {
        double sum = 0.0;
        for (std::vector<double>::iterator it = lyapunovs.begin(); it != lyapunovs.end(); it++) {
            sum += *it;
        }
        return sum / lyapunovs.size();
    }
}
