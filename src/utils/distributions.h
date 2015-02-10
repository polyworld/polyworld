
#ifndef DISTRIBUTIONS_H
#define DISTRIBUTIONS_H


float normalPDF(float x, float sigma, float mu);
float linearPDF(float x, float slope, float yIntercept);
float getLinear(float slope, float yIntercept);
float getNormal(float sigma, float mu);

#endif
