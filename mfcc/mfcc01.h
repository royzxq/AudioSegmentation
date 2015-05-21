
#ifndef __MFCC_ANOTHER_ONE__
#define __MFCC_ANOTHER_ONE__

#include "..\parameters.h"

// Cepstrum
#define	FE_CEP_ORDER			12 /* Number of cepstral coefficients is CEPORDER+1 including c0 */

// Dynamic features
#define FE_DELTA                5 /* Order of delta coefficients */
#define FE_DDELTA               3 /* Order of acceleration (delta-delta) coefficients */

// Filter bank
#define FE_NUM_CHANNELS			23 /* Number of bands in filter bank */
#define FE_FBANK_FLOOR			(-50.0) /* Energy floor for filter bank coefficients */
#define FE_MIN_ENERGY			(1)
#define FE_MIN_LOG_ENERGY		(0)
#define	M_PI		3.14159265358979323846
//#define NR_NUM_CHANNELS             23
#define NR_FL                       17

#define MFCC_START_FREQUENCY		0.0	//64.0
#define EMPHASIZING_FACTOR			0.97

typedef struct { /* mel filter banks */
    int m_lowX;
    int m_centerX;
    int m_highX;
} MfccMelFB;

typedef struct {
    int m_lowX;
    int m_centerX;
    int m_highX;
	float m_sumWeight;
} WfMelFB; /* mel filter bank for noise reduction */


int preemphasize(float *sample, int sampleN);
int preprocessing(const short *sample, int sampleN, float *out);
int Do_hamm_window(float *inputA, int inputN);
int CalculateMFCC(const short *sample, int frameSize, double *pMfcc, int ceporder, int fft_size);
int MfccInitDCTMatrix (float *dctMatrix, int ceporder, int numChannels);
int _mel_cepstrum_basic(float *sample, int frameSize, double *mel_cep, int fborder, int ceporder, int fft_size);
void MfccInitMelFilterBanks(float startingFrequency, float samplingRate, int fftLength, int numChannels);
float LogE(float x);
void PRFFT_NEW(float *a, float *b, int m, int n_pts, int iff);
void MfccMelFilterBank(float *sigFFT, int numChannels, float* output, bool bNormalize);
int	_filterbank_basic(float *sample, int frameSize, float *filter_bank, int fborder, int fftSize, int cep_smooth, int cepFilterLen);
void MfccDCT(float *x, float *dctMatrix, int ceporder, int numChannels, double *mel_cep);

/*
class CCalcMFCC {
private:
	float fF0;

public:
    CCalcMFCC();
    ~CCalcMFCC();
};
*/

#endif // __MFCC_ANOTHER_ONE__

