
#ifndef	__MY_SVM_INTERFACE__
#define	__MY_SVM_INTERFACE__

#include <iostream>	// 如不加此头文件，下面的 std 就引起报错
using namespace std;

#include "ppsvm/svm.h"
#include "ppsvm/ppsvm.h"

class CSvmInterface {
private:
	const char* m_pInRootPath;
	const char* m_pOutRootPath;

private:
//	void fileFormatTransform(const char* psfn_original, int ndim);

#ifdef __TRAINING_PHASE
private:
	struct svm_parameter m_param;	// set by ParseConfigFile
	struct svm_problem m_prob;		// set by ReadProblem()
	struct svm_model *m_pModel;
	struct svm_node *mx_space;

private:
	void pp_svm_train(const char *input_file_name, const char *model_file_name);
	void ReadProblem(const char *filename);
	void DoCrossValidation();
	void ParseConfigFile(const char *svm_config_file);

public:
	bool ppsvmTrain(const char *psfn_trainingData, int ndim);

#else
private:
	struct svm_node *m_pSvmNodes;

	CSvmPredict* m_pPredictor;	// = new CSvmPredict();
	CSvmScale* m_pScaler;	// = new CSvmScale();	//yds : remove parameter "ndims"

public:
	int LoadSvmModel(int ndims);
	int ppsvmPredict(const double *pdbfeature, int ndim);

#endif	// __TRAINING_PHASE

private:

public:

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

public:
	CSvmInterface(const char* psIn, const char* psOut);
	virtual ~CSvmInterface();
};

#endif	// #ifndef	__MY_SVM_INTERFACE__
