
#include <string>

#include "mydefs.h"
#include "svm.h"

using std::string;

#ifndef __TRAINING_PHASE

class CSvmPredict {
public:
    CSvmPredict();
    ~CSvmPredict();

    /* predict a feature vector */
    double predict(struct svm_node *x);

    /* read a model from a model file(generated by svm-train) */
    bool LoadModel(const char *model_file);
	const struct svm_model *ModelPtr() const { return pmodel_; }

protected:
    /* store the model */
    struct svm_model *pmodel_;
};

#endif

/* implement the functions in svm-scale.c */
class CSvmScale
{
public:

#ifdef __TRAINING_PHASE
    CSvmScale(int ndim);
    void scaleDataSet(string src_file, string dst_file, string par_file);
#else
    CSvmScale() { mins_ = NULL; maxs_ = NULL; dim_ = 0; };
	int loadPar(const char* psfn);
    void scaleFeature(struct svm_node *x);
#endif
    ~CSvmScale();

protected:
    // dimension of feature vector
    int dim_;
    // min value and max value of features
    double * mins_;
    double * maxs_;

public:
	int NumDims() const { return dim_; }

public:
    // others
//yds
    const static double lower;	// = -1.0;
    const static double upper;	// = 1.0;
//
};
