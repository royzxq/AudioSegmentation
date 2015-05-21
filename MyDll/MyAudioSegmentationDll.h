
#ifndef __MY_AUDIO_SEGMENTATION_DLL_HHH__
#define __MY_AUDIO_SEGMENTATION_DLL_HHH__

// 下列 ifdef 块是创建使从 DLL 导出更简单的宏的标准方法。
// 此 DLL 中的所有文件都是用命令行上定义的 MY_AUDIO_SEGMENTATION_DLL_EXPORTS 符号编译的。
// 在使用此 DLL 的任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// MY_AUDIO_SEGMENTATION_DLL_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef MY_AUDIO_SEGMENTATION_DLL_EXPORTS
#define MY_AUDIO_SEGMENTATION_DLL_API __declspec(dllexport)
#else
#define MY_AUDIO_SEGMENTATION_DLL_API __declspec(dllimport)
#endif

#include "mydefs.h"	// 全局宏定义 - 条件编译

#ifndef __TRAINING_PHASE
#include "myVector.h"
#endif	// __TRAINING_PHASE

// "argv1" 输入文件根路径（分类器模型文件等在此），必须以“\”结尾
// "argv2" 输出文件根路径（临时文件等在此），必须以“\”结尾
MY_AUDIO_SEGMENTATION_DLL_API int InitDll(const char* argv1, const char* argv2);
#ifdef __TRAINING_PHASE
// "psfn_examps" 分类器训练样本（音频文件）列表文件名（仅文件名，不带路径！）
MY_AUDIO_SEGMENTATION_DLL_API int TrainClassifier(const char* psfn_examps);
#else
// "psfn" 待分段的音频文件（带绝对路径）
MY_AUDIO_SEGMENTATION_DLL_API int SegmentAudioPiece(const char* psfn, SEGMENT_VECTOR& segVector);
// "nClassNo" 类别编号（0 ～）
MY_AUDIO_SEGMENTATION_DLL_API const char* ClassLabel(int nClassNo);
#endif	// __TRAINING_PHASE
MY_AUDIO_SEGMENTATION_DLL_API void FreeDllMemory();

#endif	// __MY_AUDIO_SEGMENTATION_DLL_HHH__

//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

/*
// 此类是从 MyDll.dll 导出的
class MY_AUDIO_SEGMENTATION_DLL_API CMyDll {
public:
	CMyDll(void);
	// TODO: 在此添加您的方法。
};

extern MY_AUDIO_SEGMENTATION_DLL_API int nMyDll;

MY_AUDIO_SEGMENTATION_DLL_API int fnMyDll(void);
*/
