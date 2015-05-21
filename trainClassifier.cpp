

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
#include <direct.h>

#include <conio.h>	// for "_getch()"

#include "parameters.h"
#include "procobj.h"
#include "fft\fft.h"

#ifdef __TRAINING_PHASE

#include "svmInterface.h"

int WriteVectorToTrainingDataFile(const double *pdbvector, int nNumDims, int nClassNo, FILE *pfw_tr);

//bool ppsvmTrain(const char *psfn_trainingData, int num_dims);
//void ppsvmDeleteTrainingDataFiles();

#include <TCHAR.H>

// 训练分类器，。。。
// 输入：训练样本列表文件
// 打开训练样本列表文件，处理各个样本，。。。
// "psfn_examps" 训练样本（音频文件）列表文件名（不带路径！）
int CProcessingObj::TrainClassifier(const char* psfn_examps)
{
	m_nNumFilesAll = 0;

	m_nNumInvalidExamples = 0;	// 信号不符合要求的样本个数
//
	// 创建训练样本数据文件
	sprintf(m_szfn, "%s%s", m_psOutPath, FN_TRAINING_DATA_ORGINAL);
	m_pfTrainingDataTxt = fopen(m_szfn, "wt");	// 创建文件
//
	// 打开训练样本“音频文件名（绝对路径） - 类别号”列表文件，其中每行表示一个训练样本
	sprintf(m_szfn, "%s%s", m_psInPath, psfn_examps);
	FILE* pfr_t = fopen(m_szfn, "rt");	// 读
	TCHAR* szline = Malloc(TCHAR, 512);
	TCHAR* psPos;
	while (1) {
		if (_fgetts(szline, 512, pfr_t) == NULL) {	break; }
		if (szline[0] == _T('\0')) { break; }
		// 找回车或换行符处截断，。。。
		psPos = szline;
		while (*psPos != _T('\0')) {
			if (*psPos == _T('\r') || *psPos == _T('\n')) { *psPos = _T('\0'); break; }
			psPos = _tcsinc(psPos);
		}
		// 约定文件名与类别号之间用 '\t' 隔开
		psPos = _tcschr(szline, _T('\t'));	// 1st occurrence of '\t'
		*psPos = _T('\0');
		psPos = _tcsinc(psPos);

		m_nNumFilesAll++;
		printf("\n\nTraining example No.%d\n", m_nNumFilesAll);
		if (ProcessExample(szline, atoi(psPos)) != 0) {

		}
	}
	fclose(pfr_t);
	free(szline);
//
	fclose(m_pfTrainingDataTxt);
	m_pfTrainingDataTxt = NULL;

	printf("\n无效样本个数：%d\n", m_nNumInvalidExamples);
//	fprintf(, "\n共 %d 个样本（其中含 %d 个无效样本）\n", m_nNumFilesAll, m_nNumInvalidExamples);

#ifdef __GENERATE_WEKA_TRAINING_FILE
	return 0;
#endif	// __GENERATE_WEKA_TRAINING_FILE
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
	CSvmInterface svm(m_psInPath, m_psOutPath);

// 训练分类器（训练结束后将分类器模型数据存入文件），。。。
	printf("\nBegin to train classifier, ...\n");
	sprintf(m_szfn, "%s%s", m_psOutPath, FN_TRAINING_DATA_ORGINAL);
	svm.ppsvmTrain(m_szfn, m_nNumDims);
//
	// 删除训练样本数据文件
	sprintf(m_szfn, "%s%s", m_psOutPath, FN_TRAINING_DATA_ORGINAL);
	remove(m_szfn);

	return 0;
}

#define NUM_SKIPS_LOCAL	10	// 放得下连续几帧（考虑帧间重叠）

// "fhr" 训练样本信号数据文件，。。。
// 从一个音频文件可能形成多个训练样例
int CProcessingObj::ProcessTrainingExample(int fhr)
{
	int nBufSizeShorts = NumSamplesInFrameSkipM*(NUM_SKIPS_LOCAL-1)+NumSamplesInFrameM;
	short* pBuffer = Malloc(short, nBufSizeShorts);
	if (pBuffer == NULL) {
		return -1;
	}
#ifdef _DEBUG
	int sr;
	unsigned long istart, num_samples;
	if (_read(fhr, &sr, sizeof(int)) != sizeof(int)) {
	}
	if (_read(fhr, &istart, sizeof(unsigned long)) != sizeof(unsigned long)) {
	}
	if (_read(fhr, &num_samples, sizeof(unsigned long)) != sizeof(unsigned long)) {
	}
#else
	_lseek(fhr, sizeof(int)+sizeof(unsigned long)+sizeof(unsigned long), SEEK_SET);
#endif

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	m_nNumFrames = 0;	// 当前样本（的当前单元）中已提取的帧数

	short* pCurFrame;
	int nShorts = 0;	// a must !
	int nBytesRead;
	while (1) {
		nBytesRead = _read(fhr, pBuffer+nShorts, (nBufSizeShorts-nShorts)*sizeof(short));
		if (nBytesRead <= 0) {// 采样数据文件中的数据读完了，则退出
			break;
		}
//
		nShorts += nBytesRead/sizeof(short);	// 缓存中采样（short）个数
		pCurFrame = pBuffer;
		while (nShorts >= NumSamplesInFrameM) {// 采样个数够一帧，。。。
			ProcSingleFrame(pCurFrame);
			m_nNumFrames++;
//
			if (m_nNumFrames == NumFrameSkipsInUnitM) {// 满一个单元了，。。。
				printf("Number of frames : %d\n", m_nNumFrames);
				// 此单元形成一个训练样例
				MakeAndWriteTrainingExample(true);
			}

			pCurFrame += NumSamplesInFrameSkipM;	// 帧跳一步
			nShorts -= NumSamplesInFrameSkipM;
		}
		if ((nBytesRead % sizeof(short)) != 0) {// 文件有错！！！
			printf("Invalid sample data !\n");
			break;
		}
//
		// 剩余的、不足一帧的采样数据移到缓存首
		memmove(pBuffer, pCurFrame, sizeof(short)*nShorts);
	}

	free(pBuffer);

	printf("Number of frames : %d\n", m_nNumFrames);
	if (m_nNumFrames < MINIMUM_NUM_FRAMES_IN_UNIT) {// 训练样本（或其最后一个单元）太短，则丢弃
		printf("Example too short to be used !");
		return 0;
	}
//
	// 处理最后一个单元并返回，。。。
	return MakeAndWriteTrainingExample(false);
}

int CProcessingObj::MakeAndWriteTrainingExample(bool bMove)
{
	int nReturn = 0;
	// 生成样本特征向量，。。。
	if (MakeFeatureVector() == 0) {
		// 将样本特征向量和样本类别号写入训练样本数据文件
		if (WriteVectorToTrainingDataFile(m_pvector, m_nNumDims, m_iClassNo) != 0) {
			printf("Error writing training example to file !\n");
			nReturn = -1;
		}
	} else {
		m_nNumInvalidExamples++;
		printf("Error generating training example feature vector !\n");
		nReturn = -2;
	}

	if (bMove) {// 单元之间的重叠区域，移到缓存首
		MoveUnitOverlap();
	}

	return nReturn;
}

// 将训练样本数据写成 SVM 分类器训练程序所需的格式，（写一个样本的数据），。。。
// "nClassNo" 该样本的类别编号，from 1 on, ...
// "nNumDims" 该样本特征向量维数
// 
int CProcessingObj::WriteVectorToTrainingDataFile(const double *pdbvector, int nNumDims, int nClassNo)
{
#ifdef __GENERATE_WEKA_TRAINING_FILE
	for (int idim=0; idim<nNumDims; idim++) {
		if (!_finite(pdbvector[idim])) {
			printf("dimension %d invalid value !\n", idim+1);
			_getch();
		}
		fprintf(m_pfTrainingDataTxt, "%g ", pdbvector[idim]);
	}
	fprintf(m_pfTrainingDataTxt, "%d\n", nClassNo);	// 输出训练样本的类别编号（1～）

#else
	fprintf(m_pfTrainingDataTxt, "%d ", nClassNo);	// 输出训练样本的类别编号（1～）
	// 先输出向量
	int idim;
	for (idim=0; idim<nNumDims-1; idim++) {// !!! subtract one !!!
		if (!_finite(pdbvector[idim])) {
			printf("dimension %d invalid value !\n", idim+1);
			_getch();
		}
		fprintf(m_pfTrainingDataTxt, "%d:%g ", idim+1, pdbvector[idim]);
	}
	if (!_finite(pdbvector[idim])) {
		printf("dimension %d invalid value !\n", idim+1);
		_getch();
	}
	fprintf(m_pfTrainingDataTxt, "%d:%g\n", idim+1, pdbvector[idim]);

#endif

	return 0;
}


#endif	// __TRAINING_PHASE

