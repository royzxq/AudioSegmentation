
#ifndef	__CTRAINING_EXAMPLE_H__
#define __CTRAINING_EXAMPLE_H__

#include <stdio.h>	// file

// 为了检测音频信号中的关键段（平均能量最大的段）

class CTrainingExample {
private:

private:

public:
	void SamplesComing(short *samps, long iSampStartGlobal, int nNumSampsInBuffer);
	int CalcVector();
	bool IsCreatedOK();

public:
	CTrainingExample();
	virtual ~CTrainingExample();
};

#endif	// __CTRAINING_EXAMPLE_H__
