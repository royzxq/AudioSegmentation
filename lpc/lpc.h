

#ifndef	_MY_LPC_H_
#define _MY_LPC_H_

#include <stdio.h>

#include "mydefs.h"

class CCalcLPC {
private:
//	void LPC(short *xxx, int NNN, double *aaa, int ppp);
//	void LPC02(short *xxx, int NNN, double *aaa, int ppp);
//	void wAutocorrelate(short *xxx, int NNN, double *rrr_out, int PPP, double lambda);
//	int LevinsonRecursion(int P, const float *R, float *A, float *K);

private:
	int CalcVector(FILE *pfr);

public:
	CCalcLPC();
	~CCalcLPC();
};

#endif // #ifndef	_MY_LPC_H_




