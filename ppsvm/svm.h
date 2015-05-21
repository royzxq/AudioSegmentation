#ifndef _LIBSVM_H
#define _LIBSVM_H

#define LIBSVM_VERSION 311

#ifdef __cplusplus
extern "C" {
#endif

extern int libsvm_version;

#define FN_SVM_CONFIG	"ppsvm.config"

// 归一化训练数据文件
#define FN_SCALED_DATA	"~f001"
/*
// 训练数据文件
#define FN_TRAINING_DATA	"~f002"
*/
// 训练数据归一化参数文件（用于对待识样本的特征数据进行归一化，识别前）
#define FN_SCALE_PARAMS	"~f009"
// 训练出的分类模型数据文件
#define FN_MODEL_BUILT	"~f00a"

struct svm_node {
	int index;
	double value;
};

struct svm_problem {
	int ll;
	double *pdbyy;
	struct svm_node **ppx;
};

enum { C_SVC, NU_SVC, ONE_CLASS, EPSILON_SVR, NU_SVR };	/* svm_type */
enum { LINEAR, POLY, RBF, SIGMOID, PRECOMPUTED }; /* kernel_type */

struct svm_parameter {
	int svm_type;
	int kernel_type;
	int degree;	/* for poly */
	double gamma;	/* for poly/rbf/sigmoid */
	double coef0;	/* for poly/sigmoid */

	/* these are for training only */
	double cache_size; /* in MB */
	double eps;	/* stopping criteria */
	double C;	/* for C_SVC, EPSILON_SVR and NU_SVR */
	int nr_weight;		/* for C_SVC */
	int *weight_label;	/* for C_SVC */
	double* weight;		/* for C_SVC */
	double nu;	/* for NU_SVC, ONE_CLASS, and NU_SVR */
	double p;	/* for EPSILON_SVR */
	int shrinking;	/* use the shrinking heuristics */
	int probability; /* do probability estimates */
	unsigned char tag[4];
};

// svm_model
struct svm_model
{
	struct svm_parameter param;	/* parameter */
	int nr_class;		/* number of classes, = 2 in regression/one class svm */
	int num_totalSVs;			/* total #SV */
	struct svm_node **SV;		/* SVs (SV[l]) */
	double **sv_coef;	/* coefficients for SVs in decision functions (sv_coef[k-1][l]) */
	double *rho;		/* constants in decision functions (rho[k*(k-1)/2]) */
	double *probA;		/* pariwise probability information */
	double *probB;

	/* for classification only */

	int *label;		/* label of each class (label[k]) */
	int *nSV;		/* number of SVs for each class (nSV[k]) */
				/* nSV[0] + nSV[1] + ... + nSV[k-1] = l */
	/* XXX */
	int free_sv;	// 1 if model is created by svmLoadModel(...), 0 if model is created by svm_train(...)
};

struct svm_model *SvmTrain(const struct svm_problem *prob, const struct svm_parameter *param);
void svmCrossValidation(const struct svm_problem *prob, const struct svm_parameter *param, double *target);

int svmSaveModel(const char *model_file_name, const struct svm_model *model);
struct svm_model *svmLoadModel(const char *model_file_name);

int svm_get_svm_type(const struct svm_model *model);
int svm_get_nr_class(const struct svm_model *model);
void svm_get_labels(const struct svm_model *model, int *label);
double svm_get_svr_probability(const struct svm_model *model);

double svm_predict_values(const struct svm_model *model, const struct svm_node *x, double* dec_values);
double svm_predict(const struct svm_model *model, const struct svm_node *x);
double svm_predict_probability(const struct svm_model *model, const struct svm_node *x, double* prob_estimates);

void svmFreeModelContent(struct svm_model *model_ptr);
void svmFreeAndDestroyModel(struct svm_model **model_ptr_ptr);
void svmDestroyParam(struct svm_parameter *param);

const char *svm_check_parameter(const struct svm_problem *prob, const struct svm_parameter *param);
int svm_check_probability_model(const struct svm_model *model);

void svm_set_print_string_function(void (*print_func)(const char *));

#ifdef __cplusplus
}
#endif

#endif /* _LIBSVM_H */
