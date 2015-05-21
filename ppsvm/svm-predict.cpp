
#include "stdafx.h"	//yds

#include "ppsvm.h"

#ifndef __TRAINING_PHASE

CSvmPredict::CSvmPredict()
{
	pmodel_ = 0;
}

CSvmPredict::~CSvmPredict()
{
	if (pmodel_) {
        svmFreeAndDestroyModel(&pmodel_);
	}
}

// 在 CSvmPredict 类对象生存期间，可能多次装入不同模型（如外部控制的交叉验证——训练+测试）
bool CSvmPredict::LoadModel(const char *model_file)
{
	if (pmodel_) {
        svmFreeAndDestroyModel(&pmodel_);
	}
//
    pmodel_ = svmLoadModel(model_file);
	if (pmodel_ == NULL) {
        return false;
	}

    return true;
}

double CSvmPredict::predict(struct svm_node *x)
{
    return svm_predict(pmodel_, x);
}

#endif
