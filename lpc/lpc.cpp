
#include "stdafx.h"

#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#include "parameters.h"

#include "lpc.h"

void LPC(const short xxx[], int NNN, double aaa_out[], int ppp);

#define FRAME_SIZE_IN_SAMPLES				441		// 在 22050 Hz 的统一信号采样率下，合 20 毫秒

#define NUM_FRAMES_IN_LPC_WINDOW			6
#define NUM_FRAMES_LPC_WINDOW_HOP			4

CCalcLPC::CCalcLPC()
{

}

CCalcLPC::~CCalcLPC()
{

}

// 输入文件（"pfr"）：关键段音频数据文件
int CCalcLPC::CalcVector(FILE *pfr)
{
	double *aaa = (double *)malloc(sizeof(double)*NUM_DIMS_LPC+
									sizeof(short)*FRAME_SIZE_IN_SAMPLES*NUM_FRAMES_IN_LPC_WINDOW);
	if (aaa == NULL) {
		printf("No memory, LPC !\n");
		return -1;
	}
	short *pSampDataL = (short *)(aaa+NUM_DIMS_LPC);	// 采样数据缓冲区（一个 LPC 窗口大小）

// 先从音乐关键段采样数据文件头读出 3 个参数
	unsigned long ulStartSampGlobal, ulNumSamples;
	int s_rate;
	fread(&s_rate, sizeof(int), 1, pfr);	// 信号采样率
	assert(s_rate == (int)UNIFIED_SAMPLING_RATE);
	fread(&ulStartSampGlobal, sizeof(unsigned long), 1, pfr);
	fread(&ulNumSamples, sizeof(unsigned long), 1, pfr);

// /////////////////////////////////////////////////////

	size_t nNumRead, nNumShortsInBuf = 0;
	while (!feof(pfr)) {
		// 实际读入了多少？
		nNumRead = fread(pSampDataL+nNumShortsInBuf, sizeof(short), 
						FRAME_SIZE_IN_SAMPLES*NUM_FRAMES_IN_LPC_WINDOW - nNumShortsInBuf, pfr);
		if (nNumRead < FRAME_SIZE_IN_SAMPLES*NUM_FRAMES_IN_LPC_WINDOW - nNumShortsInBuf) {
			break;
		}

		LPC(pSampDataL, FRAME_SIZE_IN_SAMPLES*NUM_FRAMES_IN_LPC_WINDOW, aaa, NUM_DIMS_LPC);

// LPC 特征已得到，。。。


// move overlapped data to buffer top
		memmove(pSampDataL, pSampDataL+FRAME_SIZE_IN_SAMPLES*NUM_FRAMES_LPC_WINDOW_HOP,
					sizeof(short)*FRAME_SIZE_IN_SAMPLES*(NUM_FRAMES_IN_LPC_WINDOW-NUM_FRAMES_LPC_WINDOW_HOP));

		nNumShortsInBuf = FRAME_SIZE_IN_SAMPLES*(NUM_FRAMES_IN_LPC_WINDOW-NUM_FRAMES_LPC_WINDOW_HOP);
	}

	free(aaa);

	return 0;	// 0 : OK
}

void wAutocorrelate(const short *xxx, int NNN, double *rrr_out, int PPP, double lambda);

void LPC(const short xxx[], int NNN, double aaa_out[], int ppp)
{
	memset(aaa_out, 0, sizeof(double)*ppp);
	double EEE;
/*	EEE = 0.0;
	for (int ii=0; ii<NNN; ii++) {
		EEE += abs(xxx[ii]);
	}
	if (EEE/NNN <= MINIMUM_MEAN_ABS_SIGNAL_VALUE) {// 信号绝对值平均值太小，则给全 0 输出，。。。
		return;
	}*/

	double kkk;
	double *RRR = (double *)malloc(sizeof(double)*(ppp+1));
	double *aaa = (double *)malloc(sizeof(double)*(ppp+1));
	double *ttt = (double *)malloc(sizeof(double)*(ppp+1));

// 求自相关函数，。。。

/*	for (int jjj=0; jjj<=ppp; jjj++) {
		RRR[jjj] = 0.0;
		for (int iii=0; iii<NNN-jjj; iii++) {
			RRR[jjj] += xxx[iii]*xxx[iii+jjj];
		}
	}*/
	wAutocorrelate(xxx, NNN, RRR, ppp, 0.0);

	if (RRR[0] <= 0.0) {
		memset(aaa_out, 0, sizeof(double)*ppp);
		free(ttt);
		free(aaa);
		free(RRR);
		return;
	}
	EEE = RRR[0];

	for (int iii=1; iii<=ppp; iii++) {
		kkk = 0.0;
		for (int jjj=1; jjj<iii; jjj++) {
			kkk += aaa[jjj]*RRR[iii-jjj];
		}
		assert(EEE > 0.0);
		kkk = (RRR[iii]-kkk)/EEE;

		aaa[iii] = kkk;
		for (int jjj=1; jjj<iii; jjj++) {
			ttt[jjj] = aaa[jjj]-kkk*aaa[iii-jjj];
		}
		for (int jjj=1; jjj<iii; jjj++) {
			aaa[jjj] = ttt[jjj];
		}
		EEE = (1.0-kkk*kkk)*EEE;
	}

	for (int jjj=0; jjj<ppp; jjj++) {
		aaa_out[jjj] = aaa[jjj+1];
	}

	free(ttt);
	free(aaa);
	free(RRR);
}

void LPC02(short *xxx, int NNN, double *aaa, int ppp)
{
	double *eee = (double *)malloc(sizeof(double)*NNN);
	double *bbb = (double *)malloc(sizeof(double)*NNN);
	double *ttt = (double *)malloc(sizeof(double)*NNN);
	// 初始化，。。。
	for (int nnn=0; nnn<NNN; nnn++) {
		eee[nnn] = bbb[nnn] = xxx[nnn];
	}

	double kkk, denominator01, denominator02;
	for (int iii=1; iii<ppp; iii++) {
		kkk = 0.0;
		denominator01 = eee[0]*eee[0];
		denominator02 = 0.0;
		for (int nnn=1; nnn<NNN; nnn++) {
			kkk += eee[nnn]*bbb[nnn-1];
			denominator01 += eee[nnn]*eee[nnn];
			denominator02 += bbb[nnn-1]*bbb[nnn-1];
		}
		kkk = 2.0*kkk/(denominator01+denominator02);

		for (int jjj=1; jjj<iii; jjj++) {
			ttt[jjj] = aaa[jjj]-kkk*aaa[iii-jjj];
		}
		for (int jjj=1; jjj<iii; jjj++) {
			aaa[jjj] = ttt[jjj];
		}
		aaa[iii] = kkk;

		for (int nnn=NNN-1; nnn>0; nnn--) {
			bbb[nnn] = bbb[nnn-1]-kkk*eee[nnn];	// 计算 bbb[nnn] 时，bbb[nnn-1] 必须还是原值，故下标从大到小
			eee[nnn] = eee[nnn]-kkk*bbb[nnn-1];	
		}
		bbb[0] = 0.0-kkk*eee[0];
		eee[0] = eee[0]-0.0;	
	}

	free(ttt);
	free(eee);
	free(bbb);
}


// 从 jAudio 所参考的地方搞来的！！！
/*
Notes : 
The autocorrelation function implements a warped autocorrelation, so that frequency resolution can be specified 
by the variable 'lambda'. Levinson-Durbin recursion calculates autoregression coefficients a and reflection 
coefficients (for lattice filter implementation) K. Comments for Levinson-Durbin function implement matlab version 
of the same function.
*/
//find the order-P autocorrelation array, R, for the sequence x of length L and warping of lambda
//wAutocorrelate(&pfSrc[stIndex], siglen, R, P, 0);
void wAutocorrelate(const short xxx[], int NNN, double rrr_out[], int PPP, double lambda)
{
//	double lambda = 0.0;

    double *dl = new double[NNN];
    double *Rt = new double[NNN];

    Rt[0] = 0.0;

	Rt[0] += xxx[0]*xxx[0];
	dl[0] = 0.0 - lambda*(xxx[0]-0.0);
    for (int nn=1; nn<NNN; nn++) {
		Rt[0] += xxx[nn]*xxx[nn];
		dl[nn] = xxx[nn-1] - lambda*(xxx[nn]-dl[nn-1]);
    }

//	double xxx_pre, dl_pre_new;
//	xxx_pre = 0;
//	dl_pre_new = 0;

	double dbtmp;
	double dl_pre_old;
    for (int ii=1; ii<=PPP; ii++) {
 //		dl_pre_new = 0;
        Rt[ii] = 0.0;

        dl_pre_old = 0.0;
		Rt[ii] += dl[0]*xxx[0];
		dbtmp = dl[0];
		dl[0] = dl_pre_old - lambda*(dl[0]-0.0);
		dl_pre_old = dbtmp;
//		dl_pre_new = dl[nn];

		for (int nn=1; nn<NNN; nn++) {
            Rt[ii] += dl[nn]*xxx[nn];
			dbtmp = dl[nn];
			dl[nn] = dl_pre_old - lambda*(dl[nn]-dl[nn-1]);
            dl_pre_old = dbtmp;
//			dl_pre_new = dl[nn];
        }
    }

    rrr_out[0] = 0;	// 显然多余（我抄来的，所以放着）
    for(int ii=0; ii<=PPP; ii++) {
        rrr_out[ii] = Rt[ii];
	}

    delete []dl;
    delete []Rt;

	return;
}

// Calculate the Levinson-Durbin recursion for the autocorrelation sequence R of length P+1 and return 
// the autocorrelation coefficients A and reflection coefficients K
int LevinsonRecursion(int P, const float *R, float *A, float *K)
{
    double Am1[62];

    if( R[0] == 0.0f ) { 
        for(int i=1; i<=P; i++) {
            K[i]=0.0; 
            A[i]=0.0;
        }
	} else {
        double km,Em1,Em;
        unsigned int k, s, m;
        for (k=0; k<=P; k++){
            A[0]=0;
            Am1[0]=0; 
		}
        A[0] = 1;
        Am1[0] = 1;
        km = 0;
        Em1 = R[0];
        for (m=1; m<= P; m++) {                   //m=2:N+1
        
            double err=0.0f;                    //err = 0;

            for (k=1; k<=m-1; k++)            //for k=2:m-1
                err += Am1[k]*R[m-k];        // err = err + am1(k)*R(m-k+1);

            km = (R[m]-err)/Em1;            //km=(R(m)-err)/Em1;
            K[m-1] = -float(km);
            A[m]=(float)km;                        //am(m)=km;

            for (k=1; k<=m-1; k++)            //for k=2:m-1
                A[k]=float(Am1[k]-km*Am1[m-k]);  // am(k)=am1(k)-km*am1(m-k+1);

            Em = (1-km*km)*Em1;                //Em=(1-km*km)*Em1;

            for(s=0; s<=P; s++)                //for s=1:N+1
                Am1[s] = A[s];                // am1(s) = am(s)

            Em1 = Em;                        //Em1 = Em;
        }
    }

    return 0;
}

