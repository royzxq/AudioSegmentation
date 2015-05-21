// AudioSegmentation02.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include <io.h>	// for "_access(...)"
//#include <errno.h>
//#include <direct.h>

//#include <conio.h>	// for "_getch()"

//#include <ctype.h>

//#include <time.h>
//#include <sys/types.h>
//#include <sys/timeb.h>
//#include <math.h> 
#include <assert.h>

#include "MyAudioSegmentationDll.h"

int _tmain(int argc, _TCHAR* argv[])
{
#ifdef __TRAINING_PHASE
// 分类器训练过程，3 个参数：程序名、输入文件目录、输出文件目录
	int nNumParameters = 3;
#else
// 音频分类（按类分段）过程，4 个参数：程序名、输入文件目录、输出文件目录、待分段音频文件名（带全路径）
	int nNumParameters = 4;
#endif	// __TRAINING_PHASE
	if (argc != nNumParameters) {
		printf("%d command line parameters, less than %d !\n", argc, nNumParameters);
		return -1;
	}
	if (_access(argv[1], 0) != 0) {
		printf("Path %s does not exist !\n", argv[1]);
		return -1;
	}
	if (_access(argv[2], 0) != 0) {
		printf("Path %s does not exist !\n", argv[2]);
		return -1;
	}
#ifndef __TRAINING_PHASE
	if (_access(argv[3], 0) != 0) {// 待分段的音频文件（绝对路径）
		printf("File %s does not exist !\n", argv[3]);
		return -1;
	}
#endif	// __TRAINING_PHASE

	InitDll(argv[1], argv[2]);
#ifdef __TRAINING_PHASE
	// "FN_TRAINING_EXAMPLES_LIST" 训练样本（音频文件）列表文件名（仅文件名，不带路径！）
	TrainClassifier(FN_TRAINING_EXAMPLES_LIST);
#else
	// 存放分段信息的向量
	SEGMENT_VECTOR segVector;
//
	// "argv[3]" 待分段的音频文件名（绝对路径）
	SegmentAudioPiece(argv[3], segVector);

	// 后处理，。。。

	segVector.clear();

#endif	// __TRAINING_PHASE
	FreeDllMemory();	// 释放内存

	return 0;
}



