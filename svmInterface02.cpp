

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
#include <string>
#include <iostream>
#include <fstream>

#include "parameters.h"
#include "svmInterface.h"


#ifdef __TRAINING_PHASE

#else	// #ifdef __TRAINING_PHASE
//

int CSvmInterface::LoadSvmModel(int ndims)
{
    // 要求装入分类模型文件（以前训练得来的）
	// 如果此前装入过模型，则会先删掉以前的模型，再装新的
	string fname(m_pInRootPath);	// 输入文件目录（约定模型文件在此目录下）
	fname += FN_MODEL_BUILT;
	if (!m_pPredictor->LoadModel(fname.c_str())) {
		cerr << "Fail to load svm model !" << endl;
		return -1;
	}
	if (m_pPredictor->ModelPtr()->param.tag[2]) {// 装入特征向量归一化参数（分类模型训练时做了特征向量数据归一化），。。。
		fname = m_pInRootPath;	// 输入文件目录（约定特征向量归一化参数文件在此目录下）
		fname += FN_SCALE_PARAMS;
		if (m_pScaler->loadPar(fname.c_str()) != 0) {
			cerr << "Fail to load scaling parameters !" << endl;
			return -2;
		}
		if (m_pScaler->NumDims() != ndims) {// 不一致，说明程序有逻辑错误！！！
			printf("m_pScaler->NumDims() != ndims !\n");
			return -3;
		}
	}

	// reallocate memory for the predictor(see the following code)
	struct svm_node *pSvmNodes = (struct svm_node*)realloc(m_pSvmNodes, sizeof(struct svm_node)*(ndims*2+2));
	if (pSvmNodes == NULL) {
		printf("pSvmNodes == NULL, abort !\n");
		return -2;
	}
	m_pSvmNodes = pSvmNodes;

	return 0;
}


// 输入参数为特征向量，以及特征维数
// SVM 分类模型文件中包含了 SVM 分类算法需要的所有参数（训练模型时确定的）
int CSvmInterface::ppsvmPredict(const double *pdbfeature, int ndim)
{
    // prepare a svm type feature
    int ii;
    for (ii = 0; ii<ndim; ii++) {
		m_pSvmNodes[ii].index = ii+1;
		m_pSvmNodes[ii].value = pdbfeature[ii];
    }
    m_pSvmNodes[ii].index = -1;	// 表示数据结束
    m_pSvmNodes[ii].value = 0;
    
    if (m_pPredictor->ModelPtr()->param.tag[2]) {// scale the feature
		m_pScaler->scaleFeature(m_pSvmNodes);
    }
    // predict from the feature
    int res = (int)m_pPredictor->predict(m_pSvmNodes);

    return res;
}


//
#endif	// #ifdef __TRAINING_PHASE

void exit_with_help()
{
    fprintf(stderr, "libsvm error!\n");
	exit(1);
}

void exit_input_error(int line_num)
{
	fprintf(stderr, "Wrong input format at line %d\n", line_num);
	exit(1);
}

/*
// file format transform
// 我是参考这个函数，直接生成的 SVM 训练程序所需的文件，故我不需要这个函数了
// 
void CSvmInterface::fileFormatTransform(const char* psfn_original, int ndim)
{
// original data file, ...
    ifstream *fOriginal = new ifstream(psfn_original, ios::in);
    if (fOriginal == NULL) {
        cerr << "data file " << psfn_original << " open error" << endl;
        return ;
    }

// 新创建文件：libsvm format data file, ...
//yds
	string str01(m_pOutRootPath);
	str01 += FN_TRAINING_DATA;
    ofstream *fsvm = new ofstream(str01.c_str(), ios::out);
    if (fsvm == NULL) {
        cerr << "file " << str01 << " create error" << endl;
        return ;
    }
//
    double *features = new double[ndim+1];
    int class_no;
    int idx;
    while (*fOriginal >> features[1]) {
	// read a feature vector
        for (idx = 2; idx <= ndim; idx++) {
            *fOriginal >> features[idx];
        }
        *fOriginal >> class_no;

	// write a feature vector
        *fsvm << class_no;	// 先输出类别编号（或其他标注值）
        for (idx = 1; idx <= ndim; idx++) {
            *fsvm << " " << idx << ":" << features[idx];	// 如此输出，据说是为了能表示“空值”（即冒号后可无值）
        }
        *fsvm << endl;
    }
	delete []features;

    fsvm->close();
    fOriginal->close();
}
*/

/*
#include "stdafx.h"	//yds

#include <iostream>
#include <fstream>
#include <string>

#include <io.h>	//yds
#include <direct.h>	//yds, for _mkdir()
#include <assert.h>	//yds

using std::string;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::ios;

#include "mydefs.h"	//yds
#include "ppsvm.h"

// fuctions
// 1. transform data.txt --> libsvm file format
// 2. scale the data to [lower, upper], optional
// 3. train the model using libsvm and save the model
// 4. predict the input feature using the trained model


#ifdef __TRAINING_PHASE

#else

#endif	// #ifndef __TRAINING_PHASE


// 创建子目录，供 SVM 程序存放生成的文件
// 
int ppsvmMakeDir()
{
	int nReturn = 0;
	char *psdir = (char *)malloc(260);
	sprintf(psdir, "%s%s", PTR_ROOT_PATH_STR, SUB_DIR_SVM);
	if (_access(psdir, 0) != 0) {// not 
		nReturn = _mkdir(psdir);
		if (nReturn != 0) {
			printf("Can't create sub dir %s\n", psdir);
		}
	}
	free(psdir);
	if (nReturn != 0) {
		return -1;
	}
	return 0;
}

void ppsvmDeleteFilesAndDir()
{
	char *psfn = Malloc(char, 260);
	sprintf(psfn, "%s%s", PTR_ROOT_PATH_STR, SUB_DIR_SVM);
	if (_access(psfn, 0) != 0) {// sub dir deosn't exist, ...
		free(psfn);
		return;
	}
	int nlen = strlen(psfn);

	strcat(psfn, FN_TRAINING_DATA);
	remove(psfn);

	psfn[nlen] = '\0';
	strcat(psfn, FN_SCALED_DATA);
	remove(psfn);
//
	psfn[nlen] = '\0';
	strcat(psfn, FN_SCALE_PARAMS);
	remove(psfn);

	psfn[nlen] = '\0';
	strcat(psfn, FN_MODEL_BUILT);
	remove(psfn);

	psfn[nlen] = '\0';
	if (_rmdir(psfn) !=0) {
		printf("Can't delete dir %s\n", psfn);
	}

	free(psfn);
}

*/

