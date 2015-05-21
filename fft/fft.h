

#ifndef	_MY_FFT_H_
#define _MY_FFT_H_

#include "mydefs.h"

// td - time domain; fd - frequency domain; nNumPts - number of samples, must be 2^n for some integer n >= 1.
void FFT(int nNumPts, cpxv_t *x_td, cpxv_t *x_fd);
void IFFT(int nNumPts, cpxv_t *x_td, cpxv_t *x_fd);
void DoFFT(const short xxx[], cpxv_t fdom[]);
void CalcMFCC(const cpxv_t pFSP[], double pmfcc[], int nNumMfccDims, double* pbandLog);
void AreaOfMoments(const double pdbIndata[], int num_rows, int MM, double pdbOutData[], int KK);

#endif // #ifndef	_MY_FFT_H_




