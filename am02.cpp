

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

void CProcessingObj::MakeFeatureVectorAM()
{
	int ss;
	int nNumVectors;
// AM 特征，。。。
	if (m_pMeanAM == NULL)
		goto NEXT_STEP;

	nNumVectors = 0;
	for (int ii=NumFramesAreaMomentsSrcM-1; ii<m_nNumFrames; ii++) {
		ss = NumDimsAreaMomentsM*ii;
		for (int iDim=0; iDim<NumDimsAreaMomentsM; iDim++) {
			m_pMeanAM[iDim] += m_pdbAM[ss+iDim];	// 和
		}
		nNumVectors++;
	}
	for (int iDim=0; iDim<NumDimsAreaMomentsM; iDim++) {
		m_pMeanAM[iDim] /= nNumVectors;
	}
	if (m_pStdAM && nNumVectors >= 2) {
		for (int ii=NumFramesAreaMomentsSrcM-1; ii<m_nNumFrames; ii++) {
			ss = NumDimsAreaMomentsM*ii;
			for (int iDim=0; iDim<NumDimsAreaMomentsM; iDim++) {
				m_pStdAM[iDim] += (m_pdbAM[ss+iDim]-m_pMeanAM[iDim])*(m_pdbAM[ss+iDim]-m_pMeanAM[iDim]);	// 
			}
		}
		for (int iDim=0; iDim<NumDimsAreaMomentsM; iDim++) {
			m_pStdAM[iDim] = sqrt(m_pStdAM[iDim]/(nNumVectors-1));
		}
	}

NEXT_STEP:

	if (m_pMeanDeriAM == NULL)
		return;

	nNumVectors = 0;
	for (int ii=NumFramesAreaMomentsSrcM; ii<m_nNumFrames; ii++) {
		ss = NumDimsAreaMomentsM*ii;
		int pp = ss-NumDimsAreaMomentsM;
		for (int mm=0; mm<NumDimsAreaMomentsM; mm++) {
			m_pMeanDeriAM[mm] += (m_pdbAM[ss+mm]-m_pdbAM[pp+mm]);	// 和
		}
		nNumVectors++;
	}
	for (int mm=0; mm<NumDimsAreaMomentsM; mm++) {
		m_pMeanDeriAM[mm] /= nNumVectors;
	}
	if (m_pStdDeriAM && nNumVectors >= 2) {
		for (int ii=NumFramesAreaMomentsSrcM; ii<m_nNumFrames; ii++) {
			ss = NumDimsAreaMomentsM*ii;	// 当前帧的向量的起始位置
			int pp = ss-NumDimsAreaMomentsM;
			for (int mm=0; mm<NumDimsAreaMomentsM; mm++) {
				m_pStdDeriAM[mm] += ((m_pdbAM[ss+mm]-m_pdbAM[pp+mm])-m_pMeanDeriAM[mm])*
									((m_pdbAM[ss+mm]-m_pdbAM[pp+mm])-m_pMeanDeriAM[mm]);
			}
		}
		for (int mm=0; mm<NumDimsAreaMomentsM; mm++) {
			m_pStdDeriAM[mm] = sqrt(m_pStdDeriAM[mm]/(nNumVectors-1));
		}
	}
}

void CProcessingObj::PrepareSrcAM()
{
	memmove(&m_pSrcAM[0][0], &m_pSrcAM[1][0], (NumFramesAreaMomentsSrcM-1)*WIDTH_SOURCE_DATA_AM*sizeof(double));
	double *pdbSrcData = &m_pSrcAM[NumFramesAreaMomentsSrcM-1][0];
	for (int iBin=0; iBin<WIDTH_SOURCE_DATA_AM; iBin++) {
		pdbSrcData[iBin] = sqrt(m_fdom[iBin].im*m_fdom[iBin].im+m_fdom[iBin].re*m_fdom[iBin].re);
	}
}

