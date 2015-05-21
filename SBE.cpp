

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

#ifdef __USE_SUB_BAND_ENERGY

// 基于 m_pBandSum[...] 数据做计算，所以，在单元分类（/音频分段）阶段，单元跳时要前移该数据！！！

void CProcessingObj::MakeFeatureVectorSBE()
{
	if (m_pFeatureSBE == NULL) return;

	double* bandsEng = Malloc(double, NUM_DIMS_AM_SORCE_DATA);
#ifndef __USE_MEL_BAND_VALUE_AS_AM_SOURCE
	double dbsum = 0.0;
#endif
	int iBand;
	for (iBand=0; iBand<NUM_DIMS_AM_SORCE_DATA; iBand++) {
		bandsEng[iBand] = 0.0;
		for (int ifrm=0; ifrm<m_nNumFrames; ifrm++) {
			bandsEng[iBand] += m_pBandSum[ifrm][iBand];
		}
		bandsEng[iBand] /= m_nNumFrames;
//		bandsEng[iBand] = log(1.0+bandsEng[iBand]);
#ifndef __USE_MEL_BAND_VALUE_AS_AM_SOURCE
		dbsum += bandsEng[iBand];
#endif
	}
#ifndef __USE_MEL_BAND_VALUE_AS_AM_SOURCE
	if (dbsum > 0.0)
	for (iBand=0; iBand<NUM_DIMS_AM_SORCE_DATA; iBand++) {
		bandsEng[iBand] /= dbsum;
	}
#endif
	for (int kkk=1; kkk<=NUM_DIMS_SBE; kkk++) {// 最终特征维数
#ifdef __USE_MEL_BAND_VALUE_AS_AM_SOURCE
		m_pFeatureSBE[kkk-1] = 0.0;
		for (int mmm=1; mmm<=NUM_DIMS_AM_SORCE_DATA; mmm++) {
			m_pFeatureSBE[kkk-1] += bandsEng[mmm-1]*cos((double)kkk*MY_PI/NUM_DIMS_AM_SORCE_DATA*((double)mmm-0.5));
		}
#else
		m_pFeatureSBE[kkk-1] = bandsEng[kkk-1];
#endif	// __USE_MEL_BAND_VALUE_AS_AM_SOURCE
	}
	free(bandsEng);
}

#endif	// __USE_SUB_BAND_ENERGY

