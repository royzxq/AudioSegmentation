/*----------------------------------------------------------------------------
   fft.c - fast Fourier transform and its inverse (both recursively)
   Copyright (C) 2004, Jerome R. Breitenbach.  All rights reserved.

   The author gives permission to anyone to freely copy, distribute, and use
   this file, under the following conditions:
      - No changes are made.
      - No direct commercial advantage is obtained.
      - No liability is attributed to the author for any damages incurred.
  ----------------------------------------------------------------------------*/

/******************************************************************************
 * This file defines a C function fft that, by calling another function       *
 * fft_rec (also defined), calculates an FFT recursively.  Usage:             *
 *   fft(nNumPts, x_time_d, x_frequency_d);                                                            *
 * Parameters:                                                                *
 *   nNumPts: number of points in FFT (must equal 2^n for some integer n >= 1)      *
 *   x_time_d: pointer to nNumPts time-domain samples given in rectangular form (Re x_time_d,     *
 *      Im x_time_d)                                                                 *
 *   x_frequency_d: pointer to nNumPts frequency-domain samples calculated in rectangular form  *
 *      (Re x_frequency_d, Im x_frequency_d)                                                          *
 * Similarly, a function IFFT with the same parameters is defined that        *
 * calculates an inverse FFT (IFFT) recursively.  Usage:                      *
 *   IFFT(nNumPts, x_time_d, x_frequency_d);                                                           *
 * Here, nNumPts and x_frequency_d are given, and x_time_d is calculated.                              *
 ******************************************************************************/

#include "stdafx.h"

#include <stdlib.h>
#include <math.h>
#include <FLOAT.H>

#include "fft.h"
#include "parameters.h"

void fft_rec(int nNumPts, const int offset, const int delta, cpxv_t *x_td, cpxv_t *x_fd, cpxv_t *XX);

/* FFT */
// nNumPts : number of samples(points) in time domain;
// x_time_d : (input)sequence of samples(each is a complex number);
// x_frequency_d : (output)sequence of samples in frequency domain(each is a complex number).
void FFT(int nNumPts, cpxv_t *x_time_d, cpxv_t *x_frequency_d)
{
  /* Declare a pointer to scratch space. */
  cpxv_t *XX = (cpxv_t *)malloc(nNumPts * sizeof(cpxv_t));

  /* Calculate FFT by a recursion. */
  fft_rec(nNumPts, 0, 1, x_time_d, x_frequency_d, XX);

  /* Free memory. */
  free(XX);
}

/* FFT recursion */
void fft_rec(int nNumPts, const int offset, const int delta, cpxv_t *x_time_d, cpxv_t *x_frequency_d, cpxv_t *XX)
{
	int N2 = nNumPts/2;            /* half the number of points in FFT */
	int k;                   /* generic index */
	double cs, sn;           /* cosine and sine */
	int k00, k01, k10, k11;  /* indices for butterflies */
	double tmp0, tmp1;       /* temporary storage */

	if (nNumPts != 2) {
	/* Perform recursive step. */
		/* Calculate two (nNumPts/2)-point DFT's. */
		fft_rec(N2, offset, 2*delta, x_time_d, XX, x_frequency_d);
		fft_rec(N2, offset+delta, 2*delta, x_time_d, XX, x_frequency_d);

		/* Combine the two (nNumPts/2)-point DFT's into one nNumPts-point DFT. */
		for(k=0; k<N2; k++) {
			cs = cos(TWO_PI*k/(double)nNumPts); sn = sin(TWO_PI*k/(double)nNumPts);

			k10 = offset + 2*k*delta;
			k11 = k10 + delta;
			tmp0 = XX[k11].re * cs + XX[k11].im * sn;	// Re part
			tmp1 = XX[k11].im * cs - XX[k11].re * sn;	// Im part

			k00 = offset + k*delta;    
			k01 = k00 + N2*delta;

			x_frequency_d[k00].re = XX[k10].re + tmp0;
			x_frequency_d[k00].im = XX[k10].im + tmp1;

			x_frequency_d[k01].re = XX[k10].re - tmp0;
			x_frequency_d[k01].im = XX[k10].im - tmp1;
        }
	} else {
	/* Perform 2-point DFT. */
		k00 = offset;
		k01 = k00 + delta;
		x_frequency_d[k00].re = x_time_d[k00].re + x_time_d[k01].re;
		x_frequency_d[k00].im = x_time_d[k00].im + x_time_d[k01].im;

		x_frequency_d[k01].re = x_time_d[k00].re - x_time_d[k01].re;
		x_frequency_d[k01].im = x_time_d[k00].im - x_time_d[k01].im;
    }
}

/* IFFT */
void IFFT(int nNumPts, cpxv_t *x_time_d, cpxv_t *x_frequency_d)
{
	int N2 = nNumPts/2;       /* half the number of points in IFFT */
	int i;              /* generic index */
	double tmp0, tmp1;  /* temporary storage */

	/* Calculate IFFT via reciprocity property of DFT. */
	FFT(nNumPts, x_frequency_d, x_time_d);
	x_time_d[ 0].re = x_time_d[ 0].re/nNumPts;    
	x_time_d[ 0].im = x_time_d[ 0].im/nNumPts;
	x_time_d[N2].re = x_time_d[N2].re/nNumPts;  
	x_time_d[N2].im = x_time_d[N2].im/nNumPts;
	for(i=1; i<N2; i++) {
		tmp0 = x_time_d[i].re/nNumPts;
		tmp1 = x_time_d[i].im/nNumPts;
		x_time_d[i].re = x_time_d[nNumPts-i].re/nNumPts;
		x_time_d[i].im = x_time_d[nNumPts-i].im/nNumPts;
		x_time_d[nNumPts-i].re = tmp0;
		x_time_d[nNumPts-i].im = tmp1;
    }
}

void PreEmphasize(cpxv_t *sample, int nNumSamps);

void DoFFT(const short xxx[], cpxv_t fdom[])
{
	cpxv_t* tdom = Malloc(cpxv_t, NumBinsInFftWinM);
	int isamp;
	for (isamp=0; isamp<NumSamplesInFrameM; isamp++) {// copy
		tdom[isamp].re = xxx[isamp];
		tdom[isamp].im = 0.0;
	}
	PreEmphasize(tdom, NumSamplesInFrameM);
	for (isamp=0; isamp<NumSamplesInFrameM; isamp++) {
		// Hamming windowing
		tdom[isamp].re *= (0.54-0.46*cos(TWO_PI*isamp/NumSamplesInFrameM));
		tdom[isamp].im = 0.0;
	}
	for (; isamp<NumBinsInFftWinM; isamp++) {// zero padding
		tdom[isamp].re = 0.0;
		tdom[isamp].im = 0.0;
	}
	FFT(NumBinsInFftWinM, tdom, fdom);
	free(tdom);
}

void CalcMFCC(const cpxv_t pFSP[], double pmfcc[], int nNumDimsMfcc, double* pbandVals)
{
	double f_min = 0.0;		// Hz
	double f_max = UNIFIED_SAMPLING_RATE/2;	// 采样频率的一半

	// Hz 到 Mel 的转换关系

	double ddd = (2595.0*log10(1.0+f_max/700)-2595.0*log10(1.0+f_min/700))/(NUM_MEL_BANDS+1);
	// fc 频带的中心频率（Hz），包括 0 频率
	double* fc = (double*)malloc(sizeof(double)*(NUM_MEL_BANDS+1)*2);
	// XXX 各个频带的累加值
	double* XXX = fc+NUM_MEL_BANDS+1;

	int mmm, kkk;
	fc[0] = 0.0;
	for (mmm=1; mmm<=NUM_MEL_BANDS; mmm++) {
		// 求频带的中心频率（Hz）
		fc[mmm] = 700.0*(pow(10.0, ddd*mmm/2595.0)-1.0);
	}

	ddd = UNIFIED_SAMPLING_RATE/NumBinsInFftWinM;
	double ff, dbTemp;
	// 一个单元浪费，但处理简单不易错
	for (mmm=1; mmm<=NUM_MEL_BANDS; mmm++) {
		XXX[mmm] = 0.0;
		for (kkk=0; kkk<NumBinsInFftWinM/2; kkk++) {// 高于采样频率二分之一的频谱分量不予考虑
			ff = ddd*kkk;	// 分量对应的频率
			// 三角滤波器
			if (fc[mmm-1] <= ff && ff < fc[mmm]) {
				dbTemp = sqrt(pFSP[kkk].re*pFSP[kkk].re+pFSP[kkk].im*pFSP[kkk].im);	// 模，幅值
				dbTemp *= (ff-fc[mmm-1])/(fc[mmm]-fc[mmm-1]);	// 加权
			} else if (fc[mmm] <= ff && ff < fc[mmm+1]) {
				dbTemp = sqrt(pFSP[kkk].re*pFSP[kkk].re+pFSP[kkk].im*pFSP[kkk].im);	// 模，幅值
				dbTemp *= (ff-fc[mmm+1])/(fc[mmm]-fc[mmm+1]);	// 加权
			} else {
				dbTemp = 0.0;
			}
			XXX[mmm] += dbTemp;	// 频带内幅值累加
		}
		if (pbandVals) {
			pbandVals[mmm-1] = XXX[mmm];
		}
//		XXX[mmm] = log(1.0+CCCC_FACTOR*XXX[mmm]);	// 取自然对数
		XXX[mmm] = log(1.0+XXX[mmm]);	// 取自然对数
	}

/*	if (pbandLog) {// 各个 Mel band 的值
		for (mmm=1; mmm<=NUM_MEL_BANDS; mmm++) {
			pbandLog[mmm-1] = XXX[mmm];
		}
	}*/

	if (pmfcc) {// 做离散余弦变换
		for (kkk=1; kkk<=nNumDimsMfcc; kkk++) {
			pmfcc[kkk-1] = 0.0;
			for (mmm=1; mmm<=NUM_MEL_BANDS; mmm++) {
				pmfcc[kkk-1] += XXX[mmm]*cos((double)kkk*MY_PI/NUM_MEL_BANDS*((double)mmm-0.5));
			}
		}
	}

	free(fc);
}

// Filter : H(z) = 1-a*z^(-1)
void PreEmphasize(cpxv_t *sample, int nNumSamps)
{
// Setting emphFac=0 turns off preemphasis.

	double fEmphFactor = 0.92;	//0.95;	//0.97;
	for (int iSamp = nNumSamps-1; iSamp > 0; iSamp--) {
		sample[iSamp].re = sample[iSamp].re - fEmphFactor * sample[iSamp-1].re;
	}
	sample[0].re = (1.0 - fEmphFactor) * sample[0].re;
}
