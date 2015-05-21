

#include "stdafx.h"

#include <assert.h>
#include <FLOAT.H>	// for DBL_MAX

#include <math.h>
#include <stdio.h>

#include <stdio.h>
#include <malloc.h>

#include <string.h>	// for "memmove(...)"

// 注意：当每个八度计算 12 个等间隔音高时，程序要修改（特征向量维数）

#include "procobj.h"

#include "fft\fft.h"

#ifdef __USE_SPECTRAL_SHAPE_FEATURES

#include "order_list.h"

void CProcessingObj::MakeFeatureVectorSpectralShape()
{
	if (m_pMeanSpectralShape) {
		int ss;
		for (int ii=0; ii<m_nNumFrames; ii++) {
			ss = NUM_DIMS_SPECTRAL_SHAPE*ii;
			for (int mm=0; mm<NUM_DIMS_SPECTRAL_SHAPE; mm++) {
				m_pMeanSpectralShape[mm] += m_pSpectralShape[ss+mm];	// 和
				if (m_pStdSpectralShape)
					m_pStdSpectralShape[mm] += m_pSpectralShape[ss+mm]*m_pSpectralShape[ss+mm];	// 平方和
			}
		}
		for (int mm=0; mm<NUM_DIMS_SPECTRAL_SHAPE; mm++) {
			m_pMeanSpectralShape[mm] /= m_nNumFrames;
			if (m_pStdSpectralShape) {
				m_pStdSpectralShape[mm] /= m_nNumFrames;
				m_pStdSpectralShape[mm] -= m_pMeanSpectralShape[mm]*m_pMeanSpectralShape[mm];	// 减均值平方
				m_pStdSpectralShape[mm] = sqrt(m_pStdSpectralShape[mm]);
			}
		}
	}
}

// 频带划分
static int BandUpperBounds[] = {100, 300, 600, 1200, 2400, 4800, 9600};

/*
01 : 0000 - 0100
02 : 0100 - 0300
03 : 0300 - 0600
04 : 0600 - 1200
05 : 1200 - 2400
06 : 2400 - 4800
07 : 4800 - 9600
*/

#ifndef __USE_MEL_BAND_VALUE_AS_AM_SOURCE
void BandSum(const cpxv_t fdom[], double bandSums[])
{
	size_t uNumBands = sizeof(BandUpperBounds)/sizeof(BandUpperBounds[0]);
	assert(uNumBands == NUMBER_OF_OCTAVES);

	int ibin;	// FFT bin 下标

// (11 - 333)
	ibin = 0;
	for (int idx=0; idx<NUMBER_OF_OCTAVES; idx++) {// 7 个频带
		bandSums[idx] = 0.0;
		do {
			bandSums[idx] += sqrt(fdom[ibin].im*fdom[ibin].im+fdom[ibin].re*fdom[ibin].re);
			ibin++;
		} while (UNIFIED_SAMPLING_RATE*ibin/NumBinsInFftWinM <= BandUpperBounds[idx]);
	}
//
/*	for (int idx=0; idx<NUMBER_OF_OCTAVES; idx++) {
		bandSums[idx] = log(1.0+bandSums[idx]);
	}*/

	return;
}
#endif	// #ifndef __USE_MEL_BAND_VALUE_AS_AM_SOURCE

void SpectralShape(const cpxv_t fdom[], double pSpectralShape[])
{
	size_t uNumBands = sizeof(BandUpperBounds)/sizeof(BandUpperBounds[0]);
	int* NNN = Malloc(int, uNumBands);	// 各个频带的 FFT bins 个数，7 个频带

	int ibin;	// FFT bin 下标

// (11 - 333)
	ibin = 0;
	for (int idx=0; idx<uNumBands; idx++) {// 7 个频带
		NNN[idx] = 0;
		do { NNN[idx]++; ibin++;
		} while (UNIFIED_SAMPLING_RATE*ibin/NumBinsInFftWinM <= BandUpperBounds[idx]);
	}

//
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

	for (int idx=0; idx<uNumBands; idx++) {// 7 个频带
		pSpectralShape[idx] = 0.0;
	}
//
// (22 - 333)
	double dbtmp;
	COrderedDoubles *pListMax, *pListMin;
	ibin = 0;
	for (int idx=0; idx<uNumBands; idx++) {// 7 个频带
		int nNumDoubles = NNN[idx]*0.2;
		if (nNumDoubles == 0) nNumDoubles = 1;
		pListMax = new COrderedDoubles(nNumDoubles, true);
		pListMin = new COrderedDoubles(nNumDoubles, false);
		do {	dbtmp = sqrt(fdom[ibin].im*fdom[ibin].im+fdom[ibin].re*fdom[ibin].re);
			pSpectralShape[idx] += dbtmp;
			pListMax->NewValueCome(dbtmp);
			pListMin->NewValueCome(dbtmp);
			ibin++;
		} while (UNIFIED_SAMPLING_RATE*ibin/NumBinsInFftWinM <= BandUpperBounds[idx]);
//		pSpectralShape[uNumBands+idx] = pListMin->AverageVal()/pListMax->AverageVal();
		pSpectralShape[uNumBands+idx] = log(1.0+pListMin->AverageVal())/log(1.0+pListMax->AverageVal());	// 明显更好！
		delete pListMax;
		delete pListMin;
	}
	free(NNN);
//
	for (int idx=0; idx<uNumBands; idx++) {
		pSpectralShape[idx] = log(1.0+pSpectralShape[idx]);
	}

	return;

// NUM_DIMS_SPECTRAL_SHAPE

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
//	double dbtmp;
	double dbsum = 0.0;
	pSpectralShape[0] = 0.0;
	for (int ibin=0; ibin<NumBinsInFftWinM/2; ibin++) {// 高于采样频率二分之一的频谱分量不予考虑
		dbtmp = sqrt(fdom[ibin].im*fdom[ibin].im+fdom[ibin].re*fdom[ibin].re);
		pSpectralShape[0] += (ibin+1)*dbtmp;
		dbsum += dbtmp;	// 幅值累加
	}
	if (dbsum > 0)
		pSpectralShape[0] /= dbsum;

// 带宽，。。。
	pSpectralShape[1] = 0.0;
	for (int ibin=0; ibin<NumBinsInFftWinM/2; ibin++) {// 高于采样频率二分之一的频谱分量不予考虑
		pSpectralShape[1] += (ibin+1-pSpectralShape[0])*(ibin+1-pSpectralShape[0])*
			sqrt(fdom[ibin].im*fdom[ibin].im+fdom[ibin].re*fdom[ibin].re);
	}
	if (dbsum > 0)
		pSpectralShape[1] /= dbsum;
	pSpectralShape[1] = sqrt(pSpectralShape[1]);

	dbtmp = 0.0;
	pSpectralShape[2] = NumBinsInFftWinM/2+1;
	for (int ibin=0; ibin<NumBinsInFftWinM/2; ibin++) {
		dbtmp += sqrt(fdom[ibin].im*fdom[ibin].im+fdom[ibin].re*fdom[ibin].re);
		if (dbtmp >= 0.95*dbsum) {
			pSpectralShape[2] = ibin;
			break;
		}
	}

	pSpectralShape[3] = 0.0;
	for (int ibin=1; ibin<NumBinsInFftWinM/2-1; ibin++) {
		pSpectralShape[3] += fabs(	log10(1.0+sqrt(fdom[ibin].im*fdom[ibin].im+fdom[ibin].re*fdom[ibin].re)) -
							(	log10(1.0+sqrt(fdom[ibin-1].im*fdom[ibin-1].im+fdom[ibin-1].re*fdom[ibin-1].re))+
								log10(1.0+sqrt(fdom[ibin].im*fdom[ibin].im+fdom[ibin].re*fdom[ibin].re))+
								log10(1.0+sqrt(fdom[ibin+1].im*fdom[ibin+1].im+fdom[ibin+1].re*fdom[ibin+1].re))
							)/3.0
						);
	}
	if (dbsum > 0)
		pSpectralShape[3] /= dbsum;
*/
}

#ifdef __TO_BE_CORRECTED

void CProcessingObj::CalcPre()
{
	for (int ibin=0; ibin<NumBinsInFftWinM; ibin++) {
		m_ftPre[ibin] = sqrt(m_fdom[ibin].re*m_fdom[ibin].re+m_fdom[ibin].im*m_fdom[ibin].im);
	}
}

// 返回值 <= 0 : 表明出错；> 0 : 表明正确
// 
int CProcessingObj::CalcTimbre(double timbre[])
{
// 111 - 频谱质心（bin 序号加 1），信号倍增，其值不变
	double dbtmp;
	double dbsum = 0.0;	// 一帧中所有 bins 的幅值总和
	timbre[0] = 0.0;
	timbre[3] = 0.0;	// 相邻帧频谱差别（欧氏距离）大小
	for (int ibin=0; ibin<NumBinsInFftWinM/2; ibin++) {
		dbtmp = sqrt(m_fdom[ibin].re*m_fdom[ibin].re+m_fdom[ibin].im*m_fdom[ibin].im);
		timbre[3] += (dbtmp - m_ftPre[ibin])*(dbtmp - m_ftPre[ibin]);
		timbre[0] += (ibin+1)*dbtmp;
		dbsum += dbtmp;	// 一帧中所有 bins 的幅值总和
	}
	if (dbsum > 0.0)
		timbre[0] /= dbsum;
	timbre[3] = sqrt(timbre[3]);

// 222 - band width（bin 间距，bins 个数），
	timbre[1] = 0.0;
// 333 - roll off（bin 序号），
	timbre[2] = 0.0;
	dbtmp = 0.0;
	for (int ibin=0; ibin<NumBinsInFftWinM/2; ibin++) {
		timbre[1] += fabs((ibin+1)-timbre[0])*sqrt(m_fdom[ibin].re*m_fdom[ibin].re+m_fdom[ibin].im*m_fdom[ibin].im);
		if (timbre[2] == 0.0) {// 表明 roll off 点还未定！
			dbtmp += sqrt(m_fdom[ibin].re*m_fdom[ibin].re+m_fdom[ibin].im*m_fdom[ibin].im);
			if (dbtmp >= dbsum*0.95) {
				timbre[2] = ibin+1;	// roll off 点确定！
			}
		}
	}
	if (dbsum > 0.0)
		timbre[1] /= dbsum;	// 带宽（bins 个数）

// 444 - 

// 555 - compactness(from jAudio)
	timbre[4] = 0.0;
	for (int ibin=1; ibin<nNumBinsCQ-1; ibin++) {// 头尾各去掉一个 bin
		timbre[4] += fabs(	log10(1.0+ISMIR2009P0190_CCC*cqcq[ibin].re) -
							(	log10(1.0+ISMIR2009P0190_CCC*cqcq[ibin-1].re)+
								log10(1.0+ISMIR2009P0190_CCC*cqcq[ibin].re)+
								log10(1.0+ISMIR2009P0190_CCC*cqcq[ibin+1].re)
							)/3.0
						);
	}
	if (dbsum > 0.0)
		timbre[4] = timbre[4]/dbsum;

// 

	int idx = 5;
	int istart = 0;
	int NNN, MMM;
	for (int iband=0; iband<NUM_BANDS_TIMBRE; iband++) {// 分别处理每个频带，。。。
		// 先将当前频带内的 bins 按幅值大小排序，为找出几个最大的和最小的，。。。
		for (int ibin=istart; ibin<istart+NUM_EQUAL_TEMPERED_TONES_IN_OCTAVE-1 && ibin<nNumBinsCQ-1; 
								ibin++) {		
			for (int ibin02=ibin+1; ibin02<istart+NUM_EQUAL_TEMPERED_TONES_IN_OCTAVE && ibin02<nNumBinsCQ; 
									ibin02++) {
				if (cqcq[ibin].re >= cqcq[ibin02].re) {
					continue;
				}
				cqcq[0].im = cqcq[ibin].re;	// 借用 "cqcq[0].im" ！！！
				cqcq[ibin].re = cqcq[ibin02].re;
				cqcq[ibin02].re = cqcq[0].im;
			}
		}
		// 当前频带（/本八度）内实际 bins 个数
		if (istart+NUM_EQUAL_TEMPERED_TONES_IN_OCTAVE > nNumBinsCQ) {
		// 不够 "NUM_EQUAL_TEMPERED_TONES_IN_OCTAVE" 个，。。。
			NNN = nNumBinsCQ-istart;
		} else {
			NNN = NUM_EQUAL_TEMPERED_TONES_IN_OCTAVE;
		}
		MMM = floor(0.2*NNN);	// 参见微软人的文章，只取若干个值最大及值最小的 bins
		assert(MMM > 0);

// Peak，最大的几（MMM）个峰值的均值
//		timbre[idx] = 0.0;
		cqcq[0].im = 0.0;
		for (int ibin=0; ibin<MMM; ibin++) {
//			timbre[idx] += cqcq[istart+ibin].re;
			cqcq[0].im += cqcq[istart+ibin].re;
		}
/*
//		timbre[idx] = log10(1.0+ISMIR2009P0190_CCC*timbre[idx]/MMM);
		timbre[idx] = timbre[idx]/MMM;*/
//		idx++;
		cqcq[0].im /= MMM;	// 峰值均值

// Valley，最小的几（MMM）个谷值的均值
		NNN += istart-1;	// 本频带最右边的 bin 的下标
//		timbre[idx] = 0.0;
		cqcq[1].im = 0.0;
		for (int ibin=0; ibin<MMM; ibin++) {
//			timbre[idx] += cqcq[NNN-ibin].re;
			cqcq[1].im += cqcq[NNN-ibin].re;
		}
/*
//		timbre[idx] = log10(1.0+ISMIR2009P0190_CCC*timbre[idx]/MMM);
		timbre[idx] = timbre[idx]/MMM;*/
//		idx++;
		cqcq[1].im /= MMM;	// 谷值均值

// SCk
		assert(cqcq[0].im-cqcq[1].im >= 0.0);
		timbre[idx] = (cqcq[0].im-cqcq[1].im)/(cqcq[0].im+cqcq[1].im);
		idx++;

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		istart += NUM_EQUAL_TEMPERED_TONES_IN_OCTAVE;

	}
	assert(idx == NUM_DIMS_TIMBRE);

	return idx;
}

#endif

#endif	// #ifdef __USE_SPECTRAL_SHAPE_FEATURES



