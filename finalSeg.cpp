

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

#ifndef __TRAINING_PHASE

extern FILE* g_pfwInfo;

int JoinSegments(SEG_INFO_T* pSegInfo, int& nNumSegs, int lenMin);

//

//#define __PIN_POINT_THE_BOUNDARY
#ifndef __PIN_POINT_THE_BOUNDARY
//#define __PIN_POINT_THE_BOUNDARY_02
#endif	// __PIN_POINT_THE_BOUNDARY

#ifdef __PIN_POINT_THE_BOUNDARY
unsigned long PinPoint(const short xxx[]);
#define NUM_FRAME_SKIPS_OF_SMAPLES_READ		NumFrameSkipsInUnitSkipM
#endif	// __PIN_POINT_THE_BOUNDARY
#ifdef __PIN_POINT_THE_BOUNDARY_02
unsigned long StePinPoint(const double ste[]);
#define NUM_FRAME_SKIPS_OF_STE_READ			NumFrameSkipsInUnitM
#endif	// __PIN_POINT_THE_BOUNDARY

//
// 对每个“大段”，都调用一次这个函数
int CProcessingObj::FinalSegProcessing(int fhr_samples, int& nNumSamplesCur, int nNumSamplesLeft, SEGMENT_VECTOR& segVector)
{
	// 存放段的结束位置（单元编号）
	SEG_INFO_T* pSegInfo = Malloc(SEG_INFO_T, m_unitVector.size());	// 以待分段的音频中单元的个数为依据
	if (pSegInfo == NULL) {
		printf("No memory, pSegInfo == NULL !\n");
		return -1;
	}

// （一）对单元进行同类合并（仅仅是同类合并），得自然段，。。。

	int nNumSegs = 0;	// 自然段数
	int iClassNoPre = -1;
	for (SEG_VECTOR_L::size_type iUnit=0; iUnit<m_unitVector.size(); iUnit++) {
		if (m_unitVector[iUnit].classNo != iClassNoPre) {// 新段开始（第一段也是）
			pSegInfo[nNumSegs].classNo = m_unitVector[iUnit].classNo;
			pSegInfo[nNumSegs].numFrames = m_unitVector[iUnit].numFrames;	// 随后一般会被修改
			nNumSegs++;

			iClassNoPre = m_unitVector[iUnit].classNo;
		} else {// 与前段同类，则并入前段，
			pSegInfo[nNumSegs-1].numFrames += m_unitVector[iUnit].numFrames;	// 注意下标，"nNumSegs-1" !!!
		}
	}
//
	m_unitVector.clear();	// 只为释放内存


	if (g_pfwInfo) {
		fprintf(g_pfwInfo, "共有 %d 个自然段，。。。\n", nNumSegs);
	}
	assert(nNumSegs > 0);
	pSegInfo = (SEG_INFO_T*)realloc(pSegInfo, sizeof(SEG_INFO_T)*nNumSegs);	// 释放多余内存

/*	// 输出中间结果，只为调试程序
	if (g_pfwInfo) {
		for (int iSeg=0; iSeg<nNumSegs; iSeg++) {
			fprintf(g_pfwInfo, "UnitNo - %05d : ClassNo - %02d\n", pSegInfo[iSeg].end, pSegInfo[iSeg].classNo);
		}
	}*/

//
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// （二）将过短的段并入相邻段

	int nNumSegsDeleted;
	nNumSegsDeleted = JoinSegments(pSegInfo, nNumSegs, MINMUN_NUM_FRAMES_IN_SEGMENT);
	if (g_pfwInfo) {
		fprintf(g_pfwInfo, "过短的段合并后剩下 %d 段（消去了 %d 个自然段），。。。\n", nNumSegs, nNumSegsDeleted);
	}

// （三）将短于阈值的静音段合并到左邻段中，。。。

	int iSeg;
	int iSegDest = 0;
	for (iSeg=0; iSeg<nNumSegs; iSeg++) {
		if (iSeg == 0) {// 第一段，无论是否静音段，总是保留，。。。
			// 可能是静音段，但因其无左邻段，故保留
			pSegInfo[iSegDest].classNo = pSegInfo[iSeg].classNo;
			pSegInfo[iSegDest].numFrames = pSegInfo[iSeg].numFrames;
			iSegDest++;

			iClassNoPre = pSegInfo[iSeg].classNo;
			continue;	// !!!
		}

		// 只有当左邻段是吸收了一个静音段后，才可能出现这个情况！（即两个同类段之间夹了一个静音段）
		if (pSegInfo[iSeg].classNo == iClassNoPre) {// 并入左邻段，
			pSegInfo[iSegDest-1].numFrames += pSegInfo[iSeg].numFrames;	// 注意下标，"iSegDest-1" !!!
			continue;	// !!!
		}

		if (pSegInfo[iSeg].classNo == 0 && pSegInfo[iSeg].numFrames < NUM_FRAMES_IN_SILENT_SEGMENT_TO_SURVIVE) {
		// 比较短的静音段，则并入左邻段，。。。
			pSegInfo[iSegDest-1].numFrames += pSegInfo[iSeg].numFrames;	// 注意下标，"iSegDest-1" !!!
		} else {// 不是静音段，或是较长的静音段，则保留，。。。			
			pSegInfo[iSegDest].classNo = pSegInfo[iSeg].classNo;
			pSegInfo[iSegDest].numFrames = pSegInfo[iSeg].numFrames;
			iSegDest++;

			iClassNoPre = pSegInfo[iSeg].classNo;
		}
	}
	if (g_pfwInfo) {
		fprintf(g_pfwInfo, "静音段合并后剩下 %d 段（减少了 %d 段），。。。\n", iSegDest, nNumSegs-iSegDest);
	}
	nNumSegs = iSegDest;	// 修改段数

//	
// 注意：每段的帧数是从前一段的结束帧之后的帧到本段结束帧的帧数；
//			显然，第一段的起始帧是第 0 帧。

	// 位移帧数
	int& nOffsetFrames = nNumSegsDeleted;
	nOffsetFrames = NumFrameSkipsInUnitM-(NumFrameSkipsInUnitSkipM+NumFrameSkipsInUnitM)/2;
	int& iSample = iSegDest;
	// 比较合理的段间切分点是相邻单元（即前段的结束单元和后段的开始单元）覆盖范围的中心点，。。。
	int nNumFrames = -nOffsetFrames;
	for (iSeg=0; iSeg<nNumSegs; iSeg++) {
		nNumFrames += pSegInfo[iSeg].numFrames;	// 到本段末尾帧为止的总帧数
		nNumFrames += (iSeg == nNumSegs-1)?nOffsetFrames:0;	// 位移帧数回补，否则总长就会不对了
		iSample = (nNumFrames-1)*NumSamplesInFrameSkipM+NumSamplesInFrameM;	// 将帧数换算成采样个数！！！
		if (iSeg == nNumSegs-1) {// 是本“大段”的最后一段，
			iSample -= NumSamplesInFrameM-NumSamplesInFrameSkipM;	// 扣除一部分，留给下一“大段”
			// 注意：只有对最后一个“大段”，"nNumSamplesLeft" 的值才不为 0
			iSample += nNumSamplesLeft;	// 加上剩余的不足一帧跳步的采样个数
		}
//
		iSample += nNumSamplesCur;	// 加上以前所有“大段”累计的采样数
		if (iSeg == nNumSegs-1) {// 是本“大段”的最后一段，
			// 记下新的以前所有“大段”累计采样数，以备处理后一“大段”时用
			nNumSamplesCur = iSample;
		}
//
		// 将“段类别号”和“段结束位置（毫秒）”输出到向量中
		segVector.push_back(CSegment(pSegInfo[iSeg].classNo, 1000.0*(iSample-1)/UNIFIED_SAMPLING_RATE));
	}

	free(pSegInfo);

	return 0;
}

#endif	// #ifndef __TRAINING_PHASE

