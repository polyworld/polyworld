#pragma once

void usage_brainfunction();
int process_brainfunction(int argc, char *argv[]);


#ifdef BRAINFUNCTION_CPP

#include "complexity.h"

class Callback_bf : public CalcComplexity_brainfunction_callback
{
public:
	Callback_bf(bool _bare,
		    const char **_parts_combos,
		    int _nparts_combos) : bare(_bare), parts_combos(_parts_combos), nparts_combos(_nparts_combos), reported(NULL) {}
	virtual ~Callback_bf() {}

	virtual void begin(CalcComplexity_brainfunction_parms *parms,
			   int nparms);
	virtual void parms_result(CalcComplexity_brainfunction_result *result,
				  int parms_index);
	void display(CalcComplexity_brainfunction_result *result);
	virtual void end(CalcComplexity_brainfunction_result *result);

private:
	bool bare;
	const char **parts_combos;
	int nparts_combos;
	bool *reported;
	int last_displayed;
};

#endif // BRAINFUNCTION_CPP
