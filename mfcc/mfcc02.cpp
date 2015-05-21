
#include "stdafx.h"

#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include <float.h>

#include "mfcc01.h"

// (SPHINX-III uses 40)
static int numMelFilters = 23;
// (or 64 Hz?)
static double lowerFilterFreq = 0.0;	//133.3334;
// (or half of sampling freq.?)
static double upperFilterFreq = 6855.4976;

static double preEmphasisAlpha = 0.95;
static int numCepstra = NumDimsMfccM;

double freqToMel(double freq)
{
	return 2595.0 * log10(1.0 + freq / 700.0);
}
double inverseMel(double xx)
{
	double temp = pow(10.0, xx / 2595.0) - 1.0;
	return 700.0 * (temp);
}

double centerFreq(int ii, double samplingRate)
{
	double* mel = new double[2];
	mel[0] = freqToMel(lowerFilterFreq);
	mel[1] = freqToMel(samplingRate / 2);     
	// take inverse mel of:
	double temp = mel[0] + ((mel[1] - mel[0]) / (numMelFilters + 1)) * ii;
	delete []mel;
	
	return inverseMel(temp);
}

int* fftBinIndices(double samplingRate, int frameSize) 
{
	int* cbin = new int[numMelFilters + 2];	// Mel ÂË²¨Æ÷¸öÊý

	cbin[0] = (int)(lowerFilterFreq / samplingRate * frameSize);
	cbin[NUM_MEL_BANDS+1] = (int)(frameSize / 2);
        
	for (int ii = 1; ii <= numMelFilters; ii++) {// 
		double fc = centerFreq(ii, samplingRate);
		cbin[ii] = (int)(fc / samplingRate * frameSize);
	}
        
	return cbin;
}

double* magnitudeSpectrum(const cpxv_t* fdom)
{
	double* magSpectrum = new double[NumBinsInFftWinM];
	// calculate magnitude spectrum
	for (int k = 0; k < NumBinsInFftWinM; k++){
		magSpectrum[k] = sqrt(fdom[k].im * fdom[k].im + fdom[k].re * fdom[k].re);
	}

	return magSpectrum;
}

double* melFilter(const double bin[], const int cbin[])
{
	double* temp = new double[numMelFilters + 2];

	for (int k = 1; k <= numMelFilters; k++){
            double num1 = 0, num2 = 0;

            for (int i = cbin[k - 1]; i <= cbin[k]; i++){
                num1 += ((i - cbin[k - 1] + 1) / (cbin[k] - cbin[k-1] + 1)) * bin[i];
            }

            for (int i = cbin[k] + 1; i <= cbin[k + 1]; i++){
                num2 += (1 - ((i - cbin[k]) / (cbin[k + 1] - cbin[k] + 1))) * bin[i];
            }

            temp[k] = num1 + num2;
	}

	double* fbank = new double[numMelFilters];
	for (int i = 0; i < numMelFilters; i++) {
		fbank[i] = temp[i + 1];
	}

	delete []temp;

	return fbank;
}

double* nonLinearTransformation(const double fbank[])
{
        double* ff = new double[numMelFilters];
        double FLOOR = -50.0;
        
        for (int i = 0; i < numMelFilters; i++){
            ff[i] = log(DBL_MIN+fbank[i]);	// !!!!!!!!!!
            
            // check if ln() returns a value less than the floor
            if (ff[i] < FLOOR) ff[i] = FLOOR;
        }
        
        return ff;
}

int cepCoefficients(const double ff[], double cepc[], int numCepstra01)
{
//	double* cepc = new double[numCepstra01];
	for (int i = 0; i < numCepstra01; i++) {
		for (int j = 1; j <= numMelFilters; j++){
			cepc[i] += ff[j - 1] * cos(MY_PI * i / numMelFilters * (j - 0.5));
		}
	}
        
	return 0;
}

int CalcMfccFromMagnitudeSpectrum(const cpxv_t* fdom, double cepc_out[], int numCepstraOut)
{
	// Magnitude Spectrum
	double* binLast = magnitudeSpectrum(fdom);
	// Mel Filtering
	int* cbinLast = fftBinIndices(UNIFIED_SAMPLING_RATE, NumBinsInFftWinM);

	// get Mel Filterbank
	double* fbankLast = melFilter(binLast, cbinLast);
	delete []binLast;
	delete []cbinLast;

	// Non-linear transformation
	double* ffLast = nonLinearTransformation(fbankLast);
	delete []fbankLast;

	// Cepstral coefficients
	cepCoefficients(ffLast, cepc_out, numCepstraOut);
	delete []ffLast;

	return 0;
}

int preemphasize(double samps[], int sampleN, float emphFac)
{
	/* Setting emphFac=0 turns off preemphasis. */
	for (int ii = sampleN-1; ii > 0; ii--) {
		samps[ii] = samps[ii] - emphFac * samps[ii-1];
	}
	samps[0] = (1.0 - emphFac) * samps[0];

	return 0;
}

double* preEmphasis(short inputSignal[], int NNN)
{
	double* outputSignal = new double[NNN];
        
	// apply pre-emphasis to each sample
	for (int n = 1; n < NNN; n++) {
		outputSignal[n] = inputSignal[n] - preEmphasisAlpha * inputSignal[n - 1];
	}
        
	return outputSignal;
}

#ifdef MFCC_METHOD_002

extern float m_logEnergyFloor;
extern float m_energyFloor;

int preprocessing(const short *sample, int sampleN, float *out)
{
	for (int ii=0; ii<sampleN; ii++) {
		out[ii] = sample[ii];
	}
	//if (m_dither) Dither(out, sampleN);
	preemphasize(out, sampleN);
	return 0;
}

int Do_hamm_window(float *inputA, int inputN)
{
	int ii;
	float* hamm_window = (float*)malloc(inputN*4);
	float temp = (float)(2 * M_PI / (float)(inputN-1));

    for (ii=0 ; ii<inputN; ii++ ) hamm_window[ii] = (float)(0.54 - 0.46*cos(temp*ii));

	for (ii=0 ; ii<inputN; ii++ ) inputA[ii] = hamm_window[ii]*inputA[ii];
	
	free(hamm_window);
	
	return 0;
}

float LogE(float x)
{
	if(x>m_energyFloor) return log(x);
	else return m_logEnergyFloor;
}

#endif	// MFCC_METHOD_002

/*
CCalcMFCC::CCalcMFCC()
{
	fF0 = 0.0;

}

CCalcMFCC::~CCalcMFCC()
{

}
*/

