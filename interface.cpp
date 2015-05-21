

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

#include "parameters.h"
#include "interface.h"

#ifndef BUFFER_SIZE_IN_SAMPLES_DS
#define BUFFER_SIZE_IN_SAMPLES_DS	8192
#endif

int OpenFileWr(const char* psfn);

void CWavPreproc::CreateSampDataFile(const char* psfn)
{
	// 创建临时文件
	m_fhw = OpenFileWr(psfn);
	// 在输出文件头上保留空间，为保存“目标采样率”和“目标采样个数”2 个参数
	_lseek(m_fhw, sizeof(int)+sizeof(unsigned long), SEEK_SET);
//
	// total number of down-sampling samples made 
	m_nNumNewlyMadeSampsTotal = 0;
	// “原采样缓存”中第一个位置的采样点的全局下标
	m_iOriginalSampStartG = 0;
	// 本批生成的第一个欠采样点（即“欠采样缓存”中第一个采样点）的全局下标
	m_iTargetSampStartG = 0;

	short* ptmp = (short*)realloc(m_NewlyMadeSampsBuff, BUFFER_SIZE_IN_SAMPLES_DS*sizeof(short));
	if (ptmp) {
		m_NewlyMadeSampsBuff = ptmp;
	}
}

// "nNumOrigSamps" 原始采样个数，一个原始“采样”包含了一个采样时间点所有声道的数据
void CWavPreproc::DataCome(const unsigned char* pOrigSamps, int nNumOrigSamps)
{
// 注意：欠采样信号和原采样信号对应的时长是一样的！！！

	if (pOrigSamps == NULL || nNumOrigSamps <= 0) {// 表示数据已给完，。。。
		if (m_NewlyMadeSampsBuff) {
			free(m_NewlyMadeSampsBuff);
			m_NewlyMadeSampsBuff = NULL;
		}
//
		int sr = UNIFIED_SAMPLING_RATE;
		_lseek(m_fhw, 0, SEEK_SET);	// 文件头
		_write(m_fhw, &sr, sizeof(int));	// 目标采样率
		_write(m_fhw, &m_nNumNewlyMadeSampsTotal, sizeof(unsigned long));	// 目标采样个数
//
		_close(m_fhw);
		m_fhw = -1;
//
		free(m_NewlyMadeSampsBuff);
		m_NewlyMadeSampsBuff = NULL;
//
		printf("Length of clip in seconds : %f\n", (double)m_nNumNewlyMadeSampsTotal/UNIFIED_SAMPLING_RATE);

		return;
	}

// 处理新来的一批原始采样数据，。。。

	// 每来“一批”原采样数据，就生成一批欠采样。
	int nNumNewlyMadeSamps = 0;	// 本批（即从本次来的原始信号数据）生成的欠采样点个数

	if (m_iTargetSampStartG == 0) {
	// 无论如何做欠采样，第一个原始采样总是要直接拷贝的。也就是说，欠采样信号的第一个样本点就是原信号的第一个
	// 样本点！
		assert(nNumNewlyMadeSamps == 0);
		// 第一个采样值照搬

#ifdef __COMBINE_L_R_CHANNELS	// clrc_001
// 取各个声道的平均
		if (num_of_channels_ > 1) {
			long lval;	// 用 "long" 保证计算过程不溢出
			if (m_header.bit_p_sample == 16) {
				lval = *((short *)pOrigSamps);
				lval += *(((short *)pOrigSamps)+1);
			} else if (m_header.bit_p_sample == 8) {
				lval = *((char *)pOrigSamps);
				lval += *(((char *)pOrigSamps)+1);
				lval *= 256;
			}
			lval /= 2;
			m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] = lval;
		} else {// 只有一个声道时，。。。
			if (m_header.bit_p_sample == 16) {
				m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((short *)pOrigSamps);
			} else if (m_header.bit_p_sample == 8) {
				m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((char *)pOrigSamps);
				m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] *= 256;
			}
		}
#else
// 只取第一个声道
		if (bit_p_sample_ == 16) {
			m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((short *)pOrigSamps);	// 第一个采样值照搬
		} else if (bit_p_sample_ == 8) {
			m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((char *)pOrigSamps);	// 第一个采样值照搬
			m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] *= 256;
		}
#endif	// clrc_001
		nNumNewlyMadeSamps++;
	}

	//（二）生成对应的新采样，。。。
	while (1) {
	// 核心是将欠采样点对应到某个原采样点（只有左采样点），或两个相邻的原采样点（即左右两个采样点）之间！！！

		long iTargetSampG, iOriginalSampG;
		// 根据欠采样点（目标采样点）下标，。。。
		iTargetSampG = m_iTargetSampStartG+nNumNewlyMadeSamps;
		// 计算其对应的原采样点下标
		iOriginalSampG = (double)iTargetSampG*sample_rate_/UNIFIED_SAMPLING_RATE+0.5;
		if (iOriginalSampG > m_iOriginalSampStartG+nNumOrigSamps-1) {
		// 还未读到“目标原采样点”，。。。
			m_iOriginalSampStartG += nNumOrigSamps;
			break;
		}
		// 目标原采样点在缓存中的位置
		const unsigned char *puchar = pOrigSamps+(byte_p_sample_)*(iOriginalSampG-m_iOriginalSampStartG);
#ifdef __COMBINE_L_R_CHANNELS	// clrc_002
// 取各个声道的平均
		if (num_of_channels_ > 1) {
			long lval;
			if (m_header.bit_p_sample == 16) {// 16 位采样
				lval = *((short *)puchar);	// 本采样的第一个声道
				lval += *(((short *)puchar)+1);	// 本采样的第二个声道
			} else if (m_header.bit_p_sample == 8) {// 8 位采样
				lval = *((char *)(puchar));	// 本采样的第一个声道
				lval += *(((char *)puchar)+1);	// 本采样的第二个声道
				lval *= 256;
			}
			lval /= 2;
			m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] = lval;
		} else {// 只有一个声道时，。。。
			if (m_header.bit_p_sample == 16) {
				m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((short *)puchar);
			} else if (m_header.bit_p_sample == 8) {
				m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((char *)puchar);
				m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] *= 256;
			}
		}
#else
// 只取第一个声道
		if (bit_p_sample_ == 16) {
			m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((short *)puchar);
		} else if (bit_p_sample_ == 8) {
			m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((char *)puchar);
			m_NewlyMadeSampsBuff[nNumNewlyMadeSamps] *= 256;
		}
#endif	// clrc_002
		nNumNewlyMadeSamps++;	// 本批欠采样个数计数
			
	}	// end of "while (1) {"

	//（三）将本批新生成的采样（经“欠采样”和声道合并后的采样数据）写入输出文件
	if (_write(m_fhw, m_NewlyMadeSampsBuff, nNumNewlyMadeSamps*sizeof(short)) != nNumNewlyMadeSamps*sizeof(short)) {
		printf("Error writing temp sample data file !\n");
	}

	//（四）准备从音乐文件读入下一批原采样数据，。。。
	m_iTargetSampStartG += nNumNewlyMadeSamps;
	m_nNumNewlyMadeSampsTotal += nNumNewlyMadeSamps;

/*
	if (pOrigSamps)
		free(pOrigSamps);
*/
}

// 构造函数
CWavPreproc::CWavPreproc()
{
	m_fhw = -1;
	m_NewlyMadeSampsBuff = NULL;	// 必须的！！！
//
	assert(bit_p_sample_ == 16 || bit_p_sample_ == 8);
	// "m_header.sample_rate" : 原始采样频率
	// 原始采样率必须高于或等于 "UNIFIED_SAMPLING_RATE"（此即目标采样率！！！）
	if (sample_rate_ < UNIFIED_SAMPLING_RATE) {
		printf("Original sampling rate(%d) less than target !\n", sample_rate_);
	}
}

CWavPreproc::~CWavPreproc()
{
	if (m_NewlyMadeSampsBuff)
		free(m_NewlyMadeSampsBuff);
}

