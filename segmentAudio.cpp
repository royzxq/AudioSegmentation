

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

#include "svmInterface.h"

// "fhr_samples" （读文件）待分段的音频信号文件
int CProcessingObj::SegmentAudio(int fhr_samples, SEGMENT_VECTOR& segVector)
{
	// 每个段的末尾帧（完整帧）下标
	std::vector<int> initSegEndVector;
	// 返回值：剩余的不足一帧跳的采样个数
	int nNumSamplesLeft = SegmentAudio02(fhr_samples, initSegEndVector);
	if (nNumSamplesLeft < 0) {
		return -1;
	}
	if (initSegEndVector.back()+1 < NumFrameSkipsInUnitM) {// "initSegEndVector.back()+1" 总帧跳数
		printf("Length less than a unit, abort !\n");
		return -2;
	}
// SVM things
	CSvmInterface* pSvm = new CSvmInterface(m_psInPath, m_psOutPath);
	if (pSvm == NULL || pSvm->LoadSvmModel(m_nNumDims) != 0) {
		return -3;
	}

// 缓存大小：正好放下计算一个单元所需的信号采样数据
	int nBufSizeInShorts = NumSamplesInFrameSkipM*(NumFrameSkipsInUnitM-1)+NumSamplesInFrameM;
	short* psampd = (short*)malloc(sizeof(short)*nBufSizeInShorts);
	if (psampd == NULL) {
		delete pSvm;
		return -4;
	}

// 信号数据文件头上的 3 个参数
	_lseek(fhr_samples, 0, SEEK_SET);	// 文件头
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

	SEG_INFO_T segInfo;
	short* pCurFrame;
	int nNumUnits;
	int nShorts = 0;	// a must !
	int iFrmG = 0;	// 完整帧之全局下标
	int nNumSamplesCur = 0;	// 已处理的大段中采样个数累计
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

// 约定若干帧构成一个单元（unit），对每个单元进行分类；
// 从约定：单元间可能有重叠，或相接，但不相离！！！
	for (std::vector<int>::size_type isub=0; isub<initSegEndVector.size(); isub++) {
		// 创建临时文件，。。。
		sprintf(m_szfn, "%s%s", m_psOutPath, FN_TEMP_STE);
		m_pfSTE = fopen(m_szfn, "wb");	// 创建文件，
		if (m_pfSTE == NULL) {
			printf("m_pfSTE == NULL, Can't create file !\n");
		}
		m_unitVector.clear();	// 必须的
//
		m_nNumFrames = 0;	// 从当前样本中提取的帧数计数（单元内帧数计数）
		nNumUnits = 0;	// 当前音频中单元个数
		while (1) {
			int nBytesRead = _read(fhr_samples, psampd+nShorts, (nBufSizeInShorts-nShorts)*sizeof(short));
			if (nBytesRead <= 0 && nBufSizeInShorts > nShorts) {// 没有采样数据可读了，或出错了，退出，。。。
				break;
			}
//
			nShorts += nBytesRead/sizeof(short);	// 缓存中采样（short）个数（原有的加新读的）！！！
			pCurFrame = psampd;	// 缓存首址
			while (nShorts >= NumSamplesInFrameM) {// 缓存中待处理的采样个数够一帧，。。。
				if (iFrmG > initSegEndVector[isub]) {// 当前大段的帧数刚满，对其做最后处理。。。
					memmove(psampd, pCurFrame, nShorts*sizeof(short));
					goto PROC_SECTION;
				}
				ProcSingleFrame(pCurFrame);	// 对这一帧信号做相关处理
				m_nNumFrames++;	// 帧数计数
				iFrmG++;
//
				if (m_nNumFrames == NumFrameSkipsInUnitM) {// 处理的帧数够一个单元了，则做分类等处理，。。。
					int iClassNo;
					// 生成当前 unit 的特征向量，。。。
					if (MakeFeatureVector() == 0) {// 特征向量 OK，。。。
						// 则识别当前 unit 的类别（以 unit 为单位做分类），。。。
						iClassNo = pSvm->ppsvmPredict(m_pvector, m_nNumDims);
					} else {// 特征向量 NOT OK，。。。
						iClassNo = 0;
					}
					segInfo.classNo = iClassNo;
//
					segInfo.numFrames = NumFrameSkipsInUnitSkipM;
					if (nNumUnits == 0) {
						segInfo.numFrames = NumFrameSkipsInUnitM;
					}
//
					m_unitVector.push_back(segInfo);	// 单元结尾帧的下标
					nNumUnits++;
/*					if (g_pfwInfo) {// "g_pfwInfo" 可以为空（这样就不输出文件）
						fprintf(g_pfwInfo, "iUnit : %05d[%09.3f - ], iClassNo : %02d\n", 
								m_nNumUnits-1, 20.0*NumFrameSkipsInUnitSkipM*(m_nNumUnits-1)/1000.0, iClassNo);
					}*/
//
// 用越小的 "NumFrameSkipsInUnitSkipM" 值，段界位置精度越高，。。。
// 将要被再利用的“帧特征向量”个数为 "NumFrameSkipsInUnitM-NumFrameSkipsInUnitSkipM"

					MoveUnitOverlap();
				}	// end of "if (m_nNumFrames == NumFrameSkipsInUnitM) {"

				pCurFrame += NumSamplesInFrameSkipM;	// 跳一步（帧跳），下一帧信号首址
				nShorts -= NumSamplesInFrameSkipM;	// 待处理信号采样个数变少

				if (nNumUnits == MAX_NUM_UNITS) {// 内存用完了，后面的单元不处理了，。。。
					printf("iUnit == MAX_NUM_UNITS !\n");
					break;
				}
			}	// end of "while (nShorts >= NumSamplesInFrameM) {// 缓存中待处理的采样个数够一帧，。。。"

			if ((nBytesRead % sizeof(short)) != 0) {// 文件有错！！！
				printf("Sample data invalid !\n");
				break;
			}

			// 剩下的待处理采样不够一帧了，将其移到缓存首，待随后补齐后再处理，。。。
			memmove(psampd, pCurFrame, sizeof(short)*nShorts);
//
			if (nNumUnits == MAX_NUM_UNITS) {
				break;
			}
		}	// end of "while (1) {"

PROC_SECTION:
		// 当前大段的最后一个单元的结束帧的下标，需要调整，。。。
		segInfo = m_unitVector.back();	// 取最后一个单元的值
		m_unitVector.pop_back();	// 删除最后一个单元
		// 加上不足一个单元跳步的剩余帧（完整帧）数
		segInfo.numFrames += m_nNumFrames-(NumFrameSkipsInUnitM-NumFrameSkipsInUnitSkipM);
		m_unitVector.push_back(segInfo);
//
		fclose(m_pfSTE);	// 关闭新创建的文件
		m_pfSTE = NULL;

		//////////////////////////////////////////////////

		if (g_pfwInfo) {
			fprintf(g_pfwInfo, "\n处理第 %d 大段，。。。\n", isub+1);
		}
		
		// "nNumSamplesLeft" 不足一帧跳步的采样个数
		int& nNumberOfSamples = nNumUnits;
		nNumberOfSamples = 0;
		if (isub == initSegEndVector.size()-1) {// 是最后一个“大段”
			nNumberOfSamples = nNumSamplesLeft;
			nNumberOfSamples += NumSamplesInFrameM-NumSamplesInFrameSkipM;	// 多加，备扣除，。。。
		}
		// 做最终分段（给出精确的段分界点）
		FinalSegProcessing(fhr_samples, nNumSamplesCur, nNumberOfSamples, segVector);

		sprintf(m_szfn, "%s%s", m_psOutPath, FN_TEMP_STE);
		remove(m_szfn);
	}	// end of "for () {"
//
//////////////////////////////////////////////////////////////////////////////////

	free(psampd);
	delete pSvm;
// 释放特征计算所用的内存
//	ReleaseMemory();	// 改为不释放内存，是为了能连续对多个音频文件进行分段！！！

	return 0;
}

#endif	// #ifndef __TRAINING_PHASE

