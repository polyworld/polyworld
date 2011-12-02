#ifndef COMPLEXITY_BRAIN_H
#define COMPLEXITY_BRAIN_H

#include <string>
#include <vector>

#include <gsl/gsl_matrix.h>

#include "Events.h"

struct CalcComplexity_brainfunction_parms
{
	const char *path;
	const char *parts;
	int ignore_timesteps_after;
	Events *events;
};

class CalcComplexity_brainfunction_result
{
 public:
	CalcComplexity_brainfunction_parms *parms;
	int nparms;

	double *complexity;
	long *agent_number;
	long *num_neurons;
	long *lifespan;

 public:
	CalcComplexity_brainfunction_result(CalcComplexity_brainfunction_parms *_parms,
					    int _nparms)
	{
		parms = _parms;
		nparms = _nparms;
		complexity = new double[nparms];
		agent_number = new long[nparms];
		num_neurons = new long[nparms];
		lifespan = new long[nparms];
	}
	~CalcComplexity_brainfunction_result()
	{
		delete complexity;
		delete agent_number;
		delete num_neurons;
		delete lifespan;
	}  
					    
};

class CalcComplexity_brainfunction_callback
{
 public:
	virtual ~CalcComplexity_brainfunction_callback() {}

	virtual void begin(CalcComplexity_brainfunction_parms *parms,
			   int nparms) = 0;
	virtual void parms_result(CalcComplexity_brainfunction_result *result,
				  int parms_index) = 0;
	virtual void end(CalcComplexity_brainfunction_result *result) = 0;
};

CalcComplexity_brainfunction_result *CalcComplexity_brainfunction(CalcComplexity_brainfunction_parms *parms,
							  int nparms,
							  CalcComplexity_brainfunction_callback *callback = 0);
double CalcComplexity_brainfunction(const char *path,
									const char *parts,
									Events *events = NULL,
									int ignore_timesteps_after = 0,
									long *agent_number = NULL,
									long *lifespan = NULL,
									long *num_neurons = NULL);
double CalcComplexityWithMatrix_brainfunction(gsl_matrix *matrix,
											  const char *parts,
											  long numinputneurons,
											  long numoutputneurons);

std::vector<std::string> get_list_of_brainfunction_logfiles( std::string );
std::vector<std::string> get_list_of_brainanatomy_logfiles( std::string );

gsl_matrix * readin_brainfunction(const char *path,
											 int ignore_timesteps_after,
											 int max_timestapes,
											 long *agent_number,
											 long *agent_birth,
											 long *lifespan,
											 long *num_neurons,
											 long *num_ineurons,
											 long *num_oneurons);
gsl_matrix * readin_brainanatomy( const char* );


#endif // COMPLEXITY_BRAIN_H
