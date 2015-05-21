
#include "stdafx.h"	//yds
#include <assert.h>

#include <float.h>
#include <fstream>
#include <iostream>

#include "ppsvm.h"

using std::ifstream;
using std::ofstream;
using std::ios;

//yds(see file "ppsvm.h")
const double CSvmScale::lower = -1.0;
const double CSvmScale::upper = 1.0;
//

#ifdef __TRAINING_PHASE

CSvmScale::CSvmScale(int ndim)
{
    this->dim_ = ndim;
    mins_ = new double[ndim+1];
    maxs_ = new double[ndim+1];
    for (int ii = 0; ii<=ndim; ii++) {
        mins_[ii] = DBL_MAX;
        maxs_[ii] = -DBL_MAX;	//DBL_MIN;
    }
}

// 训练阶段才调用此函数
void CSvmScale::scaleDataSet(string src_file, string dst_file, string par_file)
{
    ifstream *src = new ifstream(src_file.c_str(), ios::in);
    if (!src)    return;
    ofstream *dst = new ofstream(dst_file.c_str(), ios::out);
    if (!dst)    return;
    ofstream *par = new ofstream(par_file.c_str(), ios::out);

    int idx, class_no;
    double val;
    string str;

    // find max and min numbers
    while (*src >> class_no) {
        for (int ii = 0; ii<dim_; ii++) {
            *src >> str;
            sscanf(str.c_str(), "%d:%lf", &idx, &val);
            if (val < mins_[idx]) mins_[idx] = val;
            if (val > maxs_[idx]) maxs_[idx] = val;
        }
    }

// output scaling parameters
    *par << "x" << std::endl;
    *par << lower << " " << upper << std::endl;
	for (int ii = 1; ii <= dim_; ii++) {
        *par << ii << " " << mins_[ii] << " " << maxs_[ii] << std::endl;
	}
//

// scale the training data
    src->clear();
    src->seekg(0);
    double res;
    while (*src >> class_no) {
        *dst << class_no;
        // features
        for (int i = 0; i<dim_; i++) {
            *src >> str;
            sscanf(str.c_str(), "%d:%lf", &idx, &val);
			if (val <= mins_[idx]) {
                res = lower;
			} else if (val >= maxs_[idx]) {
                res = upper;
			} else {
                res = lower + (upper - lower) * (val - mins_[idx]) / (maxs_[idx] - mins_[idx]);
			}
            *dst << " " << idx << ":" << res;
        }
        *dst << std::endl;
    }

    src->close();
    dst->close();
    par->close();
}

#else

// 当把分段程序做成 DLL 时，这种打开和读文件的方式就不能正常其作用（不知何故），所以换成一般方式
int CSvmScale::loadPar(const char* psfn)
{
//    ifstream *par = new ifstream(par_file.c_str(), ios::in);
//	if (par == NULL) return -1;
	FILE* pfr = fopen(psfn, "rt");
	if (pfr == NULL) return -1;

    dim_ = 0;
    char buf[1024];
	fgets(buf, sizeof(buf), pfr);	// "x"
	fgets(buf, sizeof(buf), pfr);	// "-1 1"
//    par->getline(buf, sizeof(buf));
//    par->getline(buf, sizeof(buf));
//	while (par->getline(buf, sizeof(buf))) {
	while (fgets(buf, sizeof(buf), pfr)) {
        dim_++;
	}
//    std::cout << dim << std::endl;

	assert(!mins_ && !maxs_);
    mins_ = new double[dim_+1];
    maxs_ = new double[dim_+1];

/*    par->clear();
    par->seekg(0);
    par->getline(buf, sizeof(buf));
    par->getline(buf, sizeof(buf));*/
	fseek(pfr, 0, SEEK_SET);
	fgets(buf, sizeof(buf), pfr);	// "x"
	fgets(buf, sizeof(buf), pfr);	// "-1 1"

    int idx;
    double min, max;
    for(int ii = 1; ii<=dim_; ii++) {
//        par->getline(buf, sizeof(buf));
		fgets(buf, sizeof(buf), pfr);
        sscanf(buf, "%d %lf %lf", &idx, &min, &max);
        mins_[idx] = min;
        maxs_[idx] = max;
    }

/*    par->close();
	delete par;*/
	fclose(pfr);

	return 0;
}

void CSvmScale::scaleFeature(struct svm_node *xxx)
{
	int idx;	// = x[ii].index;
	double min;	// = mins_[idx];
	double max;	// = maxs_[idx];
	double val;	// = x[ii].value;
    for (int ii = 0; xxx[ii].index != -1; ii++) {
		idx = xxx[ii].index;
		min = mins_[idx];
		max = maxs_[idx];
		val = xxx[ii].value;
		if (val <= min) {
            xxx[ii].value = lower;
		} else if (val >= max) {
            xxx[ii].value = upper;
		} else {
            xxx[ii].value = lower + (upper - lower) * (val - min) / (max - min);
		}
    }
}

#endif

CSvmScale::~CSvmScale()
{
	if (mins_)
		delete [] mins_;
	if (maxs_)
		delete [] maxs_;
}

