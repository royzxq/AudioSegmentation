

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

int OpenFileRd(const char* psfn);

#ifndef __TRAINING_PHASE

// 接口函数（1），仅被调用一次！！！
int CProcessingObj::InitObj()
{
	 if (m_pWavInterface == NULL) { 
		m_pWavInterface = new CWavPreproc();
	 }
	 if (m_pWavInterface == NULL) {
		 return -1;
	 }

// 创建文件（临时文件），保存目标音频采样数据

	sprintf(m_szfn, "%s%s", m_psOutPath, FN_TEMP_WAVEFORM);
	m_pWavInterface->CreateSampDataFile(m_szfn);

	return 0;
}

// 接口函数（2），被连续多次调用！！！
int CProcessingObj::DataCome(const unsigned char* psamps, int numSamps)
{
	if (m_pWavInterface == NULL) return -1;
//
	m_pWavInterface->DataCome(psamps, numSamps);

	return 0;
}

// 接口函数（3），仅被调用一次
int CProcessingObj::OverHaHa(SEGMENT_VECTOR& segVector)
{
	if (m_pWavInterface == NULL) return -1;
//
	m_pWavInterface->DataCome(NULL, 0);	// 通知结束！！！

// 打开目标信号采样数据文件（临时文件），为读数据，。。。

	sprintf(m_szfn, "%s%s", m_psOutPath, FN_TEMP_WAVEFORM);
	int fhr = OpenFileRd(m_szfn);
	if (SegmentAudio(fhr, segVector) != 0) {
	}
	_close(fhr);

// 删除文件（临时文件），。。。
	sprintf(m_szfn, "%s%s", m_psOutPath, FN_TEMP_WAVEFORM);
	remove(m_szfn);

	return 0;
}

#endif	// #ifndef __TRAINING_PHASE
