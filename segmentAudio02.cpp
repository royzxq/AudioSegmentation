

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

#include "ErrorNumber.h"

#include "mydefs.h"
#include "procobj.h"
#include "fft\fft.h"

#ifndef __TRAINING_PHASE

extern FILE* g_pfwInfo;

#include <algorithm>
// ZONE_INFO_Ts will be sorted by ...
bool operator<(const ZONE_INFO_T& xx, const ZONE_INFO_T& yy)
{
	return xx.iRR-xx.iLL > yy.iRR-yy.iLL;
}

void DoSegmentation(const ZONE_VECTOR& zoneVector, int nNumFramesTotal,	std::vector<int>& segEndVector);

#define NUM_FRAME_SKIPS_BUFFER_CAN_HOLD	1024	// 1024 帧

#define CCC_000		2.0

// 帧跳个数
#define CCC_009		15

// 1.0 ～
#define CCC_010		5.0		// 约合 6.0 秒

// 对音频分段，给出各个段的末尾帧下标，并返回最后剩余的不满一个帧跳的采样个数
int CProcessingObj::SegmentAudio02(int fhr, std::vector<int>& segEndVector)
{
	// 最后剩余的不满一个帧跳的采样个数
	int nNumSamplesLeft = SegmentAudioHelper02(fhr);
	if (nNumSamplesLeft < 0) {
		return -1;
	}
//
	sprintf(m_szfn, "%s%s", m_psOutPath, FN_TEMP_STE);
	FILE *pfrSTE = fopen(m_szfn, "rb");	// 打开文件，为了读数据，。。。

	// 注意：这里的帧数，是完整帧的个数。
	int nNumFramesTotal;
	if (fread(&nNumFramesTotal, sizeof(int), 1, pfrSTE) != 1) {
	}

	ZONE_VECTOR zoneVector;
	ZONE_INFO_T zoneInfo;

	size_t nBufSizeInDBs = sizeof(short)*NumSamplesInFrameSkipM*NUM_FRAME_SKIPS_BUFFER_CAN_HOLD/sizeof(double);
	double *pdbSTE = Malloc(double, nBufSizeInDBs);
	int iFrm_G = 0;	// 帧（全局）下标
	int iLL = -1, iRR = -2;	// 候选切分区的左界和右界帧下标
	size_t nSTEsRead;
	while (1) {
		nSTEsRead = fread(pdbSTE, sizeof(double), nBufSizeInDBs, pfrSTE);
		if (nSTEsRead == 0) break;

		for (int ii = 0; ii<nSTEsRead; ii++, iFrm_G++) {// 注意 "iFrm_G++" ！
			if (pdbSTE[ii] <= CCC_000*MINIMUM_MEAN_ABS_SIGNAL_VALUE) {// 可能的切分点，。。。
				if (iLL < 0) {// 候选切分区左界，一旦确定，不可更改！
					iLL = iFrm_G; 
				}
				iRR = iFrm_G;	// 重置右界
				continue;
			}			
//
			if ((iRR-iLL+1) >= CCC_009) {// 候选切分区足够长
				assert(iRR >= 0);
				zoneInfo.iLL = iLL;
				zoneInfo.iRR = iRR;
				zoneVector.push_back(zoneInfo);
			}
			if (iLL >= 0) {
				iLL = -1; iRR = -2;
			}
		}
	}
	assert(iFrm_G == nNumFramesTotal);

// 可见，音频首部的候选切分区被考虑了，而音频尾部的候选切分区被忽略了。

	// 将所有候选切分区按长度从大到小排序，以便随后按顺序处理之
    sort(zoneVector.begin(), zoneVector.end());

//
////////////////////////////////////////////////////////////////////////////////////////////
//
	// 对音频分大段，输出各个大段的末尾帧下标
	DoSegmentation(zoneVector, nNumFramesTotal, segEndVector);

	// 将大段按其末尾帧下标之自然顺序排序
    sort(segEndVector.begin(), segEndVector.end());
//
	free(pdbSTE);
	// 关闭并删除临时文件，。。。
	fclose(pfrSTE);
	sprintf(m_szfn, "%s%s", m_psOutPath, FN_TEMP_STE);
	remove(m_szfn);
//
	if (g_pfwInfo) {
		fprintf(g_pfwInfo, "\n大段信息（共 %d 个大段）。。。\n", segEndVector.size());
		int& nNumSampsTotal = iLL;
		nNumSampsTotal = segEndVector.back()*NumSamplesInFrameSkipM+NumSamplesInFrameM;	// 音频总帧数
		nNumSampsTotal += nNumSamplesLeft;

		int& iSample = iFrm_G;
		int iPreEnd = -1;	// 采样下标
		for (std::vector<int>::size_type iSeg = 0; iSeg < segEndVector.size(); iSeg++) {
			ldiv_t lDivR;

			// 大段结束位置（采样下标对应的时间）（对末大段，忽略残留的不满一帧跳的采样）
			iSample = segEndVector[iSeg]*NumSamplesInFrameSkipM+NumSamplesInFrameM;
			if (iSeg == segEndVector.size()-1) {
				iSample += nNumSamplesLeft;	// 最后一个大段加上残值
			} else {
				iSample -= NumSamplesInFrameM-NumSamplesInFrameSkipM;
			}
			lDivR.rem = 1000.0*(iSample-1)/UNIFIED_SAMPLING_RATE;	// ms

			lDivR = ldiv(lDivR.rem, 60*60*1000);	// 时
			long hhh = lDivR.quot;
			lDivR = ldiv(lDivR.rem, 60*1000);	// 分
			long mmm = lDivR.quot;
			lDivR = ldiv(lDivR.rem, 1000);	// 秒
			fprintf(g_pfwInfo, "%05d - %02d:%02d:%02d:%03d(", iSeg+1, hhh, mmm, lDivR.quot, lDivR.rem);
					
			// 求段长（毫秒数）
			lDivR.rem = 1000.0*(iSample-iPreEnd)/UNIFIED_SAMPLING_RATE;	// ms
			//
			lDivR = ldiv(lDivR.rem, 60*60*1000);	// 时
			hhh = lDivR.quot;
			lDivR = ldiv(lDivR.rem, 60*1000);	// 分
			mmm = lDivR.quot;
			lDivR = ldiv(lDivR.rem, 1000);	// 秒
			fprintf(g_pfwInfo, "%02d:%02d:%02d:%03d) ", hhh, mmm, lDivR.quot, lDivR.rem);

			for (int mm=0; mm<500.0*(iSample-iPreEnd)/nNumSampsTotal; mm++) {
				fprintf(g_pfwInfo, "*");
			}
			fprintf(g_pfwInfo, "\n");

			iPreEnd = iSample;	// 段末采样下标
		}	// end of "for (...) {"
		fflush(g_pfwInfo);
	}

	// 最后剩余的不满一个帧跳的采样个数
	return nNumSamplesLeft;
}

void DoSegmentation(const ZONE_VECTOR& zoneVector, int nNumFramesTotal, std::vector<int>& segEndFrmVector)
{
	// 临时分段信息文件，。。。
	ZONE_VECTOR tmpSegVector;
	ZONE_INFO_T tmpSegInfo;
	tmpSegInfo.iLL = 0;
	tmpSegInfo.iRR = nNumFramesTotal-1;
	tmpSegVector.push_back(tmpSegInfo);
//
	ZONE_INFO_T tmpSegInfo02;
	for (ZONE_VECTOR::const_iterator zzz=zoneVector.begin(); zzz!=zoneVector.end(); zzz++) {
	// 对一个候选切分区，做。。。

		for (ZONE_VECTOR::iterator seg=tmpSegVector.begin(); seg!=tmpSegVector.end(); seg++) {
		// 对一个现存段，做。。。
			if (zzz->iLL < seg->iLL || seg->iRR < zzz->iRR) continue;
			if (zzz->iRR - seg->iLL < CCC_010*NumFrameSkipsInUnitM) continue;
			if (seg->iRR - zzz->iRR + 1 < CCC_010*NumFrameSkipsInUnitM) continue;
//
			// 可分，则一分为二，。。。
			tmpSegInfo02 = *seg;
			tmpSegVector.erase(seg);
//
			tmpSegInfo.iLL = tmpSegInfo02.iLL;
			tmpSegInfo.iRR = zzz->iRR-1;
			tmpSegVector.push_back(tmpSegInfo);
//
			tmpSegInfo.iLL = zzz->iRR;
			tmpSegInfo.iRR = tmpSegInfo02.iRR;
			tmpSegVector.push_back(tmpSegInfo);

			break;
		}
	}
//
	for (ZONE_VECTOR::const_iterator seg=tmpSegVector.begin(); seg!=tmpSegVector.end(); seg++) {
		segEndFrmVector.push_back(seg->iRR);
	}
}


#endif	// __TRAINING_PHASE

/*
// A return value of -1 indicates an error
int OpenFileWr(const char* psfn);
int OpenFileRd(const char* psfn);

// "psfn" 为绝对路径
#ifdef __TRAINING_PHASE
int CProcessingObj::ProcessExample(const char *psfn, int iClassNo)
#else
// 分类并分段，。。。
int CProcessingObj::ProcessExample(const char *psfn, SEGMENT_VECTOR& segVector)
#endif
{
	int nReturn = CreateSampleDataFile(psfn);
	if (nReturn < 0) {
		return nReturn;
	}

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int& fhandle = nReturn;
	// 打开目标信号采样数据文件，为读信号数据，。。。
	sprintf(m_szfn, "%s%s", m_psOutPath, FN_TEMP_WAVEFORM);
	fhandle = OpenFileRd(m_szfn);
#ifdef __TRAINING_PHASE
	assert(iClassNo > 0);
	m_iClassNo = iClassNo;
//
	if (ProcessTrainingExample(fhandle) != 0) {
		printf("Invalid file %s(ClassNo%02d) !\n", psfn, iClassNo);
//		fprintf(, "Invalid file %s(ClassNo%02d) !\n", psfn, iClassNo);
	}
#else	// __TRAINING_PHASE
	if (SegmentAudio(fhandle, segVector) != 0) {
	}
#endif	// __TRAINING_PHASE
	_close(fhandle);
//
	// 删除临时生成的信号数据文件
	sprintf(m_szfn, "%s%s", m_psOutPath, FN_TEMP_WAVEFORM);
	remove(m_szfn);

	return 0;	// 0 : OK; non 0 : not OK
}

int CProcessingObj::CreateSampleDataFile(const char *psfn)
{
	assert(m_pfWaveform);

// 9999ertgjkll
//
	if ( m_pfWaveform->OpenWaveFile(psfn) < 0 ) {
		return ERROR_NO_WAVE_FORM_FILE;
	}

// 创建文件，临时保存目标音频采样数据
	sprintf(m_szfn, "%s%s", m_psOutPath, FN_TEMP_WAVEFORM);
	int fhandle = OpenFileWr(m_szfn);
	if (fhandle == -1) {
		printf("Can't create file \"%s\" !\n", m_szfn);
		m_pfWaveform->CloseWaveFile();		
		return ERROR_NO_TMP_SAMPLE_DATA_FILE;
	}

	int maxAbs;
	int nReturn = m_pfWaveform->MakeTargetSamplesData(fhandle, maxAbs);	// 生成目标信号采样数据
	_close(fhandle);
	m_pfWaveform->CloseWaveFile();
//
	if (nReturn < 0) {// 生成目标信号采样数据时出现错误，删除临时文件
		sprintf(m_szfn, "%s%s", m_psOutPath, FN_TEMP_WAVEFORM);
		remove(m_szfn);
		return ERROR_NO_TMP_SAMPLE_DATA_FILE;
	}

	return 0;
}
*/
