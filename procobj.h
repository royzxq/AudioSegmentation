
#ifndef	__MY_PROC_OBJ_H__
#define	__MY_PROC_OBJ_H__

#include <FLOAT.H>
#include "parameters.h"

#include "util\my_util.h"

#ifndef __TRAINING_PHASE
#include "myVector.h"
#include "interface.h"

typedef struct {
	int classNo;
	int numFrames;
} SEG_INFO_T;
typedef vector <SEG_INFO_T> SEG_VECTOR_L;

typedef struct {
	int iLL;
	int iRR;
} ZONE_INFO_T;
typedef vector <ZONE_INFO_T> ZONE_VECTOR;

typedef vector<int> SEG_END_SAMP_INDEX_VECTOR;

#endif	// __TRAINING_PHASE

typedef double BandSumArray_t[NUM_DIMS_AM_SORCE_DATA];
typedef double MagSpectArray_t[WIDTH_SOURCE_DATA_AM];

class CProcessingObj {
private:
	char m_szfn[260];
	CWFFile *m_pfWaveform;

private:
	unsigned long m_nNumValidFiles;

private:
	const char *m_psOutPath, *m_psInPath;	// 只读！！！
	int m_nNumFrames;	// 当前单元（unit）中的帧跳（frame skips）数计数
	BandSumArray_t* m_pBandSum;
	MagSpectArray_t *m_pSrcAM;
	cpxv_t* m_fdom;

	double* m_pdbMFCC;
	double* m_pdbAM;
	double* m_pLPC;
	double* m_pDataSTE;	// 短时（帧）信号值绝对值之平均值序列（对应一个单元 unit）

//
	double* m_pvector;
	double *m_pMeanMFCC, *m_pStdMFCC, *m_pMeanDeriMFCC, *m_pStdDeriMFCC;
	double *m_pMeanAM, *m_pStdAM, *m_pMeanDeriAM, *m_pStdDeriAM;
	double *m_pMeanLPC, *m_pStdLPC, *m_pMeanDeriLPC, *m_pStdDeriLPC;
//
	int m_nNumDims;

private:
	int CreateSampleDataFile(const char *psfn);
#ifdef __NORMALIZE_AUDIO_SAMPLES
	int NormalizeSamples(int maxAbs);
#endif
	void PrepareSrcAM();

#ifdef __TRAINING_PHASE
private:
	int m_iClassNo;
//
	unsigned long m_nNumFilesAll;
	FILE* m_pfTrainingDataTxt;
	int m_nNumInvalidExamples;	// 信号不符合要求的训练样本个数
private:
	int ProcessExample(const char *psfn, int iClassNo);
	int ProcessTrainingExample(int fhr);
	int WriteVectorToTrainingDataFile(const double *pdbvector, int nNumDims, int nClassNo);
	int MakeAndWriteTrainingExample(bool bMove);
public:
	int TrainClassifier(const char* psfn);
#else	// #ifdef __TRAINING_PHASE
private:
//	unsigned char* m_pClassNo;	// 存放各个单元的类别号
	SEG_VECTOR_L m_unitVector;

	int SegmentAudio(int fhr, SEGMENT_VECTOR& segVector);
	void JoinSameClassSegs(SEGMENT_VECTOR& segVector);
	void SaveSegInfoToFile(SEGMENT_VECTOR& segVector);

	int FinalSegProcessing(int fhr_samples, int&, int nNumSamplesLeft, SEGMENT_VECTOR& segVector);
	int FinePosition(int fhr_samples, SEG_INFO_T* pSegInfo, int nNumSegs);

	int SegmentAudio02(int fhr, std::vector<int>& segEndVector);
	int SegmentAudioHelper02(int fhr);

private:
	CWavPreproc* m_pWavInterface;
public:
	int ProcessExample(const char *psfn, SEGMENT_VECTOR& segVector);
// or else, ...
	int InitObj();
	int DataCome(const unsigned char*, int);
	int OverHaHa(SEGMENT_VECTOR& segVector);

private:
	FILE* m_pfSTE;	// 存放待分段音频的帧能量值的文件
#endif	// #ifdef __TRAINING_PHASE

private:
	void ProcSingleFrame(const short psamps[]);
	int MakeFeatureVector();
	void MakeFeatureVectorMFCC();
	void MakeFeatureVectorLPC();

	void MoveUnitOverlap();

#ifdef __USE_SPECTRAL_SHAPE_FEATURES
private:
//	double* m_ftPre;
private:
	double* m_pSpectralShape;
	double *m_pMeanSpectralShape, *m_pStdSpectralShape;

	void MakeFeatureVectorSpectralShape();

//	void CalcPre();
//	int CalcTimbre(double timbre[]);
#endif	// __USE_SPECTRAL_SHAPE_FEATURES

#ifdef __USE_STE_FEATURES
private:
	double *m_pFeatureSTE;
	void MakeFeatureVectorSTE();
#endif	// __USE_STE_FEATURES

#ifdef __USE_BEAT_HISTOGRAM
private:
	double* m_pFeatureBH;
	void MakeOnsets(double onset[]);
	void MakeFeatureVectorBH();
	void MakeFeatureVectorAM();
#endif	// __USE_BEAT_HISTOGRAM

#ifdef __USE_MODULATION_SPECTRUM
private:
	double* m_pFeatureMSP;
	void MakeFeatureVectorMSP();
#endif	// __USE_MODULATION_SPECTRUM

#ifdef __USE_ZERO_CROSSING
private:
	double* m_pZC;
	double* m_pFeatureZC;
	void MakeFeatureVectorZC();
#endif	// __USE_ZERO_CROSSING
#ifdef __USE_SUB_BAND_ENERGY
private:
	double* m_pFeatureSBE;
	void MakeFeatureVectorSBE();
#endif

/*	int m_nNumTmpVectors;
	int* m_pNumFrames;
	int NumTmpVectors();
	int NumTmpVectorsToCompute(int iframe);*/

public:
	int MirexTrainTest();

public:
	void SetPath(const char *psInPath, const char *psOutPath) { m_psInPath = psInPath; m_psOutPath = psOutPath; }

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

private:
	void ReleaseMemory();
public:
	CProcessingObj();
	virtual ~CProcessingObj();
};

#endif
// #ifndef	__MY_PROC_OBJ_H__
