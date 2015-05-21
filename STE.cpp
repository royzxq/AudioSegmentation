

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
#include "order_list.h"

#ifdef __USE_STE_FEATURES

#define RATIO_TO_STE_AVERAGE	0.05

#define NNN_DDD_WWW		10

// "m_pDataSTE[...]" 短时能量（信号绝对值的平均值）序列
void CProcessingObj::MakeFeatureVectorSTE()
{
	if (m_pFeatureSTE == NULL) return;
	
// 求均值，。。。
	double dbval = 0.0;
	for (int iframe=0; iframe<m_nNumFrames; iframe++) {
		dbval += m_pDataSTE[iframe];
	}
	dbval /= m_nNumFrames;

// （111）低能点所占比例，。。。
	int idx = 0;
	m_pFeatureSTE[idx] = 0.0;
	for (int iframe=0; iframe<m_nNumFrames; iframe++) {
		//dbval*RATIO_TO_STE_AVERAGE
		if (m_pDataSTE[iframe] <= MINIMUM_MEAN_ABS_SIGNAL_VALUE*2) m_pFeatureSTE[idx] += 1.0;
	}
	m_pFeatureSTE[idx] /= m_nNumFrames;
	idx++;

// （222）低能段平均段长，。。。
	int nnn = 0;
	int iii = -1;
	int iframe;
	m_pFeatureSTE[idx] = 0.0;
	for (iframe=0; iframe<m_nNumFrames; iframe++) {
		//dbval*RATIO_TO_STE_AVERAGE
		if (m_pDataSTE[iframe] <= MINIMUM_MEAN_ABS_SIGNAL_VALUE*2) {
			if (iii == -1) iii = iframe;	// 低能段起始位置
		} else {
			if (iii != -1) {// 处理这个低能段
				assert(iframe-iii > 0);
				m_pFeatureSTE[idx] += iframe-iii;	// 低能段长
				nnn++;
			}
			iii = -1;
		}
	}
	if (iii != -1) {
		assert(iframe-iii > 0);
		m_pFeatureSTE[idx] += iframe-iii;	// 低能段长
		nnn++;
	}
	if (nnn > 0)
		m_pFeatureSTE[idx] /= nnn;
	idx++;
//
	// 低通滤波，。。。
	double* ptmp = Malloc(double, NumFrameSkipsInUnitM); 
	assert(m_nNumFrames > NNN_DDD_WWW);
	for (int iframe=NNN_DDD_WWW-1; iframe<m_nNumFrames; iframe++) {
		ptmp[iframe] = 0.0;
		for (nnn=iframe-(NNN_DDD_WWW-1); nnn<=iframe; nnn++) {
			ptmp[iframe] += m_pDataSTE[nnn];	// 0.5+0.5*cos(TWO_PI*(iframe-nnn)/(2.0*NNN_DDD_WWW-1.0))	// half Hanning
		}
	}

/*	COrderedDoubles ordDoublesUp(5, true);
	COrderedDoubles ordDoublesDown(5, true);*/

	nnn = 0;	// 上升次数
	iii = 0;	// 下降次数
	m_pFeatureSTE[idx] = 0.0;	// 上升度
	m_pFeatureSTE[idx+1] = 0.0;	// 下降度

	ptmp[1] = 0.0;
	for (int iframe=NNN_DDD_WWW-1; iframe<m_nNumFrames; iframe++) {
		if (iframe == NNN_DDD_WWW-1) {// 左界
			if (ptmp[iframe] > ptmp[iframe+1]) {// 峰
				ptmp[0] = ptmp[iframe]; ptmp[1] = +1.0;
			} else if (ptmp[iframe] < ptmp[iframe+1]) {// 谷
				ptmp[0] = ptmp[iframe]; ptmp[1] = -1.0;
			}
		} else if (iframe == m_nNumFrames-1) {// 右界
			if (ptmp[1] < 0.0 && ptmp[iframe-1] < ptmp[iframe]) {// 峰，上升
				m_pFeatureSTE[idx] += ptmp[iframe] - ptmp[0];
				nnn++;
			} else if (ptmp[1] > 0.0 && ptmp[iframe-1] > ptmp[iframe]) {// 谷，下降
				m_pFeatureSTE[idx+1] += ptmp[0] - ptmp[iframe];
				iii++;
			}
		} else {// 中间点，。。。
			if (ptmp[iframe-1] <= ptmp[iframe] && ptmp[iframe] > ptmp[iframe+1]) {// 峰
				if (ptmp[1] < 0.0) {// 上升
					m_pFeatureSTE[idx] += ptmp[iframe] - ptmp[0]; nnn++;
				}
				ptmp[0] = ptmp[iframe]; ptmp[1] = +1.0;
			} else if (ptmp[iframe-1] >= ptmp[iframe] && ptmp[iframe] < ptmp[iframe+1]) {// 谷
				if (ptmp[1] > 0.0) {// 下降
					m_pFeatureSTE[idx+1] += ptmp[0] - ptmp[iframe]; iii++;
				}
				ptmp[0] = ptmp[iframe]; ptmp[1] = -1.0;
			}
		}
	}
	free(ptmp);
//
/*	for (int iframe=1; iframe<m_nNumFrames; iframe++) {// from 1 on, ...
		if (m_pDataSTE[iframe-1] < m_pDataSTE[iframe]) {// 上升，。。。
			m_pFeatureSTE[idx] += m_pDataSTE[iframe]-m_pDataSTE[iframe-1];
			nnn++;
		} else if (m_pDataSTE[iframe-1] > m_pDataSTE[iframe]) {// 下降，。。。
			m_pFeatureSTE[idx+1] += m_pDataSTE[iframe-1]-m_pDataSTE[iframe];
			iii++;
		}
	}*/

	if (nnn > 0)
		m_pFeatureSTE[idx] /= nnn;	// 上升度
	m_pFeatureSTE[idx] /= dbval;
	if (iii > 0)
		m_pFeatureSTE[idx+1] /= iii;	// 下降度
	m_pFeatureSTE[idx+1] /= dbval;
	idx += 2;
//
	COrderedDoubles ordDoublesBig(m_nNumFrames*0.2, true);
	COrderedDoubles ordDoublesSmall(m_nNumFrames*0.2, false);
	for (int iframe=0; iframe<m_nNumFrames; iframe++) {
		ordDoublesBig.NewValueCome(m_pDataSTE[iframe]);
		ordDoublesSmall.NewValueCome(m_pDataSTE[iframe]);
	}
	m_pFeatureSTE[idx] = ordDoublesBig.AverageVal();
	if (m_pFeatureSTE[idx] > 0.0) {
		m_pFeatureSTE[idx] = ordDoublesSmall.AverageVal()/m_pFeatureSTE[idx];
	}
	idx++;

	assert(idx == NUM_STE_FEATURES);
}

#endif	// __USE_STE_FEATURES

// 信号绝对值平均值
void STE(const short xxx[], int NNN, double& ste)
{
	ste = 0.0;
	for (int isamp=0; isamp<NNN; isamp++) {
		ste += abs(xxx[isamp]);
	}
	ste /= NNN;
}

bool IsSilent(const double xxx[], int nNumFrames)
{
	int nzeros = 0;
	int nmaxLeg = 0;
	for (int iframe=0; iframe<nNumFrames; iframe++) {
		if (xxx[iframe] <= MINIMUM_MEAN_ABS_SIGNAL_VALUE) {
			nzeros++;
		} else {
			if (nmaxLeg < nzeros) nmaxLeg = nzeros;
			nzeros = 0;
		}
	}
	if (nmaxLeg < nzeros) nmaxLeg = nzeros;

	if (nmaxLeg >= RATIO_PERCENT_SILENT_LENGTH*nNumFrames) return true;	// 低能段长超过（单元）总长的预定比例
	else return false;
}
