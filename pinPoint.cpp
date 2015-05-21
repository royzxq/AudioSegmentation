

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

int CProcessingObj::FinePosition(int fhr_samples, SEG_INFO_T* pSegInfo, int nNumSegs)
{
// （四）
#ifdef __PIN_POINT_THE_BOUNDARY
	// 采样数据缓存
	// 每个单元的帧跳数，每个帧跳的采样点数
	short* psamples = Malloc(short, NumFrameSkipsInUnitM*NumSamplesInFrameSkipM);	// 其实比实际需要的多了
	if (psamples == NULL) {
		printf("psamples == NULL !\n");
		return -2;
	}

	// 指针移到文件头
	_lseek(fhr_samples, 0, SEEK_SET);
	// 信号数据文件开头的 2 个参数
#ifdef _DEBUG
	int sr;
	unsigned long lstart, num_samples;
	if (_read(fhr_samples, &sr, sizeof(int)) != sizeof(int)) {// 采样率
	}
	if (_read(fhr_samples, &lstart, sizeof(unsigned long)) != sizeof(unsigned long)) {
	}
	if (_read(fhr_samples, &num_samples, sizeof(unsigned long)) != sizeof(unsigned long)) {// 采样点个数
	}
#else
	_lseek(fhr_samples, sizeof(int)+sizeof(unsigned long)+sizeof(unsigned long), SEEK_SET);	// 直接跳过参数
#endif
	int& sample_ptr = iSeg02;
	sample_ptr = 0;	// 数据文件中当前元素的位置（元素的下标）
#endif	// #ifdef __PIN_POINT_THE_BOUNDARY
#ifdef __PIN_POINT_THE_BOUNDARY_02
	sprintf(m_szfn, "%s%s", m_psOutPath, FN_TEMP_STE);
	FILE *pfrSTE = fopen(m_szfn, "rb");	// 打开为读数据，。。。

	double* pSTE = Malloc(double, NUM_FRAME_SKIPS_OF_STE_READ);
	if (pSTE == NULL) {
		printf("pSTE == NULL !\n");
		fclose(pfrSTE);
		return -2;
	}
	int& double_ptr = iSeg02;
	double_ptr = 0;	// 数据文件中当前元素的位置（元素的下标）
#endif	// __PIN_POINT_THE_BOUNDARY_02

// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	int iStartSampleOfNextSeg;
	for (int iSeg=0; iSeg<nNumSegs; iSeg++) {
		if (iSeg == nNumSegs-1) {
			pSegInfo[iSeg].numFrames = pSegInfo[iSeg].end*NumSamplesInFrameSkipM;	// 段的结束帧的起始采样的下标 
			pSegInfo[iSeg].numFrames = pSegInfo[iSeg].end+NumSamplesInFrameM;	// 加上结束帧的采样个数 
			pSegInfo[iSeg].numFrames = pSegInfo[iSeg].end-1;	// 减一后得最后一个采样的下标
			break;
		}
		// 当前段的最后一个单元的起始帧的（全局）下标，（假想！）
		iStartSampleOfNextSeg = pSegInfo[iSeg].end-NumFrameSkipsInUnitM+1+
								// 段分界处两边相邻两个单元覆盖范围的中心处
								(NumFrameSkipsInUnitM+NumFrameSkipsInUnitSkipM)/2;
#ifdef __PIN_POINT_THE_BOUNDARY
		// 回退半个单元跳步（对应的帧数）
		iStartSampleOfNextSeg -= NUM_FRAME_SKIPS_OF_SMAPLES_READ/2;
		iStartSampleOfNextSeg *= NumSamplesInFrameSkipM;	// 转换为采样下标
		// 将文件指针位置移到下标为 "iStartSampleOfNextSeg" 的信号采样的位置 
		_lseek(fhr_samples, (iStartSampleOfNextSeg-sample_ptr)*sizeof(short), SEEK_CUR);
		// 读出 1 个单元跳步对应的信号采样数据
		if (_read(fhr_samples, psamples, NUM_FRAME_SKIPS_OF_SMAPLES_READ*NumSamplesInFrameSkipM*sizeof(short)) != 
										NUM_FRAME_SKIPS_OF_SMAPLES_READ*NumSamplesInFrameSkipM*sizeof(short)) {
			break;
		}
		// 1 个单元跳步对应的信号采样数据
		// 文件指针当前位置对应的信号采样的下标
		sample_ptr = iStartSampleOfNextSeg+NUM_FRAME_SKIPS_OF_SMAPLES_READ*NumSamplesInFrameSkipM;

		// 求段的精确结束位置（毫秒）
		iStartSampleOfNextSeg += PinPoint(psamples);
#endif	// #ifdef __PIN_POINT_THE_BOUNDARY
#ifdef __PIN_POINT_THE_BOUNDARY_02
		iStartSampleOfNextSeg -= NUM_FRAME_SKIPS_OF_STE_READ/2;
		fseek(pfrSTE, (iStartSampleOfNextSeg-double_ptr)*sizeof(double), SEEK_CUR);
		if (fread(pSTE, sizeof(double), NUM_FRAME_SKIPS_OF_STE_READ, pfrSTE) != NUM_FRAME_SKIPS_OF_STE_READ) {
			break;
		}
		double_ptr = iStartSampleOfNextSeg+NUM_FRAME_SKIPS_OF_STE_READ;

		iStartSampleOfNextSeg *= NumSamplesInFrameSkipM;	// 帧下标转换为采样下标
		iStartSampleOfNextSeg += StePinPoint(pSTE);
#endif	// #ifdef __PIN_POINT_THE_BOUNDARY_02
//
		pSegInfo[iSeg].end = iStartSampleOfNextSeg-1;	// 段结束位置从此前的帧下标变为信号采样点下标，备后用。
	}
#ifdef __PIN_POINT_THE_BOUNDARY
	free(psamples);	// 采样数据缓存
	psamples = NULL;
#endif	// __PIN_POINT_THE_BOUNDARY
#ifdef __PIN_POINT_THE_BOUNDARY_02
	fclose(pfrSTE);
	free(pSTE);
#endif	// __PIN_POINT_THE_BOUNDARY_02

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// 将过短的段合并到相邻段中
	nNumSegsDeleted = JoinSegments(pSegInfo, nNumSegs, MINMUN_NUM_FRAMES_IN_SEGMENT);
	if (g_pfwInfo) {
		fprintf(g_pfwInfo, "\n最终分段信息，。。。\n");
		fprintf(g_pfwInfo, "\n合并后剩下 %d 段（消去了 %d 个过短段），。。。\n", nNumSegs, nNumSegsDeleted);
	}
*/
	return 0;
}

#ifdef __PIN_POINT_THE_BOUNDARY_02

// 找最宜切点，。。。
// "ste[]" 当前单元（unit）的前 "NumFrameSkipsInUnitSkipM" 帧跳对应的信号
unsigned long StePinPoint(const double ste[])
{
	int mm = NUM_FRAME_SKIPS_OF_STE_READ/2;	// 中心帧
	if (ste[mm] <= MINIMUM_MEAN_ABS_SIGNAL_VALUE) {
		return mm*NumSamplesInFrameSkipM;
	}
	int ll = mm-1;	// 往左
	int rr = mm+1;	// 往右
	while (ll >= 0 || rr < NUM_FRAME_SKIPS_OF_STE_READ) {
		if (ll >= 0) {
			if (ste[ll] <= MINIMUM_MEAN_ABS_SIGNAL_VALUE) {
				return ll*NumSamplesInFrameSkipM;	// 先碰到的，则定
			}
			ll--;
		}
		if (rr < NUM_FRAME_SKIPS_OF_STE_READ) {
			if (ste[rr] <= MINIMUM_MEAN_ABS_SIGNAL_VALUE) {
				return rr*NumSamplesInFrameSkipM;	// 先碰到的，则定
			}
			rr++;
		}
	}

	return mm*NumSamplesInFrameSkipM;	// 没办法了
}

#endif	// #ifdef __PIN_POINT_THE_BOUNDARY_02

#ifdef __PIN_POINT_THE_BOUNDARY

// 找最宜切点，。。。
// "xxx[]" 当前单元（unit）的前 "NumFrameSkipsInUnitSkipM" 帧跳对应的信号
// 找信号能量最小的帧
unsigned long PinPoint(const short xxx[])
{
	int isampleG = 0;
	double dbmin = DBL_MAX;
	unsigned long imin;
	double dbsum;
	for (int ifrm=0; ifrm<NUM_FRAME_SKIPS_OF_SMAPLES_READ; ifrm++) {// 1 个单元跳步对应的帧跳步个数
		dbsum = 0.0;
		for (int ii=0; ii<NumSamplesInFrameSkipM; ii++) {
			dbsum += abs(xxx[isampleG]); isampleG++;
		}
		if (dbmin > dbsum) {
			dbmin = dbsum; imin = ifrm;
		}
	}
	imin *= NumSamplesInFrameSkipM;	// 将帧下标变成信号采样点的下标（相对值，当前单元内）
	return imin;
}

#endif	// #ifdef __PIN_POINT_THE_BOUNDARY

#endif	// #ifndef __TRAINING_PHASE

