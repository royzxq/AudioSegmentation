// fft_dll.cpp : 定义 DLL 应用程序的入口点。
//

#include "stdafx.h"

#include <stdlib.h>
#include <math.h>

#include "features.h"
#include "fft.h"
#include "..\parameters.h"

#define NUM_DIMENSIONS_CHROMA		13
#define NUM_DIMS_OF_OTHER_FEATURES	8

int GetNumDimsMFCC() { return NumDimsMfccM; }
int NumDimsFrameSpectralFeatures() { return NumDimsMfccM+NUM_DIMENSIONS_CHROMA; }
int NumDimsOtherFeatures() { return NUM_DIMS_OF_OTHER_FEATURES; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 以下是计算 MFCC 的类，。。。。。。


#define NUM_CHROMA_BINS					32	// 特殊的特征计算算法！！！

#define CCC_FACTOR						(1.0E+5)

// 这是已导出类的构造函数。
CCalcMFCC::CCalcMFCC()
{
	return;
}
CCalcMFCC::~CCalcMFCC()
{
	return;
}

void CCalcMFCC::SetParams(int srate, short numbins)
{
	// 计算 MFCC 时，必须知道信号采样频率以及傅里叶频谱分量个数
	m_samplingRate = srate;
	m_nNumBins = numbins;
}

int CCalcMFCC::CalcFrameSpectralFeatures(const cpxv_t *pFSP, double *pdbfeatures)
{
	CalcMFCC(pFSP, pdbfeatures, NumDimsMfccM, NULL);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
	double *XX = new double[NUM_CHROMA_BINS+1];

	double ff;
	int mmm, kkk;
	int ggg;	// 为了省时间！
	for (mmm=1; mmm<=NUM_CHROMA_BINS; mmm++) {
		XX[mmm] = 0.0;
		ggg = 0;
		for (kkk=(mmm-1)*2+1; kkk<NUM_CHROMA_BINS*2; ) {// 高于采样频率二分之一的频谱分量不予考虑
			if (ggg == 0) {
				ff = (double)kkk/m_nNumBins*m_samplingRate;	// 分量对应的频率
				if (ff >= 16.352) {// C0
					XX[mmm] += sqrt(pFSP[kkk].re*pFSP[kkk].re+pFSP[kkk].im*pFSP[kkk].im);
					ggg = 1;
				}
			} else {
				XX[mmm] += sqrt(pFSP[kkk].re*pFSP[kkk].re+pFSP[kkk].im*pFSP[kkk].im);
			}
			kkk *= 2;
		}
	}
	int nnn = 2;
	mmm = 1;	// 第一个元素（下标为 0）内存浪费了
	int sss = nnn;
	for (kkk=NUM_CHROMA_BINS*2; kkk<m_nNumBins/2; kkk++) {
		XX[mmm] += sqrt(pFSP[kkk].re*pFSP[kkk].re+pFSP[kkk].im*pFSP[kkk].im);
		sss--;
		if (sss == 0) {
			mmm++;
			sss = nnn;
		}
		if (mmm > NUM_CHROMA_BINS) {
			nnn *= 2;
			mmm = 1;
			sss = nnn;
		}
	}
	for (mmm=1; mmm<=NUM_CHROMA_BINS; mmm++) {
		XX[mmm] = log(1.0+CCC_FACTOR*XX[mmm]);
	}

// 离散余弦变换
	for (kkk=1; kkk<=NUM_DIMENSIONS_CHROMA; kkk++) {
		pdbfeatures[NumDimsMfccM+kkk-1] = 0.0;
		for (mmm=1; mmm<=NUM_CHROMA_BINS; mmm++) {
			pdbfeatures[NumDimsMfccM+kkk-1] += XX[mmm]*cos((double)kkk*MY_PI/NUM_CHROMA_BINS*((double)mmm-0.5));
		}
	}

	//////////////////////////////////////////////////////////////////////////////

	delete []XX;

	// 返回提取的特征个数
	return NumDimsMfccM+NUM_DIMENSIONS_CHROMA;
}

/*
int LPC(const short xxx[], int nNumSamples, double LpcCoefficients[]);*/

// 返回应该提取的特征个数（即使提取不成功！）
// 
int CalcTempFeatures(const short *pshSamps, int nNumSamps, double *pdbout)
{
	int iDim = 0;	// 提取的特征个数

	double dbRMS = 0.0;
	int nNumCrossings = 0;
	short smap_pre = 0;
	for (int ii=0; ii<nNumSamps; ii++) {
		dbRMS += (double)pshSamps[ii]*pshSamps[ii];
		// 过零率
		if (smap_pre == 0) {
			if (pshSamps[ii] != 0) nNumCrossings++;
		} else if (smap_pre > 0) {
			if (pshSamps[ii] < 0) nNumCrossings++;
		} else {// smap_pre < 0
			if (pshSamps[ii] > 0) nNumCrossings++;
		}
		smap_pre = pshSamps[ii];
	}
	if (nNumSamps > 0) {
		dbRMS /= nNumSamps;
	}
	pdbout[iDim] = sqrt(dbRMS);
	iDim++;
//	if (fwrite(&, sizeof(double), 1, pfout) != 1) { }	// 短时能量

	dbRMS = nNumCrossings;
	if (nNumSamps > 0) {
		dbRMS /= nNumSamps;
	}
	pdbout[iDim] = dbRMS;
	iDim++;
//	if (fwrite(&, sizeof(double), 1, pfout) != 1) { }	// 过率

//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
/*	// LPC coefficients
	iDim += LPC(pshSamps, nNumSamps, pdbout+iDim);*/

	return iDim;
}

//int CalcSpectralFeatures02(const cpxv_t *pFT, int nNumBins, double *pdbout);

// 返回应该提取的特征个数（即使提取不成功！）
// 
int CalcSpectralFeatures(const cpxv_t *pFT, int nNumBins, double *pdbout)
{
	int iDim = 0;

	// (11111) 质心位置
	double dbsum = 0.0;
	double centroid = 0.0;
	for (int iBin=0; iBin<nNumBins/2; iBin++) {
		centroid += (iBin+1)*sqrt(pFT[iBin].re*pFT[iBin].re+pFT[iBin].im*pFT[iBin].im);
		dbsum += sqrt(pFT[iBin].re*pFT[iBin].re+pFT[iBin].im*pFT[iBin].im);
	}
	if (dbsum > 0.0) {
		centroid = centroid/dbsum-1.0;
	}
	pdbout[iDim] = centroid;
	iDim++;
//	if (fwrite(&, sizeof(double), 1, pfout) != 1) { }	// 质心位置

	// (22222)
	double dbsquaredsigma = 0.0;
	for (int iBin=0; iBin<nNumBins/2; iBin++) {
		dbsquaredsigma += (iBin-centroid)*(iBin-centroid)*sqrt(pFT[iBin].re*pFT[iBin].re+pFT[iBin].im*pFT[iBin].im);
	}
	if (dbsum > 0.0) {
		dbsquaredsigma /=dbsum;
	}
	pdbout[iDim] = dbsquaredsigma;
	iDim++;
//	if (fwrite(&, sizeof(double), 1, pfout) != 1) { }	// 延展性

	// (33333)
	double mm = 0.0;	// m3
	for (int iBin=0; iBin<nNumBins/2; iBin++) {
		mm += (iBin-centroid)*(iBin-centroid)*(iBin-centroid)*
						sqrt(pFT[iBin].re*pFT[iBin].re+pFT[iBin].im*pFT[iBin].im);
	}
	if (dbsum > 0.0) {
		mm /=dbsum;
	}
	if (dbsquaredsigma > 0.0) {
		mm /= dbsquaredsigma*sqrt(dbsquaredsigma);	// sigma to the power of 3
	}
	pdbout[iDim] = mm;
	iDim++;
//	if (fwrite(&, sizeof(double), 1, pfout) != 1) { }	// m3

	// (44444)
	mm = 0.0;	// m4
	for (int iBin=0; iBin<nNumBins/2; iBin++) {
		mm += (iBin-centroid)*(iBin-centroid)*(iBin-centroid)*(iBin-centroid)*
						sqrt(pFT[iBin].re*pFT[iBin].re+pFT[iBin].im*pFT[iBin].im);
	}
	if (dbsum > 0.0) {
		mm /=dbsum;
	}
	if (dbsquaredsigma > 0.0) {
		mm /= dbsquaredsigma*dbsquaredsigma;	// sigma to the 4th power
	}
	pdbout[iDim] = mm;
	iDim++;
//	if (fwrite(&, sizeof(double), 1, pfout) != 1) { }	// m4

	// (55555) spectral decrease
	dbsum = 0.0;
	mm = 0.0;
	centroid = sqrt(pFT[1].re*pFT[1].re+pFT[1].im*pFT[1].im);	// amplitude of the 2nd bin
	for (int iBin=2; iBin<nNumBins/2; iBin++) {// from the 3rd bin, ...
		dbsum += sqrt(pFT[iBin].re*pFT[iBin].re+pFT[iBin].im*pFT[iBin].im);
		mm += (sqrt(pFT[iBin].re*pFT[iBin].re+pFT[iBin].im*pFT[iBin].im)-centroid)/(iBin-1);
	}
	if (dbsum > 0.0) {
		mm /= dbsum;
	}
	pdbout[iDim] = mm;
	iDim++;
//	if (fwrite(&, sizeof(double), 1, pfout) != 1) { }	// 

	// (66666) spectral roll-off
	dbsum = 0.0;
	for (int iBin=0; iBin<nNumBins/2; iBin++) {// 一半
		// sum of squared amplitudes of half the bins
		dbsum += pFT[iBin].re*pFT[iBin].re+pFT[iBin].im*pFT[iBin].im;
	}
	mm = 0.0;
	for (int iBin=0; iBin<nNumBins/2; iBin++) {
		mm += pFT[iBin].re*pFT[iBin].re+pFT[iBin].im*pFT[iBin].im;	// summing, 
		if (mm > 0.95*dbsum) {// , until the sum exceeds 95% of the total
			mm = iBin;
			break;
		}
	}
	pdbout[iDim] = mm;
	iDim++;
//	if (fwrite(&, sizeof(double), 1, pfout) != 1) { }	// 

//	iDim += CalcSpectralFeatures02(pFT, nNumBins, pdbout+iDim);

	return iDim;
}

/*
void InsertElement(const double *pdbbuf, short iBin, short *pIdxPeak, short& nNumElements);
void SortList(short *pIdxPeak, short nNumElements);

//#define sin_quarter_pi	0.78539816339744830961566084581988
#define NUM_ELEMENTS_PEAK	20

// 返回应该提取的特征个数（即使提取不成功！）
// 
int CalcSpectralFeatures02(const cpxv_t *pFT, int nNumBins, double *pdbout)
{
	double *pdbbuf = new double[nNumBins/2];
	if (pdbbuf == NULL) {
		return -1;
	}

	short iDim = 0;

// 求幅值谱，。。。
	for (int ii=0; ii<nNumBins/2; ii++) {
		pdbbuf[ii] = sqrt(pFT[ii].re*pFT[ii].re+pFT[ii].im*pFT[ii].im);
	}

// 对幅值谱进行滤波，为了抑制谱中的高频成分，。。。
	double dbtmp01, dbtmp02;
	dbtmp01 = pdbbuf[0];
//	pdbbuf[0] = (pdbbuf[0]+pdbbuf[1]*sin_quarter_pi)/(1.0+sin_quarter_pi);
	pdbbuf[0] = (pdbbuf[0]+pdbbuf[1])/2.0;
	for (int ii=1; ii<nNumBins/2-1; ii++) {
		dbtmp02 = pdbbuf[ii];
//		pdbbuf[ii] = (dbtmp01*sin_quarter_pi+pdbbuf[ii]+pdbbuf[ii+1]*sin_quarter_pi)/(sin_quarter_pi+1.0+sin_quarter_pi);
		pdbbuf[ii] = (dbtmp01+pdbbuf[ii]+pdbbuf[ii+1])/3.0;
		dbtmp01 = dbtmp02;
	}
//	pdbbuf[nNumBins/2-1] = (dbtmp01*sin_quarter_pi+pdbbuf[nNumBins/2-1])/(sin_quarter_pi+1.0);
	pdbbuf[nNumBins/2-1] = (dbtmp01+pdbbuf[nNumBins/2-1])/2.0;

	//////////////////////////////////////////////////////////////////////////////////////////

	short nNumElementsPeak = 0;
	short *pIdxPeak = new short[NUM_ELEMENTS_PEAK];
	memset(pIdxPeak, 0, sizeof(short)*NUM_ELEMENTS_PEAK);

	dbtmp01 = 0.0;
	int nNumPeaks = 0;
	int iBin = -1;
	for (int ii=1; ii<nNumBins/2-1; ii++) {
		if (pdbbuf[ii-1] <= pdbbuf[ii] && pdbbuf[ii] > pdbbuf[ii+1]) {// 遇到峰值，。。。
			InsertElement(pdbbuf, ii, pIdxPeak, nNumElementsPeak);	// 峰点按峰值从大到小排序

			dbtmp01 += pdbbuf[ii];
			nNumPeaks++;
			if (iBin == -1) {
				pdbout[iDim] = ii;	// 频率最低的（最左边的）峰值对应的频率
				iDim++;
			}
			iBin = ii;
		}
	}
	SortList(pIdxPeak, nNumElementsPeak);	// 峰点（DFT bin number）按频率从大到小排序

	if (nNumPeaks > 1) {
		dbtmp01 /= nNumPeaks;
	}
	pdbout[iDim] = nNumPeaks;	// 峰值个数
	iDim++;
	pdbout[iDim] = iBin;	// 频率最高的（最右边的）峰值对应的频率
	iDim++;

	dbtmp02 = 0.0;
	for (int ii=1; ii<nNumBins/2-1; ii++) {
		if (pdbbuf[ii-1] <= pdbbuf[ii] && pdbbuf[ii] > pdbbuf[ii+1]) {// 遇到峰值，。。。
			dbtmp02 += (dbtmp01-pdbbuf[ii])*(dbtmp01-pdbbuf[ii]);
		}
	}
	if (nNumPeaks > 2) {
		dbtmp02 = sqrt(dbtmp02/(nNumPeaks-1));
	}
	pdbout[iDim] = log10(1.0E-10+dbtmp01);	// 峰值均值
	iDim++;
	pdbout[iDim] = log10(1.0E-10+dbtmp02);	// 峰值方差
	iDim++;

	delete []pIdxPeak;
	delete []pdbbuf;

	return 5;
}

// "nNumElements" number of elements currently in the array which hold the DFT bin numbers of peaks
// bin number 按对应的峰值从大到小排序，。。。
// 
void InsertElement(const double *pdbbuf, short iBin, short BinIdxPeakAry[], short& nNumElements)
{
	short ii, kk;
	for (ii=0; ii<nNumElements; ii++) {
		if (pdbbuf[iBin] > pdbbuf[BinIdxPeakAry[ii]]) {
			break;
		}
	}
	kk = nNumElements-1;
	if (nNumElements == NUM_ELEMENTS_PEAK) {// 已满，。。。
		kk--;
	}
	for (; kk>=ii; kk--) {
		BinIdxPeakAry[kk+1] = BinIdxPeakAry[kk];
	}
	if (ii < NUM_ELEMENTS_PEAK) {
		BinIdxPeakAry[ii] = iBin;
	}
	if (nNumElements < NUM_ELEMENTS_PEAK) {
		nNumElements++;
	}
}

// 按数组元素（峰值对应的 DFT bin number）从小到大排序，。。。
// 
void SortList(short IdxBinPeakAry[], short nNumElements)
{
	short ii, kk, tmp;
	for (ii=0; ii<nNumElements; ii++) {
		for (kk=ii+1; kk<nNumElements; kk++) {
			if (IdxBinPeakAry[ii] < IdxBinPeakAry[kk]) {
				continue;
			}
			tmp = IdxBinPeakAry[ii];
			IdxBinPeakAry[ii] = IdxBinPeakAry[kk];
			IdxBinPeakAry[kk] = tmp;
		}
	}
}

void InsertElement(double SpectrumAry[], short nNumBins, const short BinIdxPeakAry[], short nNumElements, double OutAry[])
{
	double dbsum;
	short ibin, size, dd;
	for (short ii=0; ii<nNumElements; ii++) {
		dbsum = 0.0;
		ibin = size = BinIdxPeakAry[ii];
		while (ibin < nNumBins/2) {
			dbsum += SpectrumAry[ibin];
			SpectrumAry[ibin] = 0.0;
			ibin += size;
		}
		dd = 2;
		while ((ibin = size/dd) > 0) {
			dbsum += SpectrumAry[ibin];
			SpectrumAry[ibin] = 0.0;
			dd++;
		}

	}
}

*/

