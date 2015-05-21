

#include "stdafx.h"

#include <io.h>
#include <stdio.h>
// for _O_RDONLY etc.
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <FLOAT.H>
#include <direct.h>

#include "parameters.h"

#include "procobj.h"
#include "fft\fft.h"

#ifdef __USE_BEAT_HISTOGRAM

// 基于 m_pBandSum[...] 数据做计算，所以，在单元分类（/音频分段）阶段，单元跳时要前移该数据！！！

void CProcessingObj::MakeOnsets(double* pOnsetSeq)
{
	double *p1, *p2;
	int ifr;
	for (ifr=1; ifr<m_nNumFrames; ifr++) {
		p1 = m_pBandSum[ifr-1];	// 前帧
		p2 = m_pBandSum[ifr];	// 后帧
		pOnsetSeq[ifr] = 0.0;
		for (int iBandMel=0; iBandMel<NUM_DIMS_AM_SORCE_DATA; iBandMel++) {
			if (p2[iBandMel] > p1[iBandMel])
				pOnsetSeq[ifr] += p2[iBandMel]-p1[iBandMel];
		}
	}
}

void CProcessingObj::MakeFeatureVectorBH()
{
	double* pOnsetSeq = Malloc(double, m_nNumFrames);
	MakeOnsets(pOnsetSeq);

	int iFr;
/*	// 低通滤波，没什么作用，。。。
	for (iFr=1; iFr<m_nNumFrames; iFr++) {
		if (iFr == 1) {// 左界
			pOnsetSeq[0] = pOnsetSeq[iFr];
			pOnsetSeq[iFr] = (pOnsetSeq[iFr]+pOnsetSeq[iFr+1])/2.0;
		} else if (iFr == m_nNumFrames-1) {// 右界
			pOnsetSeq[iFr] = (pOnsetSeq[0]+pOnsetSeq[iFr])/2.0;
		} else {
			double dbtmp = pOnsetSeq[iFr];
			pOnsetSeq[iFr] = (pOnsetSeq[0]+pOnsetSeq[iFr]+pOnsetSeq[iFr+1])/3.0;
			pOnsetSeq[0] = dbtmp;
		}
	}*/
	cpxv_t* tdom = Malloc(cpxv_t, BH_FFT_WIN_WIDTH*2);
	cpxv_t* ffdom = tdom+BH_FFT_WIN_WIDTH;
	for (iFr=0; iFr<m_nNumFrames-1; iFr++) {// !!!
		// Hamming windowing
		tdom[iFr].re = (0.54-0.46*cos(TWO_PI*iFr/(m_nNumFrames-1)))*pOnsetSeq[iFr+1];	// !!!
		tdom[iFr].im = 0.0;
	}
	free(pOnsetSeq);
	for (; iFr<BH_FFT_WIN_WIDTH; iFr++) {// zero padding
		tdom[iFr].re = 0.0;
		tdom[iFr].im = 0.0;
	}
	FFT(BH_FFT_WIN_WIDTH, tdom, ffdom);
/*	for (int ibin=0; ibin<NUM_DIMS_BH; ibin++) {
		m_pFeatureBH[ibin] = sqrt(ffdom[ibin].im*ffdom[ibin].im+ffdom[ibin].re*ffdom[ibin].re);
	}*/

	// DCT, ...
	for (int uuu=0; uuu<NUM_DIMS_BH; uuu++) {
		m_pFeatureBH[uuu] = 0.0;
		for (int xxx=0; xxx<BH_FFT_WIN_WIDTH/2; xxx++) {
			m_pFeatureBH[uuu] += sqrt(ffdom[xxx].im*ffdom[xxx].im+ffdom[xxx].re*ffdom[xxx].re)*
				cos(MY_PI*(2.0*xxx+1.0)*uuu/BH_FFT_WIN_WIDTH);
		}
		if (uuu > 0) {
			m_pFeatureBH[uuu] *= 1.4142135623730950488016887242097;	// 2 开方
		}
	}

	free(tdom);
}

#endif	// __USE_BEAT_HISTOGRAM
