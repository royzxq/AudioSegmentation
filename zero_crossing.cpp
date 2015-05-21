
#include "stdafx.h"
#include  <math.h>
#include  <stdlib.h>


#define ZZZ_CCC_ZONE	32	// 32767(the largest abs value of signal)*0.10%	

double ZeroCrossing(const short xxx[], int nNumSamples)
{
	double dbnzc = 0;
	int tag;
	if (xxx[0] > ZZZ_CCC_ZONE) {
		tag = 1;
	} else if (xxx[0] < -ZZZ_CCC_ZONE) {
		tag = -1;
	} else {
		tag = 0;
	}
	int ii;
	for (ii=1; ii<nNumSamples; ii++) {// from 1 on, ...
		if (xxx[ii] > ZZZ_CCC_ZONE) {
			if (tag <= 0) { dbnzc += 1.0; }
			tag = 1;
		} else if (xxx[ii] < -ZZZ_CCC_ZONE) {
			if (tag >= 0) { dbnzc += 1.0; }
			tag = -1;
		} else {
			if (tag != 0) { dbnzc += 1.0; }
			tag = 0;
		}
	}

	return dbnzc;	///nNumSamples;
}

double ShortTimeEnergy(const short xxx[], int nNumSamples)
{
	double dbenergy = 0;
	int ii;
	for (ii=0; ii<nNumSamples; ii++) {
		dbenergy += abs((int)xxx[ii]);
	}
	return dbenergy/nNumSamples;
}

