

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

#include "parameters.h"
#include "svmInterface.h"

int ReadTextLine(FILE *pfr, char*&, int&);
void exit_input_error(int line_num);
void exit_with_help();

#ifdef __TRAINING_PHASE

void print_null(const char *s)
{

}

void CSvmInterface::ParseConfigFile(const char *svm_config_file)
{
	void (*print_func)(const char*) = NULL;	// default printing to stdout

	// default values
	m_param.svm_type = C_SVC;
	m_param.kernel_type = RBF;
	m_param.degree = 3;
	m_param.gamma = 0;	// 1/num_features
	m_param.coef0 = 0;
	m_param.nu = 0.5;
	m_param.cache_size = 100;
	m_param.C = 1;
	m_param.eps = 1e-3;
	m_param.p = 0.1;
	m_param.shrinking = 1;
	m_param.probability = 0;
	m_param.nr_weight = 0;
	m_param.weight_label = NULL;
	m_param.weight = NULL;
	m_param.tag[0] = 0;	// no cross validation
	m_param.tag[2] = 0;	// no scale

    FILE *fc = fopen(svm_config_file, "rt");
    if (fc == NULL) {
        fprintf(stderr, "Error opening the svm config file !\n");
        exit_with_help();
    }

	int nLen = 1024;
	char* psline = Malloc(char, nLen);
    char key;
    char value[256];
    while(1) {
        if (ReadTextLine(fc, psline, nLen) != 0) break;	// 文件已结束
//
        if (psline[0] == '#') continue;
        sscanf(psline, "%c %s", &key, value);
        switch (key) {
            case 0xA:
                break;
            case 's':
				m_param.svm_type = atoi(value);
				break;
			case 't':
				m_param.kernel_type = atoi(value);
				break;
			case 'd':
				m_param.degree = atoi(value);
				break;
			case 'g':
				m_param.gamma = atof(value);
				break;
			case 'r':
				m_param.coef0 = atof(value);
				break;
			case 'n':
				m_param.nu = atof(value);
				break;
			case 'm':
				m_param.cache_size = atof(value);
				break;
			case 'c':
				m_param.C = atof(value);
				break;
			case 'e':
				m_param.eps = atof(value);
				break;
			case 'p':
				m_param.p = atof(value);
				break;
			case 'h':
				m_param.shrinking = atoi(value);
				break;
			case 'b':
				m_param.probability = atoi(value);
				break;
			case 'q':
				print_func = &print_null;
				break;
			case 'v':
				m_param.tag[0] = 1;
				m_param.tag[1] = atoi(value);
				if (m_param.tag[1] < 3) {
					fprintf(stderr, "n-fold cross validation: n must >= 3 !\n");
					m_param.tag[1] = 3;
				}
				if (m_param.tag[1] > 10) {
					m_param.tag[1] = 10;
				}
				break;
			case 'z':
				// Do scaling, ...
				m_param.tag[2] = 1;
				break;

			/*case 'w':
				++m_param.nr_weight;
				m_param.weight_label = (int *)realloc(m_param.weight_label, sizeof(int)*m_param.nr_weight);
				m_param.weight = (double *)realloc(m_param.weight, sizeof(double)*m_param.nr_weight);
				m_param.weight_label[m_param.nr_weight-1] = atoi(&argv[i-1][2]);
				m_param.weight[m_param.nr_weight-1] = atof(value);
				break;*/
			default:
				fprintf(stderr, "Unknown option: -%c\n", key);
				exit_with_help();
		}
	}
    fclose(fc);
	free(psline);
	svm_set_print_string_function(print_func);
}

#else

#endif	// #ifndef __TRAINING_PHASE


