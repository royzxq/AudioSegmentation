
#include "stdafx.h"


#include <math.h>
#include <stdio.h>
#include <malloc.h>

#include "mfcc01.h"


/**
 * Normalizes the given samples so that the absolute value of the highest sample amplitude is 1. Does nothing if all samples 
 * are 0.
 *
 * @param	samples_to_normalize	The samples to normalize.
 * @return							Returns a copy of the given samples after normalization.
 */
double* normalizeSamples(double samples_to_normalize[], int NNN)
{
	double *normalized_samples = new double[NNN];
	for (int samp = 0; samp < NNN; samp++)
		normalized_samples[samp] = samples_to_normalize[samp];

	double max_sample_value = 0.0;
	for (int samp = 0; samp < NNN; samp++)
		if (abs(normalized_samples[samp]) > max_sample_value)
			max_sample_value = abs(normalized_samples[samp]);
	if (max_sample_value != 0.0)
		for (int samp = 0; samp < NNN; samp++)
			normalized_samples[samp] /= max_sample_value;

	return normalized_samples;
}



/****************************************************************************
*
* Name: FirAlgs.c
*
* Synopsis: FIR filter algorithms for use in C
*
* Description:
*
* This module provides functions to implement Finite Impulse Response (FIR)
* filters using a variety of algorithms.
*
* These functions use most or all of the following input parameters:
*
*	input - the input sample data
*	ntaps - the number of taps in the filter
*	 h[] - the FIR coefficient array
*	z[] - the FIR delay line array
*	*p_state - the "state" of the filter; initialize this to 0 before the first call
*
* The functions fir_basic, fir_shuffle, and fir_circular are not very
* efficient in C, and are provided here mainly to illustrate FIR
* algorithmic concepts. However, the functions fir_split, fir_double_z
* and fir_double_h are all fairly efficient ways to implement FIR filters
* in C.
*
* Copyright 2000 by Grant R. Griffin
*
* Thanks go to contributors of comp.dsp for teaching me some of these
* techniques, and to Jim Thomas for his review and great suggestions.
*
* The Wide Open License (WOL)
*
* Permission to use, copy, modify, distribute and sell this software and its
* documentation for any purpose is hereby granted without fee, provided that
* the above copyright notice and this license appear in all source copies.
* THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF
* ANY KIND. See http://www.dspguru.com/wol.htm for more information.
******************************************************************************/

#include <stdio.h>
#ifdef WIN32
#include <conio.h>
#endif
#define SAMPLE double /* define the type used for data samples */
/****************************************************************************/
/* clear: zeroize a FIR delay line*/
/****************************************************************************/
void clear(int ntaps, SAMPLE z[])
{
	int ii;
	for (ii = 0; ii < ntaps; ii++) {
		z[ii] = 0;
	}
}
/****************************************************************************
* fir_basic: Does the basic FIR algorithm: store input sample, calculate
* output sample, move delay line
*****************************************************************************/
SAMPLE fir_basic(SAMPLE input, int ntaps, const SAMPLE h[], SAMPLE z[])
{
	int ii;
	SAMPLE accum;
	/* store input at the beginning of the delay line */
	z[0] = input;
	/* calc FIR */
	accum = 0;
	for (ii = 0; ii < ntaps; ii++) {
		accum += h[ii] * z[ii];
	}
	/* shift delay line */
	for (ii = ntaps - 2; ii >= 0; ii--) {
		z[ii + 1] = z[ii];
	}
	return accum;
}
/****************************************************************************
* fir_circular: This function illustrates the use of "circular" buffers
* in FIR implementations. The advantage of circular buffers is that they
* alleviate the need to move data samples around in the delay line (as
* was done in all the functions above). Most DSP microprocessors implement
* circular buffers in hardware, which allows a single FIR tap can be
* calculated in a single instruction. That works fine when programming in
* assembly, but since C doesn't have any constructs to represent circular
* buffers, you need to "fake" them in C by adding an extra "if" statement
* inside the FIR calculation, even if the DSP processor provides hardware to
* implement them without overhead.
*****************************************************************************/
SAMPLE fir_circular(SAMPLE input, int ntaps, const SAMPLE h[], SAMPLE z[], int *p_state)
{
	int ii, state;
	SAMPLE accum;
	state = *p_state; /* copy the filter's state to a local */
	/* store input at the beginning of the delay line */
	z[state] = input;
	if (++state >= ntaps) { /* incr state and checkfor wrap */
		state = 0;
	}
	/* calc FIR and shift data */
	accum = 0;
	for (ii = ntaps - 1; ii >= 0; ii--) {
		accum += h[ii] * z[state];
		if (++state >= ntaps) { /* incr state and check for wrap */
			state = 0;
		}
	}
	*p_state = state; /* return new state to caller */
	return accum;
}
/****************************************************************************
* fir_shuffle: This is like fir_basic, except that data is shuffled by
* moving it _inside_ the calculation loop. This is similar to the MACD
* instruction on TI's fixed-point processors
*****************************************************************************/
SAMPLE fir_shuffle(SAMPLE input, int ntaps, const SAMPLE h[], SAMPLE z[])
{
	int ii;
	SAMPLE accum;
	/* store input at the beginning of the delay line */
	z[0] = input;
	/* calc FIR and shift data */
	accum = h[ntaps - 1] * z[ntaps - 1];
	for (ii = ntaps - 2; ii >= 0; ii--) {
		accum += h[ii] * z[ii];
		z[ii + 1] = z[ii];
	}

	return accum;
}

/****************************************************************************
* fir_split: This splits the calculation into two parts so the circular
* buffer logic doesn't have to be done inside the calculation loop
*****************************************************************************/
SAMPLE fir_split(SAMPLE input, int ntaps, const SAMPLE h[], SAMPLE z[], int *p_state)
{
	int ii, end_ntaps, state = *p_state;
	SAMPLE accum;
	SAMPLE const *p_h;
	SAMPLE *p_z;
	/* setup the filter */
	accum = 0;
	p_h = h;
	/* calculate the end part */
	p_z = z + state;
	*p_z = input;
	end_ntaps = ntaps - state;
	for (ii = 0; ii < end_ntaps; ii++) {
		accum += *p_h++ * *p_z++;
	}
	/* calculate the beginning part */
	p_z = z;
	for (ii = 0; ii < state; ii++) {
		accum += *p_h++ * *p_z++;
	}
	/* decrement the state, wrapping if below zero */
	if (--state < 0) {
		state += ntaps;
	}
	*p_state = state; /* return new state to caller */

	return accum;
}

/****************************************************************************
* fir_double_z: double the delay line so the FIR calculation always
* operates on a flat buffer
*****************************************************************************/
SAMPLE fir_double_z(SAMPLE input, int ntaps, const SAMPLE h[], SAMPLE z[], int *p_state)
{
	SAMPLE accum;
	int ii, state = *p_state;
	SAMPLE const *p_h, *p_z;
	/* store input at the beginning of the delay line as well as ntaps more */
	z[state] = z[state + ntaps] = input;
	/* calculate the filter */
	p_h = h;
	p_z = z + state;
	accum = 0;
	for (ii = 0; ii < ntaps; ii++) {
		accum += *p_h++ * *p_z++;
	}
	/* decrement state, wrapping if below zero */
	if (--state < 0) {
		state += ntaps;
	}
	*p_state = state; /* return new state to caller */
	return accum;
}

/****************************************************************************
* fir_double_h: uses doubled coefficients (supplied by caller) so that the
* filter calculation always operates on a flat buffer.
*****************************************************************************/
SAMPLE fir_double_h(SAMPLE input, int ntaps, const SAMPLE h[], SAMPLE z[], int *p_state)
{
	SAMPLE accum;
	int ii, state = *p_state;
	SAMPLE const *p_h, *p_z;
	/* store input at the beginning of the delay line */
	z[state] = input;
	/* calculate the filter */
	p_h = h + ntaps - state;
	p_z = z;
	accum = 0;
	for (ii = 0; ii < ntaps; ii++) {
		accum += *p_h++ * *p_z++;
	}

	/* decrement state, wrapping if below zero */
	if (--state < 0) {
		state += ntaps;
	}
	*p_state = state; /* return new state to caller */

	return accum;
}

/****************************************************************************
* main: This provides a simple test suite for the functions above. An
* impulse is fed into each filter implementation, so the output should be
* the "impulse response", that is, the coefficients of the filter. You
* should see some zeroes, followed by the "coefficents" below, followed by a
* few more zeroes.
*****************************************************************************/
int main_fir(void)
{
	#define NTAPS 6
	static const SAMPLE h[NTAPS] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
	static SAMPLE h2[2 * NTAPS];
	static SAMPLE z[2 * NTAPS];
	#define IMP_SIZE (3 * NTAPS)
	static SAMPLE imp[IMP_SIZE];
	SAMPLE output;
	int ii, state;
	/* make impulse input signal */
	clear(IMP_SIZE, imp);
	imp[5] = 1.0;
	/* create a SAMPLEd h */
	for (ii = 0; ii < NTAPS; ii++) {
		h2[ii] = h2[ii + NTAPS] = h[ii];
	}
	/* test FIR algorithms */
	printf("Testing fir_basic:\n ");
	clear(NTAPS, z);
	for (ii = 0; ii < IMP_SIZE; ii++) {
		output = fir_basic(imp[ii], NTAPS, h, z);
		printf("%3.1lf ", (double) output);
	}
	printf("\n\n");
	printf("Testing fir_shuffle:\n ");
	clear(NTAPS, z);
	state = 0;
	for (ii = 0; ii < IMP_SIZE; ii++) {
		output = fir_shuffle(imp[ii], NTAPS, h, z);
		printf("%3.1lf ", (double) output);
	}
	printf("\n\n");
	printf("Testing fir_circular:\n ");
	clear(NTAPS, z);
	state = 0;
	for (ii = 0; ii < IMP_SIZE; ii++) {
		output = fir_circular(imp[ii], NTAPS, h, z, &state);
		printf("%3.1lf ", (double) output);
	}
	printf("\n\n");
	printf("Testing fir_split:\n ");
	clear(NTAPS, z);
	state = 0;
	for (ii = 0; ii < IMP_SIZE; ii++) {
		output = fir_split(imp[ii], NTAPS, h, z, &state);
		printf("%3.1lf ", (double) output);
	}
	printf("\n\n");
	printf("Testing fir_double_z:\n ");
	clear(2 * NTAPS, z);
	state = 0;
	for (ii = 0; ii < IMP_SIZE; ii++) {
		output = fir_double_z(imp[ii], NTAPS, h, z, &state);
		printf("%3.1lf ", (double) output);
	}
	printf("\n\n");
	printf("Testing fir_double_h:\n ");
	clear(NTAPS, z);
	state = 0;
	for (ii = 0; ii < IMP_SIZE; ii++) {
		output = fir_double_h(imp[ii], NTAPS, h2, z, &state);
		printf("%3.1lf ", (double) output);
	}
#ifdef WIN32
	printf("\n\nHit any key to continue.");
	_getch();
#endif
	return 0;
}



#ifdef MFCC_METHOD_002

float m_dctMatrix[(FE_CEP_ORDER+1)*FE_NUM_CHANNELS];
//int m_sampleRate=32000;
WfMelFB m_MelFB[FE_NUM_CHANNELS];
float m_MelWeight[NumBinsInFftWinM/2+1];

float m_logEnergyFloor = 0.0;
float m_energyFloor = 1.0;

int CalculateMFCC(const short *pSamps, int frameSize, double *pMfcc, int ceporder, int fftWinSize)
{
	float* frameA = (float*)malloc(frameSize*4);

	preprocessing(pSamps, frameSize, &frameA[0]);
	Do_hamm_window(&frameA[0], frameSize);

	_mel_cepstrum_basic(&frameA[0], frameSize, pMfcc, FE_NUM_CHANNELS, ceporder, fftWinSize);
	free(frameA);

	return 1;
}

int MfccInitDCTMatrix(float *dctMatrix, int ceporder, int numChannels)
{
    int i, j;
    for (i = 0; i <= ceporder; i++){//12+1
        for (j = 0; j < numChannels; j++){//23
            dctMatrix[i * numChannels + j] = (float) cos (M_PI * (float) i / (float) numChannels * ((float) j + 0.5));
			if(i==0) dctMatrix[i * numChannels + j]*=(float)sqrt(1/(float)numChannels);
			else     dctMatrix[i * numChannels + j]*=(float)sqrt(2/(float)numChannels);
		}
	}
	return 1;
}

int _mel_cepstrum_basic(float *pSamps, int frameSize, double *pMfcc, int filterBankOrder, int ceporder, int fftWinSize)
{
	float* filterBank = (float*)malloc(filterBankOrder*4);
	MfccInitDCTMatrix(&m_dctMatrix[0], ceporder, filterBankOrder);
	_filterbank_basic(pSamps, frameSize, &filterBank[0], filterBankOrder, fftWinSize, 0, 0);
	MfccDCT(filterBank, &m_dctMatrix[0], ceporder, filterBankOrder, pMfcc);
	free(filterBank);
//	scale down to be consistent with other kinds of cepstrum coefficients
	float ff = filterBankOrder/(float)fftWinSize;
	for (int ii=0; ii<=ceporder; ii++) {
		pMfcc[ii] *= ff;
	}

	return 0;
}

/* Supporting routine for MFCC */
#define MfccRound(x) ((int)((x)+0.5))
void MfccInitMelFilterBanks(float startingFrequency, float samplingRate, int fftWinSize, int numChannels)
{
    int i, k;
    float* freq=(float*)malloc(numChannels*4+8);
	int * bin = (int *)malloc(numChannels*4+8);
	float start_mel, fs_per_2_mel;
	
    /* Constants for calculation */
	freq[0] = startingFrequency;
    start_mel = (float)(2595.0 * log10 (1.0 + startingFrequency / 700.0));
	bin[0] = MfccRound(fftWinSize * freq[0] / samplingRate);
	freq[numChannels+1] = (float)(samplingRate / 2.0);
    fs_per_2_mel = (float)(2595.0 * log10 (1.0 + (samplingRate / 2.0) / 700.0));
	bin[numChannels+1] = MfccRound(fftWinSize * freq[numChannels+1] / samplingRate);
	
	/* Calculating mel-scaled frequency and the corresponding FFT-bin */
    /* number for the lower edge of the band                          */
	for (k = 1; k <= numChannels; k++) {
        freq[k] = (float)(700 * (pow (10, (start_mel + (float) k / (numChannels + 1) * (fs_per_2_mel - start_mel)) / 2595.0) - 1.0));
		bin[k] = MfccRound(fftWinSize * freq[k] / samplingRate);
	}
	
	/* This part is never used to compute MFCC coefficients */
	/* but initialized for completeness                     */
	

	for (i = 0; i<bin[0]; i++) {
		m_MelWeight[i] = 0;
	}
	m_MelWeight[fftWinSize/2] = 1;
	
	/* Initialize low, center, high indices to FFT-bin */
		for (k = 0; k <= numChannels; k++) {
		if(k<numChannels){
			m_MelFB[k].m_lowX=bin[k];
			m_MelFB[k].m_centerX=bin[k+1];
			m_MelFB[k].m_highX=bin[k+2];
		}
		for (i = bin[k]; i<bin[k+1]; i++){
			m_MelWeight[i] = (i-bin[k]+1)/(float)(bin[k+1]-bin[k]+1);
		}
    }

	free(freq);
	free(bin);
	
    return;
}

void PRFFT_NEW(float *a, float *b, int m, int n_pts, int iff)
{
	int	l,lmx,lix,lm,li,j1,j2,ii,jj,nv2,nm1,k,lmxy,n;
	float	scl,s,c,arg,T_a,T_b;
	
	n = 1 << m;
	for( l=1 ; l<=m ; l++ ) {
		lmx = 1 << (m - l) ;
		lix = 2 * lmx ;
		scl = 2 * (float)M_PI/(float)lix ;
		
		if(lmx < n_pts) lmxy = lmx ;
		else lmxy = n_pts ;
		for( lm = 1 ; lm <= lmxy ; lm++ ) {
			arg=((float)(lm-1)*scl) ;
			c = (float)cos(arg) ;
			s = iff * (float)sin(arg) ;
			
			for( li = lix ; li <= n ; li += lix ) {
				j1 = li - lix + lm ;
				j2 = j1 + lmx ;
				if(lmxy != n_pts ) {
					T_a=a[j1-1] - a[j2-1] ;
					/* T_a : real part */
					T_b=b[j1-1] - b[j2-1] ;
					/* T_b : imaginary part */
					a[j1-1] = a[j1-1] + a[j2-1] ;
					b[j1-1] = b[j1-1] + b[j2-1] ;
					a[j2-1] = T_a*c + T_b*s ;
					b[j2-1] = T_b*c - T_a*s ;
				} else{
					a[j2-1] = a[j1-1]*c + b[j1-1]*s ;
					b[j2-1] = b[j1-1]*c - a[j1-1]*s ;
				}
			}
		}
	}
	nv2 = n/2 ;
	nm1 = n - 1 ;
	jj = 1 ;
	
	for( ii = 1 ; ii <= nm1 ;) {
		if( ii < jj ) {
			T_a = a[jj-1] ;
			T_b = b[jj-1] ;
			a[jj-1] = a[ii-1] ;
			b[jj-1] = b[ii-1] ;
			a[ii-1] = T_a ;
			b[ii-1] = T_b ;
		}
		k = nv2 ;
		while( k < jj ) {
			jj = jj - k ;
			k = k/2 ;
		}
		jj = jj + k ;
		ii = ii + 1 ;
	}
	if (iff == (-1)) {
		for ( l=0 ; l<n ; l++ ) {
			a[l]/=(float)n;
			b[l]/=(float)n;
		}
	}
}

void MfccMelFilterBank (float *sigFFT, int numChannels, float* output, bool bNormalize)
{
    float sum, wsum;
    int i, k;
	MfccMelFB *melFB;
	
    for (k=0; k<numChannels; k++) {
		melFB = (MfccMelFB *)&m_MelFB[k];
        sum = sigFFT[melFB->m_centerX];
		wsum = 1;
        for (i = melFB->m_lowX; i < melFB->m_centerX; i++) {
            sum += m_MelWeight[i] * sigFFT[i];
			wsum += m_MelWeight[i];
		}
		for (i = melFB->m_centerX+1; i <= melFB->m_highX; i++) {
            sum += (1 - m_MelWeight[i-1]) * sigFFT[i];
			wsum += (1 - m_MelWeight[i-1]);
		}
        output[k] = sum;
		if (bNormalize) {
//			assert(wsum>0);
			output[k] /= wsum;
		}
    }

}

int	_filterbank_basic(float *pSamps, int frameSize, float *filterBank, int filterBankOrder, int fftWinSize, 
										int cep_smooth, int cepFilterLen)
{

	int ii;
	float* paa = (float*)malloc(fftWinSize*4);
	float* pbb = (float*)malloc(fftWinSize*4);

	int uiLogFFTSize = (int)(log((double)fftWinSize)/log((double)2)+0.5);

	// 第一个参数：开始频率
	MfccInitMelFilterBanks((float)MFCC_START_FREQUENCY, (float)UNIFIED_SAMPLING_RATE, fftWinSize, filterBankOrder);
	
	for (ii=0; ii<frameSize; ii++) { paa[ii] = pSamps[ii]; pbb[ii]=0; }
	for (ii=frameSize; ii<fftWinSize; ii++) paa[ii] = pbb[ii] = 0;
	
	PRFFT_NEW(&paa[0], &pbb[0], uiLogFFTSize, frameSize, 1);	
	for (ii=0; ii<fftWinSize; ii++){
		paa[ii] = paa[ii]*paa[ii] + pbb[ii]*pbb[ii];
		pbb[ii] = 0.0;
	}
	MfccMelFilterBank(&paa[0], filterBankOrder, &filterBank[0], true);	
	for (ii = 0; ii < filterBankOrder; ii++) 
		filterBank[ii] = 0.5*LogE(filterBank[ii]);
		
	free(paa);
	free(pbb);
	
	return 0;
}

void MfccDCT(float *x, float *dctMatrix, int ceporder, int numChannels, double *mel_cep)
{
    int i, j;
    for (i = 0; i <= ceporder; i++) {
        mel_cep[i] = 0.0;
        for (j = 0; j < numChannels; j++){
            mel_cep[i] += x[j] * dctMatrix[i * numChannels + j];
		}
    }	
    return;
}

void main_old(int argc, char* argv[])
{
	double pMfcc[FE_CEP_ORDER+1];
	short pSamps[NumBinsInFftWinM];
    FILE*  fp;
	int templen;

	//sample rate=32k 每个样点16BIT frame_size=512  fft_size=512  相邻两帧间重叠128个样点   
	//滤波器个数＝23  MFCC个数＝12＋1
	m_logEnergyFloor = FE_MIN_LOG_ENERGY;
	m_energyFloor = (float)exp(m_logEnergyFloor);

	if ((fp=fopen(argv[1],"rb")) == NULL) {
		printf("Open Error\n");
	}

	while ((templen=fread(pSamps, sizeof(short), NumSamplesInFrameM, fp)) == NumSamplesInFrameM) {

		// 这个是接口函数
		CalculateMFCC(pSamps, NumSamplesInFrameM, pMfcc, FE_CEP_ORDER, NumBinsInFftWinM);

		long offset = NumSamplesInFrameM-NumSamplesInFrameSkipM;
		offset *= sizeof(short);
		fseek(fp, -offset, SEEK_CUR);
	}
	
	fclose(fp);
}

#endif	// MFCC_METHOD_002
