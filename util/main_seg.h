
#ifndef	__CMAIN_SEG_Q__
#define __CMAIN_SEG_Q__

#include <stdio.h>	// file

// 为了检测音频信号中的关键段（平均能量最大的段）

class CMainSegment {
private:
	// 检测窗口跳步的大小，固定为 30 毫秒时长对应的信号采样个数，与“目标采样率”有关！
	unsigned short m_usNumSampsInSkip;

	unsigned short _Xn_Max;	// 当前歌曲中信号值绝对值最大值

private:
	// 此文件用于存放欠采样、声道合并（或选其一）之后的音频信号数据。与原始音频数据对应的时长一样！！！
	FILE *m_pfProcessedSamps;

	// buffer to hold signal sample data
	double *m_pdbData;

	// "LB" : Local Buffer; "GB" : Global Buffer

	// 音乐文件由若干 skips 构成（头尾相接，不重叠），skip 序号从 0 开始
	// 每个 skip 都对应一个检测窗口（以该 skip 打头的检测窗口，以该 skip 的序号作为标识），每个检测窗口对应一个检测值
	// 缓冲区 "m_pdbData" 中的“第一个检测值”对应的 skip 序号（序号从 0 开始）（指以该 skip 开始的检测窗口的检测值！）
	long m_lSkipIdxG_FirstValInBuffer;
	// 缓冲区 "m_pdbData" 中的“第一个检测值”的下标（从 0 开始，相对于缓冲区的起始位置）
	short m_shFirstValIdxInBufferL;

	long m_lCurSkipIdxG;
	double	m_dbValCurSkip;

	// 下一个 skip 的起始采样的下标
	unsigned long m_lStartSampIdxG_NextSkip;

	// 原始采样个数（经“欠”后）
	unsigned long m_lNumOriginalSamples;

	// 记录最佳段的位置（以 skip 序号表示）和检测值
	long m_lSkipIdxG_MaxDetectVal;
	double m_dbMaxDetectVal;

private:
	unsigned short m_usMaxMainSegSizeInSkips;

public:
	void SetMaxMainSegSize(unsigned short usMaxMainSegSize) { m_usMaxMainSegSizeInSkips = usMaxMainSegSize; }

public:
	// 每求得一批欠采样数据后，就调用此函数
	void DetectInWindow(short *samps, long iLowSampStart, int nNumLowSamps);

//	void StartDetection();
//	void DetectionEnd();
	int WriteMainSegToFile(int fhw, unsigned long *pulStart, unsigned long *pulNumSamps);

public:
	CMainSegment();
	virtual ~CMainSegment();

	bool IsCreatedOK() { return (m_pdbData != 0 && m_pfProcessedSamps != 0); }
};

#endif	// __CMAIN_SEG_Q__
