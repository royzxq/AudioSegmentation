// MyDll.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <assert.h>

#include "MyAudioSegmentationDll.h"

#include "procobj.h"

const char *g_psOutRootPath = 0;
const char *g_psInRootPath = 0;
static FILE *g_pfout = NULL;

static CProcessingObj* g_pMainObj = NULL;

#ifdef __TRAINING_PHASE

#include <malloc.h>

#else	// __TRAINING_PHASE

#include <map>
using namespace std;
// 类下标与类名对照表
typedef map<int, string, less<int>> ClassNo_LABEL_MAP;
typedef pair <int, string> IntStringPairT;
// 类下标与类名对照表
static ClassNo_LABEL_MAP g_ClassNoLabelMap;
FILE* g_pfwInfo = NULL;
#endif	// __TRAINING_PHASE

// 只能被调一次，如 DLL 装入时
MY_AUDIO_SEGMENTATION_DLL_API int InitDll(const char* argv1, const char* argv2)
{
	if (g_pMainObj != NULL) {
		printf("Doing nothing, g_pMainObj already exists!\n");
		return 1;
	}

	char* psfn;
#ifndef __TRAINING_PHASE
	// 类别下标和类别名对照表，。。。
	g_ClassNoLabelMap.clear();
	g_ClassNoLabelMap.insert(IntStringPairT(0, "静音段"));
	g_ClassNoLabelMap.insert(IntStringPairT(1, "音乐段"));
	g_ClassNoLabelMap.insert(IntStringPairT(2, "语音段"));
	g_ClassNoLabelMap.insert(IntStringPairT(3, "混合段"));
	g_ClassNoLabelMap.insert(IntStringPairT(4, "其他段"));

	// 创建分段信息文件名（绝对路径）
	psfn = Malloc(char, 260);
	sprintf(psfn, "%s%s", argv2, "seg_info.txt");
	g_pfwInfo = fopen(psfn, "wt");
	assert(g_pfwInfo);
	free(psfn);
	psfn = NULL;
//
#endif	// __TRAINING_PHASE

	g_psOutRootPath = _strdup(argv2);	// 拷贝
	g_psInRootPath = _strdup(argv1);	// 拷贝

	// 创建输出信息文件
	psfn = Malloc(char, 256);
	sprintf(psfn, "%s%s", argv2, "informtion.txt");
	g_pfout = fopen(psfn, "wt");
	assert(g_pfout);
	free(psfn);
	psfn = NULL;
//

	g_pMainObj = new CProcessingObj();
	if (g_pMainObj == NULL) {
		printf("Can't create CProcessingObj object !\n");
		return -2;
	}

	g_pMainObj->SetPath(argv1, argv2);	// 注意：两个（参数）路径串末尾必须有 "/" ！
	return 0;
}

#ifdef __TRAINING_PHASE
MY_AUDIO_SEGMENTATION_DLL_API int TrainClassifier(const char* psfn_examps)
{
	if (g_pMainObj == NULL) {
		printf("g_pMainObj == NULL !\n");
		return -1;
	}

	// "psfn_examps" 训练样本（音频文件）列表文件名（不带路径！）
	g_pMainObj->TrainClassifier(psfn_examps);
	return 0;
}
#else
MY_AUDIO_SEGMENTATION_DLL_API int SegmentAudioPiece(const char* psfn, SEGMENT_VECTOR& segVector)
{
	if (g_pMainObj == NULL) {
		printf("g_pMainObj == NULL !\n");
		return -1;
	}

	if (g_pfwInfo) {
		fprintf(g_pfwInfo, "\n+++\nAudio File : %s\n\n", psfn);
	}
	g_pMainObj->ProcessExample(psfn, segVector);	// "psfn" 待分段的音频文件名（绝对路径）
	return 0;
}
MY_AUDIO_SEGMENTATION_DLL_API const char* ClassLabel(int nClassNo)
{
	ClassNo_LABEL_MAP::const_iterator iter_x = g_ClassNoLabelMap.find(nClassNo);
	if (iter_x == g_ClassNoLabelMap.end()) return NULL;
	return iter_x->second.c_str();
}
#endif	// __TRAINING_PHASE

// 只能被调一次，如 DLL 卸载时
MY_AUDIO_SEGMENTATION_DLL_API void FreeDllMemory()
{
	if (g_pMainObj) {
		delete g_pMainObj;
		g_pMainObj = NULL;
	}

#ifndef __TRAINING_PHASE
	g_ClassNoLabelMap.clear();
//
	if (g_pfwInfo) {
		fclose(g_pfwInfo);
		g_pfwInfo = NULL;
	}
#endif	// __TRAINING_PHASE

	if (g_pfout) {
		fclose(g_pfout);
		g_pfout = NULL;
	}

// ///////////////////////////////////

	if (g_psOutRootPath) {
		free((void *)g_psOutRootPath);
		g_psOutRootPath = NULL;
	}
	if (g_psInRootPath) {
		free((void *)g_psInRootPath);
		g_psInRootPath = NULL;
	}
}



/*
// 这是导出变量的一个示例
MY_AUDIO_SEGMENTATION_DLL_API int nMyDll=0;

// 这是导出函数的一个示例。
MY_AUDIO_SEGMENTATION_DLL_API int fnMyDll(void)
{
	return 42;
}

// 这是已导出类的构造函数。
// 有关类定义的信息，请参阅 MyDll.h
CMyDll::CMyDll()
{
	return;
}
*/
