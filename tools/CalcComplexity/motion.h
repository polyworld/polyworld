#pragma once

#define DEFAULT_EPOCH 250
#define DEFAULT_THRESHOLD 0.15


int process_motion(int argc, char *argv[]);


#ifdef MOTION_CPP

#include "complexity.h"


class Callback : public CalcComplexity_motion_callback
{
public:
	Callback(bool _bare) : bare(_bare), reported(NULL), total_elements(0), total_nil(0) {}
	virtual ~Callback() {}

	virtual void begin(CalcComplexity_motion_parms &parms);
	virtual void epoch_result(CalcComplexity_motion_result *result,
				  int epoch_index);
	virtual void end(CalcComplexity_motion_result *result);

 private:
  	void display(CalcComplexity_motion_result *result);

 private:
	bool bare;
	bool *reported;
	int last_displayed;
	long long total_elements;
	long long total_nil;
};

#endif // MOTION_CP
