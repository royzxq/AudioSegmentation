
#ifndef	__FFT_FEATURES_H__
#define __FFT_FEATURES_H__

#include "..\mydefs.h"

int CalcTempFeatures(const short *pshSamps, int nNumSamps, double *pdbout);
int CalcSpectralFeatures(const cpxv_t *pFT, int nNumBins, double *pdbout);

int GetNumDimsMFCC();
int NumDimsFrameSpectralFeatures();
int NumDimsOtherFeatures();

//////////////////////////////////////////////////////////////////////////////////////////////////////

class CCalcMFCC {
private:
	int m_samplingRate;	// 信号采样率
	short m_nNumBins;	// 傅里叶谱分量个数

public:
	void SetParams(int srate, short numbins);
//	void CalcMFCC_FFT(const cpxv_t *pFSP, double *pmfcc, int nNumMfccDims);
	int CalcFrameSpectralFeatures(const cpxv_t *pFSP, double *pdbfeatures);

public:
	CCalcMFCC(void);
	virtual ~CCalcMFCC(void);
};

#endif	// __FFT_FEATURES_H__
