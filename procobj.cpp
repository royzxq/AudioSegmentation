

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
#include <TCHAR.h>

#include "parameters.h"

#include "procobj.h"
#include "fft\fft.h"

CProcessingObj::CProcessingObj()
{
	m_nNumValidFiles = 0;
//
	m_pfWaveform = new CWFFile;
	if (m_pfWaveform == NULL) {
		printf("m_pfWaveform == NULL !\n");
	}
/*	m_nNumTmpVectors = NumTmpVectors();
	m_pNumFrames = (int*)malloc(sizeof(int)*m_nNumTmpVectors);
	memset(m_pNumFrames, 0, sizeof(int)*m_nNumTmpVectors);*/
//
	m_fdom = Malloc(cpxv_t, NumBinsInFftWinM);
	if (m_fdom == NULL) {
		printf("m_fdom == NULL !\n");
	}

// AM 源数据（若干帧，每帧 "NUM_DIMS_AM_SORCE_DATA" 个值 --- 每个频带一个值）缓存
	m_pBandSum = Malloc(BandSumArray_t, NumFrameSkipsInUnitM);	// *NumFramesAreaMomentsSrcM
	if (m_pBandSum == NULL) {
		printf("m_pBandSum == NULL !\n");
	}
	m_pSrcAM = Malloc(MagSpectArray_t, NumFramesAreaMomentsSrcM);
	if (m_pSrcAM == NULL) {
		printf("m_pSrcAM == NULL !\n");
	}

// 特征向量序列（一个单元中的各个特征向量序列）
	m_pdbMFCC = Malloc(double, NumDimsMfccM*NumFrameSkipsInUnitM);
	m_pdbAM = Malloc(double, NumDimsAreaMomentsM*NumFrameSkipsInUnitM);
	m_pLPC = Malloc(double, NUM_DIMS_LPC*NumFrameSkipsInUnitM);
	m_pDataSTE = Malloc(double, NumFrameSkipsInUnitM);	// 短时（帧）信号值绝对值之平均值序列（对应一个单元 unit）
	if (m_pdbMFCC == NULL || m_pdbAM == NULL || m_pLPC == NULL || m_pDataSTE == NULL) {
		printf("m_pdbMFCC == NULL || ... || m_pDataSTE == NULL !\n");
	}
//
	m_pMeanMFCC = m_pStdMFCC = m_pMeanDeriMFCC = m_pStdDeriMFCC = NULL;
	m_pMeanAM = m_pStdAM = m_pMeanDeriAM = m_pStdDeriAM = NULL;
	m_pMeanLPC = m_pStdLPC = m_pMeanDeriLPC = m_pStdDeriLPC = NULL;

#ifdef __USE_STE_FEATURES
	m_pFeatureSTE = NULL;
#endif

#ifdef __USE_SPECTRAL_SHAPE_FEATURES
/*	m_ftPre = Malloc(double, NumBinsInFftWinM);
	if (m_ftPre == NULL) {
		printf("m_ftPre == NULL !\n");
	}*/
	m_pSpectralShape = Malloc(double, NUM_DIMS_SPECTRAL_SHAPE*NumFrameSkipsInUnitM);
	if (m_pSpectralShape == NULL) {
		printf("m_pSpectralShape == NULL !\n");
	}
	m_pMeanSpectralShape = m_pStdSpectralShape = NULL;
#endif	// #ifdef __USE_SPECTRAL_SHAPE_FEATURES

#ifdef __USE_BEAT_HISTOGRAM
	m_pFeatureBH = NULL;
#endif	// __USE_BEAT_HISTOGRAM

#ifdef __USE_ZERO_CROSSING
	m_pZC = Malloc(double, NumFrameSkipsInUnitM);
	if (m_pZC == NULL) {
		printf("m_pZC == NULL !\n");
	}
	m_pFeatureZC = NULL;
#endif	// __USE_ZERO_CROSSING
#ifdef __USE_SUB_BAND_ENERGY
	m_pFeatureSBE = NULL;
#endif	// __USE_SUB_BAND_ENERGY

	m_nNumDims = 0;
// 样本特征总向量
	m_nNumDims += NumDimsMfccM*2;
	m_nNumDims += NumDimsAreaMomentsM*2;
	m_nNumDims += NUM_DIMS_LPC*2;
#ifdef __USE_STE_FEATURES
	m_nNumDims += NUM_STE_FEATURES;
#endif
#ifdef __USE_SPECTRAL_SHAPE_FEATURES
	m_nNumDims += NUM_DIMS_SPECTRAL_SHAPE;	// 均值和
	m_nNumDims += NUM_DIMS_SPECTRAL_SHAPE;	// 方差
#endif
#ifdef __USE_BEAT_HISTOGRAM
	m_nNumDims += NUM_DIMS_BH;
#endif	// __USE_BEAT_HISTOGRAM
#ifdef __USE_MODULATION_SPECTRUM
	m_nNumDims += NUM_DIMS_MSP;
#endif	// __USE_MODULATION_SPECTRUM
#ifdef __USE_ZERO_CROSSING
	m_nNumDims += NUM_DIMS_ZC;
#endif
#ifdef __USE_SUB_BAND_ENERGY
	m_nNumDims += NUM_DIMS_SBE;
#endif
	m_pvector = Malloc(double, m_nNumDims);
	if (m_pvector == NULL) {
		printf("m_pvector == NULL !\n");
	}
	if (m_pvector) {
		double* ptmp = m_pvector;

		m_pMeanMFCC = ptmp; ptmp += NumDimsMfccM;
		m_pStdMFCC = ptmp; ptmp += NumDimsMfccM;
/*		m_pMeanDeriMFCC = ptmp; ptmp += NumDimsMfccM;
		m_pStdDeriMFCC = ptmp; ptmp += NumDimsMfccM;*/
//
		m_pMeanAM = ptmp; ptmp += NumDimsAreaMomentsM;
		m_pStdAM = ptmp; ptmp += NumDimsAreaMomentsM;
/*		m_pMeanDeriAM = ptmp; ptmp += NumDimsAreaMomentsM;
		m_pStdDeriAM = ptmp; ptmp += NumDimsAreaMomentsM;*/
//
		m_pMeanLPC = ptmp; ptmp += NUM_DIMS_LPC;
		m_pStdLPC = ptmp; ptmp += NUM_DIMS_LPC;
/*		m_pMeanDeriLPC = ptmp; ptmp += NUM_DIMS_LPC;
		m_pStdDeriLPC = ptmp; ptmp += NUM_DIMS_LPC;*/

#ifdef __USE_STE_FEATURES
		m_pFeatureSTE = ptmp; ptmp += NUM_STE_FEATURES;
#endif	// __USE_STE_FEATURES
#ifdef __USE_SPECTRAL_SHAPE_FEATURES
		m_pMeanSpectralShape = ptmp; ptmp += NUM_DIMS_SPECTRAL_SHAPE;	// 均值
		m_pStdSpectralShape = ptmp; ptmp += NUM_DIMS_SPECTRAL_SHAPE;	// 方差
#endif	// __USE_SPECTRAL_SHAPE_FEATURES
#ifdef __USE_BEAT_HISTOGRAM
		m_pFeatureBH = ptmp; ptmp += NUM_DIMS_BH;
#endif	// __USE_BEAT_HISTOGRAM
#ifdef __USE_MODULATION_SPECTRUM
		m_pFeatureMSP = ptmp; ptmp += NUM_DIMS_MSP;
#endif	// __USE_MODULATION_SPECTRUM
#ifdef __USE_ZERO_CROSSING
		m_pFeatureZC = ptmp; ptmp += NUM_DIMS_ZC;
#endif
#ifdef __USE_SUB_BAND_ENERGY
		m_pFeatureSBE = ptmp; ptmp += NUM_DIMS_SBE;
#endif
	}

#ifndef __TRAINING_PHASE
//	m_pClassNo = Malloc(unsigned char, MAX_NUM_UNITS);	// 存放各个单元的类别号
	m_pWavInterface = NULL;	// 必须必！

	m_pfSTE = NULL;	// 存放待分段音频的帧能量值的文件
#endif	// #ifndef __TRAINING_PHASE
}

CProcessingObj::~CProcessingObj()
{
	ReleaseMemory();
/*	if (m_pNumFrames)
		free(m_pNumFrames);*/

#ifndef __TRAINING_PHASE
//	if (m_pClassNo) free(m_pClassNo);
	if (m_pWavInterface) delete m_pWavInterface;
#endif	// #ifndef __TRAINING_PHASE
}

// 释放计算特征所需要的内存
void CProcessingObj::ReleaseMemory()
{
// 111
	// wave form 文件处理
	if (m_pfWaveform) { delete m_pfWaveform; m_pfWaveform = 0; }
// 222
	// 一帧的 FFT 结果
	if (m_fdom) { free(m_fdom); m_fdom = 0; }
// 333
	// 帧 Mel Bands 向量队列
	if (m_pBandSum) { free(m_pBandSum); m_pBandSum = 0; }
	if (m_pSrcAM) { free(m_pSrcAM); m_pSrcAM = NULL; }
// 444
	// 帧 MFCC 特征向量队列
	if (m_pdbMFCC) { free(m_pdbMFCC); m_pdbMFCC = 0; }
// 555
	// 帧 AM 特征向量队列
	if (m_pdbAM) { free(m_pdbAM); m_pdbAM = 0; }
// 666
	// 帧 LPC 特征向量队列
	if (m_pLPC) { free(m_pLPC); m_pLPC = 0; }

// 777
	// 帧能量值队列
	if (m_pDataSTE) { free(m_pDataSTE); m_pDataSTE = 0; }
// 888
#ifdef __USE_SPECTRAL_SHAPE_FEATURES
//	if (m_ftPre) { free(m_ftPre); m_ftPre = 0; }
	// 帧音色特征向量队列
	if (m_pSpectralShape) { free(m_pSpectralShape); m_pSpectralShape = 0; }
#endif
#ifdef __USE_ZERO_CROSSING
	if (m_pZC) { free(m_pZC); m_pZC = 0; }
#endif	// __USE_ZERO_CROSSING
// aaa

	// 最终特征向量
	if (m_pvector) { free(m_pvector); m_pvector = 0; }

}

// 当前单元处理完后，都要跳一步（到下一单元）。此函数用于处理相邻单元间的重叠部分。
void CProcessingObj::MoveUnitOverlap()
{
	if (NumFrameSkipsInUnitSkipM >= NumFrameSkipsInUnitM) {
		m_nNumFrames = 0;
		return;
	}

// （一）特征向量序列，。。。

	// MFCC 特征向量队列，。。。
	memmove(m_pdbMFCC, m_pdbMFCC+NumDimsMfccM*NumFrameSkipsInUnitSkipM, 
				sizeof(double)*NumDimsMfccM*(NumFrameSkipsInUnitM-NumFrameSkipsInUnitSkipM));
	// AM 特征向量队列，。。。
	memmove(m_pdbAM, m_pdbAM+NumDimsAreaMomentsM*NumFrameSkipsInUnitSkipM, 
				sizeof(double)*NumDimsAreaMomentsM*(NumFrameSkipsInUnitM-NumFrameSkipsInUnitSkipM));
	// LPC 特征向量队列，。。。
	memmove(m_pLPC, m_pLPC+NUM_DIMS_LPC*NumFrameSkipsInUnitSkipM,
				sizeof(double)*NUM_DIMS_LPC*(NumFrameSkipsInUnitM-NumFrameSkipsInUnitSkipM));

#ifdef __USE_SPECTRAL_SHAPE_FEATURES
	memmove(m_pSpectralShape, m_pSpectralShape+NUM_DIMS_SPECTRAL_SHAPE*NumFrameSkipsInUnitSkipM,
				sizeof(double)*NUM_DIMS_SPECTRAL_SHAPE*(NumFrameSkipsInUnitM-NumFrameSkipsInUnitSkipM));
#endif	//#ifdef __USE_SPECTRAL_SHAPE_FEATURES
#ifdef __USE_ZERO_CROSSING
	// 每帧一个值（1 维）
	memmove(m_pZC, m_pZC+NumFrameSkipsInUnitSkipM,
				sizeof(double)*(NumFrameSkipsInUnitM-NumFrameSkipsInUnitSkipM));
#endif	// __USE_ZERO_CROSSING

// （二）中间数据，。。。

	// 每帧 1 维
	memmove(m_pDataSTE, m_pDataSTE+NumFrameSkipsInUnitSkipM,
			sizeof(double)*(NumFrameSkipsInUnitM-NumFrameSkipsInUnitSkipM));
	// 帧 Mel Bands 向量队列
	memmove(m_pBandSum[0], m_pBandSum[NumFrameSkipsInUnitSkipM],
			sizeof(double)*(NumFrameSkipsInUnitM-NumFrameSkipsInUnitSkipM)*NUM_DIMS_AM_SORCE_DATA);

// （三）

	m_nNumFrames = NumFrameSkipsInUnitM-NumFrameSkipsInUnitSkipM;
}















/*
// 根据单元长（帧数）和单元跳步（帧数）大小确定需要几个临时帧向量
int CProcessingObj::NumTmpVectors()
{
	int nNumTmpVectors = 1;
	int nstart = 0;
	while (nstart+NumFrameSkipsInUnitSkipM < NumFrameSkipsInUnitM) {
		nNumTmpVectors++;
		nstart += NumFrameSkipsInUnitSkipM;
	}
	return nNumTmpVectors;
}

int CProcessingObj::NumTmpVectorsToCompute(int iframe)
{
	int nTmpVecotrs = 1;
	int nstart = 0;
	while (nstart+NumFrameSkipsInUnitSkipM <= iframe) {
		nTmpVecotrs++;
		nstart += NumFrameSkipsInUnitSkipM;
	}
	return nTmpVecotrs;
}
*/

