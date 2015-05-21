

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

// LPC
// "m_pLPC" LPC 特征向量（每帧一个特征向量）序列
void CProcessingObj::MakeFeatureVectorLPC()
{
	int ss;
	if (m_pMeanLPC) {
		for (int ii=0; ii<m_nNumFrames; ii++) {
			ss = NUM_DIMS_LPC*ii;
			for (int mm=0; mm<NUM_DIMS_LPC; mm++) {
				m_pMeanLPC[mm] += m_pLPC[ss+mm];	// 和
				if (m_pStdLPC)
					m_pStdLPC[mm] += m_pLPC[ss+mm]*m_pLPC[ss+mm];	// 平方和
			}
		}
		for (int mm=0; mm<NUM_DIMS_LPC; mm++) {
			m_pMeanLPC[mm] /= m_nNumFrames;
			if (m_pStdLPC) {
				m_pStdLPC[mm] /= m_nNumFrames;
				m_pStdLPC[mm] -= m_pMeanLPC[mm]*m_pMeanLPC[mm];	// 减均值平方
				m_pStdLPC[mm] = sqrt(m_pStdLPC[mm]);
			}
		}
	}
	if (m_pMeanDeriLPC) {
		for (int ii=1; ii<m_nNumFrames; ii++) {// from 1 on, ...
			ss = NUM_DIMS_LPC*ii;
			for (int mm=0; mm<NUM_DIMS_LPC; mm++) {
				m_pMeanDeriLPC[mm] += (m_pLPC[ss+mm]-m_pLPC[ss-NUM_DIMS_LPC+mm]);	// 和
				if (m_pStdDeriLPC)
					m_pStdDeriLPC[mm] += (m_pLPC[ss+mm]-m_pLPC[ss-NUM_DIMS_LPC+mm])*
											(m_pLPC[ss+mm]-m_pLPC[ss-NUM_DIMS_LPC+mm]);	// 平方和
			}
		}
		for (int mm=0; mm<NUM_DIMS_LPC; mm++) {
			m_pMeanDeriLPC[mm] /= m_nNumFrames-1;	// 减一！
			if (m_pStdDeriLPC) {
				m_pStdDeriLPC[mm] /= m_nNumFrames-1;	// 减一！
				m_pStdDeriLPC[mm] -= m_pMeanDeriLPC[mm]*m_pMeanDeriLPC[mm];	// 减均值平方
				m_pStdDeriLPC[mm] = sqrt(m_pStdDeriLPC[mm]);
			}
		}
	}
}

