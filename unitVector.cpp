

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

void LPC(const short xxx[], int NNN, double aaa_out[], int ppp);
void STE(const short xxx[], int NNN, double& ste);
void DCT2D(const double ff[], int HH, int WW, double dct[], int HHO, int WWO);

void SpectralShape(const cpxv_t fdom[], double pSpectralShape[]);
void MakeFeatureVectorSpectralShape();
double ZeroCrossing(const short xxx[], int nNumSamples);

// 另一种计算 MFCC 的方法，结果不咋地
int CalculateMFCC(const short *pSamps, int frameSize, double *pMfcc, int ceporder, int fftWinSize);

int CalcMfccFromMagnitudeSpectrum(const cpxv_t* fdom, double cepc_out[], int numCepstraOut);

void CProcessingObj::ProcSingleFrame(const short psamps[])
{
	// 当前帧做傅立叶变换
	DoFFT(psamps, m_fdom);
	PrepareSrcAM();

// "m_nNumFrames" 当前帧的下标！！！

/*
	// 另一种计算 MFCC 的方法，结果不咋地
	CalculateMFCC(psamps, NumSamplesInFrameM, m_pdbMFCC+NumDimsMfccM*m_nNumFrames, NumDimsMfccM-1, NumBinsInFftWinM);
*/
//	CalcMfccFromMagnitudeSpectrum(m_fdom, m_pdbMFCC+NumDimsMfccM*m_nNumFrames, NumDimsMfccM);

	// 计算当前帧的 MFCC 特征
	CalcMFCC(m_fdom, 
		// MFCC 特征向量保存地址
		m_pdbMFCC+NumDimsMfccM*m_nNumFrames, NumDimsMfccM,	// NULL, 0,
		// 各个 Mel band 的值存入 AM 源数据缓存的最后一帧
//		m_pBandSum[NumFramesAreaMomentsSrcM-1];
		// 各个 Mel band 的值存入 AM 源数据缓存当前帧
#ifdef __USE_MEL_BAND_VALUE_AS_AM_SOURCE
		m_pBandSum[m_nNumFrames]);	// 用 Mel 频带划分和带值计算法
#else
		NULL);	// 表明要自己划分频带
	// 自己的带值计算方法
	void BandSum(const cpxv_t fdom[], double bandSums[]);
	BandSum(m_fdom, m_pBandSum[m_nNumFrames]);	// 按 7 个八度归集
#endif	// __USE_MEL_BAND_VALUE_AS_AM_SOURCE

	if (m_nNumFrames+1 >= NumFramesAreaMomentsSrcM) {// AM 源数据缓存中的源数据够计算一个 AM 特征向量了，
/*
		AreaOfMoments(m_pBandSum[m_nNumFrames+1-NumFramesAreaMomentsSrcM], NumFramesAreaMomentsSrcM, NUM_DIMS_AM_SORCE_DATA, 
			// AM 特征向量（地址、维数）
			m_pdbAM+NumDimsAreaMomentsM*m_nNumFrames, NumDimsAreaMomentsM);*/
		AreaOfMoments(&m_pSrcAM[0][0], NumFramesAreaMomentsSrcM, WIDTH_SOURCE_DATA_AM,
			m_pdbAM+NumDimsAreaMomentsM*m_nNumFrames, NumDimsAreaMomentsM);
/*		// 基于 MFCC 求 AM，效果没改善！
		AreaOfMoments(m_pdbMFCC+(m_nNumFrames+1-NumFramesAreaMomentsSrcM)*NumDimsMfccM,
			NumFramesAreaMomentsSrcM, NumDimsMfccM, 
			m_pdbAM+NumDimsAreaMomentsM*m_nNumFrames, NumDimsAreaMomentsM);*/
/*		DCT2D(m_pBandSum[m_nNumFrames+1-NumFramesAreaMomentsSrcM], NumFramesAreaMomentsSrcM, NUM_DIMS_AM_SORCE_DATA,  
			m_pdbAM+NumDimsAreaMomentsM*m_nNumFrames, 4, 5);*/
	}

	// 最后 2 个参数：LPC 特征向量存放位置
	LPC(psamps, NumSamplesInFrameM, m_pLPC+NUM_DIMS_LPC*m_nNumFrames, NUM_DIMS_LPC);

#ifdef __USE_SPECTRAL_SHAPE_FEATURES
	SpectralShape(m_fdom, m_pSpectralShape+NUM_DIMS_SPECTRAL_SHAPE*m_nNumFrames);
#endif	// #ifdef __USE_SPECTRAL_SHAPE_FEATURES

	// 短时能量序列最终不是求均值，故窗口不重叠效果好些
	STE(psamps, NumSamplesInFrameSkipM, m_pDataSTE[m_nNumFrames]);	//NumSamplesInFrameM
#ifndef __TRAINING_PHASE
	if (m_pfSTE) {// 当前帧的 STE 值写入临时文件（将来确定段界时用）
		if (fwrite(m_pDataSTE+m_nNumFrames, sizeof(double), 1, m_pfSTE) != 1) {
		}
	}
#endif	// __TRAINING_PHASE

#ifdef __USE_ZERO_CROSSING
	// 每帧算一个值
	m_pZC[m_nNumFrames] = ZeroCrossing(psamps, NumSamplesInFrameSkipM);
#endif	// __USE_ZERO_CROSSING

#ifdef __USE_SPECTRAL_SHAPE_FEATURES
	// 必须放在最后处理！！！
//	CalcPre();
#endif
}

bool IsSilent(const double xxx[], int nNumFrames);

int CProcessingObj::MakeFeatureVector()
{
/*	// "m_pDataSTE[...]" 短时（帧）信号值绝对值之平均值序列（对应一个单元 unit）
	if (IsSilent(m_pDataSTE, m_nNumFrames)) return -1;	// 静音检测*/
	if (m_nNumFrames < NumFramesAreaMomentsSrcM) {
		printf("m_nNumFrames < NumFramesAreaMomentsSrcM !\n");
		return -2;	// 不能计算 AM 特征向量
	}

	// 最终的特征向量
	memset(m_pvector, 0, sizeof(double)*m_nNumDims);

	MakeFeatureVectorMFCC();
	MakeFeatureVectorAM();
	MakeFeatureVectorLPC();

#ifdef __USE_SPECTRAL_SHAPE_FEATURES
	MakeFeatureVectorSpectralShape();
#endif	// #ifdef __USE_SPECTRAL_SHAPE_FEATURES

#ifdef __USE_STE_FEATURES
	MakeFeatureVectorSTE();
#endif	// __USE_STE_FEATURES
#ifdef __USE_BEAT_HISTOGRAM
	MakeFeatureVectorBH();
#endif
#ifdef __USE_MODULATION_SPECTRUM
	MakeFeatureVectorMSP();
#endif
#ifdef __USE_ZERO_CROSSING
	MakeFeatureVectorZC();
#endif	// __USE_ZERO_CROSSING
#ifdef __USE_SUB_BAND_ENERGY
	MakeFeatureVectorSBE();
#endif	// __USE_SUB_BAND_ENERGY

	return 0;	// OK
}

#ifdef __USE_ZERO_CROSSING

void CProcessingObj::MakeFeatureVectorZC()
{
	if (m_pFeatureZC == NULL) return;

	double dbtemp;
	for (int ii=0; ii<m_nNumFrames-1; ii++) {
		for (int mm=ii+1; mm<m_nNumFrames; mm++) {
			if (m_pZC[ii] <= m_pZC[mm]) continue;
			dbtemp = m_pZC[ii];
			m_pZC[ii] = m_pZC[mm];
			m_pZC[mm] = dbtemp;
		}
	}
	dbtemp = m_pZC[m_nNumFrames-3]+m_pZC[m_nNumFrames-2]+m_pZC[m_nNumFrames-1];
	m_pFeatureZC[0] = (m_pZC[0]+m_pZC[1]+m_pZC[2])/dbtemp;

	return;
}

#endif	// __USE_ZERO_CROSSING
