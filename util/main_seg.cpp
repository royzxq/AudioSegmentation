
#include "stdafx.h"

#include <assert.h>
#include <FLOAT.H>	// for DBL_MAX

#include <math.h>
#include <stdio.h>
#include <io.h>

#include <stdio.h>
#include <malloc.h>

#include <string.h>

#include "parameters.h"

#include "main_seg.h"
#include "my_util.h"

extern const char *g_psOutRootPath;

//
///////////////////////////////////////////////////////////////////////////////////////////////////////

#define __CUT_SMALL_HEAD_AND_TAIL

// head_tail_cut_001
#ifdef __CUT_SMALL_HEAD_AND_TAIL
// 在找到了平均能量最大的段之后，为了削掉这个段的头向右和尾向左可能的弱音部分，。。。
// 信号值的绝对值平均是在这么长（以 skips 的个数计）的窗（称为“弱音检测窗”）中计算的
#define	NUM_SKIPS_IN_DETECT_WIN02			2		// accounts for 0.06 seconds
#endif
// head_tail_cut_001


// 检测窗口的大小（由外界设定），以跳步个数计（每跳步固定为 30 毫秒）！
#define DetectWinSizeInSkips		m_usMaxMainSegSizeInSkips	// 别名

// 检测值缓存区大小（以 double 值个数计），正好能放下 "DetectWinSizeInSkips" 个检测值 
// 每个检测值是个 double 值，对应一个检测窗！！！
// 检测窗以跳步右移，就得一系列检测窗，每个检测窗对应一个检测值，这个缓存区就是用来保存这些检测值的
// 为什么要缓存（记住最大检测值和其检测窗位置不就得了）？为了检测值计算程序的效率（采样数据只从文件读一次）！！！
#define BufferSizeInDoubles		DetectWinSizeInSkips	// 别别名

#define STR_TEMP_SAMPS_FILE_NAME	"oss_cb.bin"

CMainSegment::CMainSegment()
{
	// 设关键段检测窗口的宽度（也即关键段宽度），以窗口跳步个数计，跳步固定为 30 毫秒。
	m_usMaxMainSegSizeInSkips = 300;	// in skips, 30 ms per skip
//
	// 检测窗的跳步大小，固定为 30 毫秒时长对应的信号采样个数（与“目标采样率”有关！）
	m_usNumSampsInSkip	= (double)UNIFIED_SAMPLING_RATE/1000.0*MILISECONDS_DETECT_SKIP;
	m_pdbData = (double *)malloc(sizeof(double)*BufferSizeInDoubles);

	// 缓冲区 "m_pdbData" 中的“第一个 double 值”对应的 skip 序号（skip 序号从 0 开始）
	m_lSkipIdxG_FirstValInBuffer = 0;
	// 缓冲区 "m_pdbData" 中的“第一个值”的下标（相对于缓冲区的起始位置）
	m_shFirstValIdxInBufferL = 0;

	m_lCurSkipIdxG = 0;
	m_dbValCurSkip = 0.0;
	m_lStartSampIdxG_NextSkip = m_usNumSampsInSkip;

	m_lSkipIdxG_MaxDetectVal = -1;

	m_lNumOriginalSamples = 0;

	// 此临时文件用于存放欠采样、声道合并（或选其一）之后的音频信号数据。与原始音频数据对应的时长一样！！！
	char *psName = (char*)malloc(260);
	sprintf(psName, "%s%s", g_psOutRootPath, STR_TEMP_SAMPS_FILE_NAME);
	m_pfProcessedSamps = fopen(psName, "wb");	// 创建
	free(psName);

	_Xn_Max = 0;
}

CMainSegment::~CMainSegment()
{
	if (m_pdbData)
		free(m_pdbData);

	if (m_pfProcessedSamps) {
		fclose(m_pfProcessedSamps);
	}
	char *psName = (char*)malloc(260);
	sprintf(psName, "%s%s", g_psOutRootPath, STR_TEMP_SAMPS_FILE_NAME);
	remove(psName);	// remove the temp file
	free(psName);
}

/*
void CMainSegment::StartDetection()
{

}
void CMainSegment::DetectionEnd()
{

}
*/

// 在检测出关键段（音乐中能量最大的段）之后，可能还要“掐头去尾”（因为头尾能量太小）
// "pfw_vsd_b" 保存最终选出的音乐关键段的音频信号数据
// 输出参数 "*pulStart" 和 "*pulNumSamps" 都针对目标采样率！
// 
int CMainSegment::WriteMainSegToFile(int fhw, unsigned long *pulStart, unsigned long *pulNumSamps)
{
	if (m_pfProcessedSamps) { fclose(m_pfProcessedSamps); }

//	printf("\nMaximum signal value : %u\n", _Xn_Max);	// 当前歌曲中信号值绝对值最大值
//	fprintf(, "\nMaximum signal value : %u\n", _Xn_Max);	// 当前歌曲中信号值绝对值最大值

// ////////////////////////////////////////////////////////////////////////////////////////////

	char *psName = (char*)malloc(260);
	sprintf(psName, "%s%s", g_psOutRootPath, STR_TEMP_SAMPS_FILE_NAME);
	m_pfProcessedSamps = fopen(psName, "rb");	// 读
	free(psName);

//（一）如关键段不够长则舍弃此音乐

	int nReturn = 0;
	unsigned long lStartSampleGlobal, lNumValidSamples;
	if (m_lSkipIdxG_MaxDetectVal == -1) {// 音频段的长度不足一个检测窗口，关键段的长度为音频实际长度
		lStartSampleGlobal = 0;
		lNumValidSamples = m_lNumOriginalSamples;

	} else {// 这时关键段的长度为检测窗口宽度（此即门限值）！
		lStartSampleGlobal = m_lSkipIdxG_MaxDetectVal*m_usNumSampsInSkip;
		lNumValidSamples = DetectWinSizeInSkips*m_usNumSampsInSkip;
	}
	if (lNumValidSamples < (MINIMUM_MUSIC_LENGTH_ALLOWED_IN_SECONDS*UNIFIED_SAMPLING_RATE)) {
		printf("Music file too short, %8.4f(s), abort !\n", (double)lNumValidSamples/UNIFIED_SAMPLING_RATE);
		printf("Music file too short, %8.4f(s), abort !\n", (double)lNumValidSamples/UNIFIED_SAMPLING_RATE);
		nReturn = -10; goto ExitHere;
	}

	unsigned long lNeedToReadSize;
	size_t nSizeRead;
	size_t nSizeInShorts = BufferSizeInDoubles*sizeof(double)/sizeof(short);

// head_tail_cut_002
#ifdef __CUT_SMALL_HEAD_AND_TAIL
//（二）如果关键段头尾能量太小，就要“掐头去尾”。按现在的处理方法（两头堵）得出的关键段，其中部可能
//		有能量不合要求（即能量太小）的区域！

	// 将文件指针移到关键段起始位置
	fseek(m_pfProcessedSamps, sizeof(short)*lStartSampleGlobal, SEEK_SET);

	// 检测窗（02）由几个 skip 构成，就得有几个单元来记录这些 skips 各自内部信号值的绝对值之和
	double dbValSkip[NUM_SKIPS_IN_DETECT_WIN02];
	int iSkip = 0;
	dbValSkip[iSkip] = 0.0;

	int nNumSkips = 0;	// a detection window consists of "NUM_SKIPS_IN_DETECT_WIN02" skips
	int nNumSamples = 0;	// a skip consists of "m_usNumSampsInSkip" samples
	int iPosBuffL;	// point to a position in the local buffer
	int iSampleGlobal = lStartSampleGlobal;
	int bStatus = 0;
	unsigned long iSampleStartG, iSampleStopG;

	// 借用 "m_pdbData[]" 来存放信号采样数据（每个采样是一个 "short"）
	// figure out the number of shorts the buffer can hold
	lNeedToReadSize = lNumValidSamples;
	while (!feof(m_pfProcessedSamps) && lNeedToReadSize > 0) {
		nSizeRead = (lNeedToReadSize > nSizeInShorts)?nSizeInShorts:lNeedToReadSize;
		nSizeRead = fread(m_pdbData, sizeof(short), nSizeRead, m_pfProcessedSamps);
		if (nSizeRead == 0) { break; }

		///////////////////////////////////////////////////////////////////////////

		iPosBuffL = 0;
		while (iPosBuffL < nSizeRead) {// 本次从文件读出的数据未处理完，。。。
			dbValSkip[iSkip] += abs(((short *)m_pdbData)[iPosBuffL]);
			iPosBuffL++;	// next sample in the local buffer

			nNumSamples++;
			if (nNumSamples < m_usNumSampsInSkip) {// 本 skip 里的采样点个数不够，。。。
				continue;
			}

			// 一个 skip 满了，。。。

			nNumSkips++;	// skip 个数计数
			if (nNumSkips == NUM_SKIPS_IN_DETECT_WIN02) {// 满一个检测窗（弱音检测窗），计算其检测值
				// 将检测窗中所有 skip 的值累加起来
				double dbValWindow = 0.0;
				for (int ii=0; ii<NUM_SKIPS_IN_DETECT_WIN02; ii++) {
					dbValWindow += dbValSkip[ii]; 
				}
				dbValWindow /= NUM_SKIPS_IN_DETECT_WIN02*m_usNumSampsInSkip;

				if (bStatus == 0) {// 表明尚未找到起始点，。。。
					if (dbValWindow >= MINIMUM_MEAN_ABS_SIGNAL_VALUE) {// 碰到符合要求的检测窗，。。。
						bStatus = 1;	// 标示找到起始点了
						// 本检测窗对应的第一个采样点的位置
						iSampleStartG = iSampleGlobal + iPosBuffL-m_usNumSampsInSkip*NUM_SKIPS_IN_DETECT_WIN02;
						// 本检测窗对应的最后一个采样点的位置
						iSampleStopG = iSampleGlobal + iPosBuffL-1;
					}
				} else {// 起始点已找到（即非第一个符合要求的检测窗），。。。
				// 这里的处理方法模拟了从尾向左扫描，直到遇到合适的检测窗位置
					if (dbValWindow >= MINIMUM_MEAN_ABS_SIGNAL_VALUE) {
					// 记下位置即可。最终记的是最后一个符合要求的检测窗位置的最后一个采样点的位置
						iSampleStopG = iSampleGlobal + iPosBuffL-1;
					} else {
					// 中间碰到了不符合要求的检测窗，但不能就此截断，因为后面可能还有符合要求的检测窗！不完美！！！
					}
				}

				// 意思是说，在检测窗的 skip 检测值缓存中，离现在最久远的 skip 的检测值无用了（其余还有用），
				// 所以 skip 个数少减一个
				nNumSkips--;
			}

			iSkip++;	// 在检测窗的 skip 检测值缓存中，哪个单元将用来存下一个将计算的 skip 的检测值
			if (iSkip == NUM_SKIPS_IN_DETECT_WIN02) {
				iSkip = 0;	// skip 检测值缓存是个循环 buffer ！
			}
			dbValSkip[iSkip] = 0.0;	// 清零准备累加
			nNumSamples = 0;	// 下一个 skip 中的采样点个数计数
		}

		////////////////////////////////////////////////////////////////

		iSampleGlobal += nSizeRead;	// 本地缓存首地址对应的全局采样点序号
		lNeedToReadSize -= nSizeRead;	// decrease
	}

	if (bStatus == 0) {// 所有检测窗位置都不合要求
		lNumValidSamples = 0;
		printf("Music signal too weak !\n");
	} else {
		unsigned long nHeadCut = iSampleStartG - lStartSampleGlobal;
		unsigned long nTailCut = (lStartSampleGlobal+lNumValidSamples-1)-iSampleStopG;
		lNumValidSamples -= nHeadCut+nTailCut;
		lStartSampleGlobal = iSampleStartG;
		if (nHeadCut > 0) {
			printf("Head cut, %8.4f(s) !\n", (double)nHeadCut/UNIFIED_SAMPLING_RATE);
		}
		if (nTailCut > 0) {
			printf("Tail cut, %8.4f(s) !\n", (double)nTailCut/UNIFIED_SAMPLING_RATE);
		}
	}

	if (lNumValidSamples < (MINIMUM_MUSIC_LENGTH_ALLOWED_IN_SECONDS*UNIFIED_SAMPLING_RATE)) {
		printf("Music signal too short, %8.4f(s), abort !\n", (double)lNumValidSamples/UNIFIED_SAMPLING_RATE);
		nReturn = -20; goto ExitHere;
	}
#endif
// head_tail_cut_002

	//（三）将最终确定的关键段数据从临时文件读出并写入“关键段数据文件”

	// input file of original samples
	fseek(m_pfProcessedSamps, sizeof(short)*lStartSampleGlobal, SEEK_SET);	// move to the right pos. for reading op.

	// 文件头有 3 个元素！
	int& sr = nNumSkips;	// 借用 "nNumSkips" ！
	sr = UNIFIED_SAMPLING_RATE;
	_write(fhw, &sr, sizeof(int));	// 采样频率（可能是欠采样后的采样频率）
	_write(fhw, &lStartSampleGlobal, sizeof(unsigned long));	// index to the first valid sample
	_write(fhw, &lNumValidSamples, sizeof(unsigned long));	// number of valid samples

	lNeedToReadSize = lNumValidSamples;
	while (!feof(m_pfProcessedSamps) && lNeedToReadSize > 0) {
		nSizeRead = (lNeedToReadSize > nSizeInShorts)?nSizeInShorts:lNeedToReadSize;
		nSizeRead = fread(m_pdbData, sizeof(short), nSizeRead, m_pfProcessedSamps);
		if (nSizeRead <= 0) { // no data left or error
			break;
		}
		lNeedToReadSize -= nSizeRead;	// decrease
		_write(fhw, m_pdbData, nSizeRead*sizeof(short));
	}

ExitHere:
	if (pulStart && pulNumSamps) {
		*pulStart = lStartSampleGlobal;
		*pulNumSamps = lNumValidSamples;
	}
	return nReturn;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 来了一批新生成的采样值（其采样率已是“目标采样率”了！），对其做相应的能量检测和统计，……
// "samps" 新生成的一批采样的缓存起址
// "iSampStartGlobal" : 上述缓存中的第一个采样的全局序号
// "nNumSampsInBuffer" : 上述缓存中采样个数
// 
void CMainSegment::DetectInWindow(short *samps, long iSampStartGlobal, int nNumSampsInBuffer)
{
	// 新生成的采样数据都写入临时文件（"m_pfProcessedSamps" 所指文件），以便关键段确定之后直接从此文件读出关
	//		键段的采样值，并写入“关键段数据文件”
	// "m_pfProcessedSamps" 所指文件用于存放欠采样、声道合并（或选其一）之后的音频信号数据。它与原始音频数据
	//		对应的时长是一样的（但采样率已是“目标采样率”了）！！！
	fwrite(samps, sizeof(short), nNumSampsInBuffer, m_pfProcessedSamps);
	m_lNumOriginalSamples += nNumSampsInBuffer;

	int iSampCur;
	for (iSampCur=0; iSampCur<nNumSampsInBuffer; iSampCur++) {
		if (_Xn_Max < (samps[iSampCur] > 0 ? samps[iSampCur]:-(long)samps[iSampCur]) ) {
			_Xn_Max = samps[iSampCur] > 0 ? samps[iSampCur]:-(long)samps[iSampCur];
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	long lOffset;
	int kkIndexLB, mmIndexLB;	// "LB" : Local Buffer

	iSampCur = iSampStartGlobal;
	while (iSampCur < iSampStartGlobal+nNumSampsInBuffer) {	// 新来的数据还未处理完，继续
		while (iSampCur < iSampStartGlobal+nNumSampsInBuffer	// 缓存中的采样未处理完，。。。
			&& iSampCur < m_lStartSampIdxG_NextSkip	// 当前 skip 还未满，。。。
			) {
			m_dbValCurSkip += abs(samps[iSampCur-iSampStartGlobal]);	// 信号采样值的绝对值累加！！！
			iSampCur++;
		}
		if (iSampCur < m_lStartSampIdxG_NextSkip) {	// 新来的数据处理完了，但当前 skip 还不满，只能退出。
			break;
		}

// 当前 skip 已满，对已算出的 skip 值进行处理，。。。

		m_dbValCurSkip /= m_usNumSampsInSkip;

		// 求“以当前 skip 为头”的检测窗口的检测值存放的位置 "mmIndexLB" （指的是在 "m_pdbData" 缓冲区中的位置）
		lOffset = m_lCurSkipIdxG-m_lSkipIdxG_FirstValInBuffer;
		mmIndexLB = m_shFirstValIdxInBufferL+lOffset;
		if (mmIndexLB >= BufferSizeInDoubles) {// 越界了，折回（求模），ring buffer, 。。。
			mmIndexLB -= BufferSizeInDoubles; 
		}
		m_pdbData[mmIndexLB] = m_dbValCurSkip;	// 1st skip in the detection window
		for (kkIndexLB=m_shFirstValIdxInBufferL; kkIndexLB<m_shFirstValIdxInBufferL+lOffset; kkIndexLB++) {
			mmIndexLB = kkIndexLB;
			if (mmIndexLB >= BufferSizeInDoubles) {// 越界了，折回（求模），ring buffer, 。。。
				mmIndexLB -= BufferSizeInDoubles; 
			}
			// 凡包含当前 skip 的检测窗口（由 "mmIndexLB" 所指），都要把当前 skip 的值加到其检测值上
			m_pdbData[mmIndexLB] += m_dbValCurSkip;
		}

		if	(m_lCurSkipIdxG-m_lSkipIdxG_FirstValInBuffer+1 == DetectWinSizeInSkips) {
		// 检测窗口（其起始 skip 序号为 "m_lSkipIdxG_FirstValInBuffer"，其检测值存放位置
		// 为 "m_shFirstValIdxInBufferL"）的检测值求出来了，……
		// 也就是说，检测值缓存中的第一个检测值已经是完全的了（其余的检测值当然还不完全，越向右越差的远）
		// 注意：“缓存中的第一个检测值”并不一定是缓存中的第一个单元（缓存中的第一个单元的下标为 0 ！！！）
			// 只记最大检测值，及对应的检测窗的起始 skip 序号（/下标），。。。
			if (m_lSkipIdxG_MaxDetectVal == -1 || m_dbMaxDetectVal < m_pdbData[m_shFirstValIdxInBufferL]) {
				m_dbMaxDetectVal = m_pdbData[m_shFirstValIdxInBufferL];
				m_lSkipIdxG_MaxDetectVal = m_lSkipIdxG_FirstValInBuffer;
			}

			m_lSkipIdxG_FirstValInBuffer++;
			// 检测值缓存中的位置，。。。
			m_shFirstValIdxInBufferL++;
			if (m_shFirstValIdxInBufferL == BufferSizeInDoubles) {// 越界了，回零，。。。
				m_shFirstValIdxInBufferL = 0;
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// prepare to process the next skip
		m_lCurSkipIdxG++;	// skip 序号
		m_dbValCurSkip = 0.0;
		m_lStartSampIdxG_NextSkip += m_usNumSampsInSkip;	// 信号采样的全局下标（/序号）

	}	// all new data has been processed
}

