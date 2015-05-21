

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

void STE(const short xxx[], int NNN, double& ste);

#define NumShortsBufferCanHold	(1023*NumSamplesInFrameSkipM+NumSamplesInFrameM)	// 1024 帧

// 计算各个完整帧的 STE 值，并写入临时文件，返回最后剩余的不足一帧跳步的采样个数
int CProcessingObj::SegmentAudioHelper02(int fhr)
{
	short* pBuf = Malloc(short, NumShortsBufferCanHold);	// 采样个数
	if (pBuf == NULL) {
		printf("No memory, pBuf == NULL !\n");
		return -1;
	}
//
	// 创建临时文件，。。。
	FILE* pfwSTE;
	sprintf(m_szfn, "%s%s", m_psOutPath, FN_TEMP_STE);
	pfwSTE = fopen(m_szfn, "wb");	// 创建文件，
	if (pfwSTE == NULL) {
		printf("pfwSTE == NULL, Can't create file !\n");
		free(pBuf);
		return -2;
	}
//
	int nNumFrames = 0;
	if (fwrite(&nNumFrames, sizeof(int), 1, pfwSTE) != 1) {// 占位
	}
//
// 信号数据文件头上的 3 个参数
	_lseek(fhr, 0, SEEK_SET);	// 文件头
#ifdef _DEBUG
	int sr;
	unsigned long lstart, num_samples;
	if (_read(fhr, &sr, sizeof(int)) != sizeof(int)) {// 采样率
	}
	if (_read(fhr, &lstart, sizeof(unsigned long)) != sizeof(unsigned long)) {
	}
	if (_read(fhr, &num_samples, sizeof(unsigned long)) != sizeof(unsigned long)) {// 采样点个数
	}
#else
	_lseek(fhr, sizeof(int)+sizeof(unsigned long)+sizeof(unsigned long), SEEK_SET);	// 直接跳过参数
#endif
//
	double dbSTE;
	int nNumShorts = 0;
	short* pCurFrame;
	while (1) {
		int nBytesRead = _read(fhr, pBuf+nNumShorts, (NumShortsBufferCanHold-nNumShorts)*sizeof(short));
		if (nBytesRead <= 0) break;

		nNumShorts += nBytesRead/sizeof(short);	// 原有的加上新读入的（字节数 --> 采样个数）
		pCurFrame = pBuf;
		while (nNumShorts >= NumSamplesInFrameM) {// 够一帧（而不是一帧跳步）了，
			STE(pCurFrame, NumSamplesInFrameSkipM, dbSTE);	// 只基于一帧跳对应的采样做帧的 STE 值计算！
			if (fwrite(&dbSTE, sizeof(double), 1, pfwSTE) != 1) {
				break;
			}
			nNumFrames++;
		//
			nNumShorts -= NumSamplesInFrameSkipM;	// 未处理的采样个数
			pCurFrame += NumSamplesInFrameSkipM;	// 未处理的采样开始位置
		}
		// 剩下的不足一帧的采样
		memmove(pBuf, pCurFrame, nNumShorts*sizeof(short));
	}
	free(pBuf);
//
	fseek(pfwSTE, 0, SEEK_SET);
	if (fwrite(&nNumFrames, sizeof(int), 1, pfwSTE) != 1) {
	}
	fclose(pfwSTE);
//
	nNumShorts -= NumSamplesInFrameM-NumSamplesInFrameSkipM;

	return nNumShorts;	// 最后剩余的不满一个帧跳的采样个数
}

void CProcessingObj::JoinSameClassSegs(SEGMENT_VECTOR& segVector)
{
	int nClassNoPre = -1;	// 必须设这个变量！
	SEGMENT_VECTOR::iterator itrPre;
//
	SEGMENT_VECTOR::iterator itr = segVector.begin();
	while (itr != segVector.end()) {
		if (itr->nClassNo == nClassNoPre) {// 与左段同类，。。。
			itrPre->lSegEnd = itr->lSegEnd;	// 并入左段
			itr = segVector.erase(itr);	// 删除本段！自然进到下一个元素（itr）
			continue;
		}

		// 与左段不同类，则保留
		nClassNoPre = itr->nClassNo;
		itrPre = itr;

		itr++;	// 进到下一个元素（itr）
	}
}

void CProcessingObj::SaveSegInfoToFile(SEGMENT_VECTOR& segVector)
{
	if (g_pfwInfo == NULL) return;

	fprintf(g_pfwInfo, "\n最终分段信息（共 %d 段）。。。\n", segVector.size());

	long nLenTotal = segVector.back().lSegEnd+1;	// 总长（毫秒）

	short hhh, mmm;	// 时、分
	ldiv_t lDivR;
	long lEndPre = -1;
	for (SEGMENT_VECTOR::size_type iSeg=0; iSeg<segVector.size(); iSeg++) {
		// 段结束位置（采样下标对应的时间）
		lDivR.rem = segVector[iSeg].lSegEnd;	// ms

		lDivR = ldiv(lDivR.rem, 60*60*1000);	// 时
		hhh = lDivR.quot;
		lDivR = ldiv(lDivR.rem, 60*1000);	// 分
		mmm = lDivR.quot;
		lDivR = ldiv(lDivR.rem, 1000);	// 秒
		fprintf(g_pfwInfo, "%05d - %02d:%02d:%02d:%03d(", iSeg+1, hhh, mmm, lDivR.quot, lDivR.rem);

		// 求段长（毫秒数）
		lDivR.rem = segVector[iSeg].lSegEnd-lEndPre;
//
		lDivR = ldiv(lDivR.rem, 60*60*1000);	// 时
		hhh = lDivR.quot;
		lDivR = ldiv(lDivR.rem, 60*1000);	// 分
		mmm = lDivR.quot;
		lDivR = ldiv(lDivR.rem, 1000);	// 秒
		fprintf(g_pfwInfo, "%02d:%02d:%02d:%03d) ", hhh, mmm, lDivR.quot, lDivR.rem);
		fprintf(g_pfwInfo, "ClassNo%02d ", segVector[iSeg].nClassNo);

		for (int mm=0; mm<500.0*(segVector[iSeg].lSegEnd-lEndPre)/nLenTotal; mm++) {
			fprintf(g_pfwInfo, "*");
		}
		fprintf(g_pfwInfo, "\n");

		lEndPre = segVector[iSeg].lSegEnd;	// ms
	}
}

#endif	// __TRAINING_PHASE

