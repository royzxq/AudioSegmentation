
#include "stdafx.h"

#include <assert.h>
#include <FLOAT.H>	// for DBL_MAX

#include <math.h>
#include <stdio.h>

#include <stdio.h>
#include <malloc.h>

#include <string.h>

#include "mydefs.h"

#include "trainingExample.h"


CTrainingExample::CTrainingExample()
{

}

CTrainingExample::~CTrainingExample()
{

}

bool CTrainingExample::IsCreatedOK()
{
	return true;
}

// 一批新采样点到了，。。。
void CTrainingExample::SamplesComing(short *samps, long iSampStartGlobal, int nNumSampsInBuffer)
{
	
}

int CTrainingExample::CalcVector()
{

	return 0;
}
