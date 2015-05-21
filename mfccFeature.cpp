

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

// MFCC 特征，。。。
// "m_pdbMFCC[...]" 特征向量序列（每帧一个特征向量）
void CProcessingObj::MakeFeatureVectorMFCC()
{
	int ss, ii, mm;
	if (m_pMeanMFCC) {
		for (ii=0; ii<m_nNumFrames; ii++) {
			ss = NumDimsMfccM*ii;
			for (mm=0; mm<NumDimsMfccM; mm++) {
				m_pMeanMFCC[mm] += m_pdbMFCC[ss+mm];	// 和
				if (m_pStdMFCC)
					m_pStdMFCC[mm] += m_pdbMFCC[ss+mm]*m_pdbMFCC[ss+mm];	// 平方和
			}
		}
		for (mm=0; mm<NumDimsMfccM; mm++) {
			m_pMeanMFCC[mm] /= m_nNumFrames;
			if (m_pStdMFCC) {
				m_pStdMFCC[mm] /= m_nNumFrames;
				m_pStdMFCC[mm] -= m_pMeanMFCC[mm]*m_pMeanMFCC[mm];	// “平方的均值”减去“均值的平方”
				m_pStdMFCC[mm] = sqrt(m_pStdMFCC[mm]);	// 再开方
			}
		}
	}
// 一阶差分，。。。
	if (m_pMeanDeriMFCC) {
		for (ii=1; ii<m_nNumFrames; ii++) {// form 1 on, ...
			ss = NumDimsMfccM*ii;
			for (mm=0; mm<NumDimsMfccM; mm++) {
				m_pMeanDeriMFCC[mm] += (m_pdbMFCC[ss+mm]-m_pdbMFCC[ss-NumDimsMfccM+mm]);	// 差分和
				if (m_pStdDeriMFCC)
					m_pStdDeriMFCC[mm] += (m_pdbMFCC[ss+mm]-m_pdbMFCC[ss-NumDimsMfccM+mm])*
											(m_pdbMFCC[ss+mm]-m_pdbMFCC[ss-NumDimsMfccM+mm]);	// 差分平方和
			}
		}
		for (mm=0; mm<NumDimsMfccM; mm++) {
			m_pMeanDeriMFCC[mm] /= m_nNumFrames-1;	// ！！！
			if (m_pStdDeriMFCC) {
				m_pStdDeriMFCC[mm] /= m_nNumFrames-1;	// ！！！
				m_pStdDeriMFCC[mm] -= m_pMeanDeriMFCC[mm]*m_pMeanDeriMFCC[mm];	// 减均值的平方
				m_pStdDeriMFCC[mm] = sqrt(m_pStdDeriMFCC[mm]);
			}
		}
	}
}

