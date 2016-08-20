#include "analysis.h"

#include <stdlib.h>
#include <string>
#include <time.h>

#include "agent/agent.h"
#include "brain/Brain.h"
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
