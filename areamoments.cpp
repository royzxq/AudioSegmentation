
#include "stdafx.h"

#include <stdio.h>
#include <memory.h>
#include <math.h>

#include "parameters.h"

// 计算 area of moments 特征时用
// 转化为对数值之后的值，用一个较大数归一化，这样避免计算过程溢出
// 在样本数据集上实验发现，FFT bin 最大值都小于 1.0E+7，所以乘完 "CCCC_FACTOR" 参数之后
// 约为 1.0E+10，所以取对数后约为 10.0
#define BIG_NORMALIZER_VAL			100.0

// 返回值：结果特征维数
// "pdbIndata" FFT 幅值对数谱
// 
void AreaOfMoments(const double pdbIndata[], int num_rows, int MM, double pdbOutData[], int KK)
{
	if (KK < NumDimsAreaMomentsM) {
		printf("Not enough memory for output feature data !\n");
		return;
	}
	KK = NumDimsAreaMomentsM;

	memset(pdbOutData, 0, sizeof(double)*KK);
	
	// 全部累加
	double m00 = 0.0;
	for (int irow = 0; irow < num_rows; ++irow) {// 行，连续若干帧
		for (int jfrq = 0; jfrq < MM; ++jfrq) {// 列，当前帧的各个频率分量对应的值
			m00 += pdbIndata[irow*MM+jfrq];	///BIG_NORMALIZER_VAL;	// 防溢出
		}
	}
	if (m00 == 0.0) {
		return;
	}

	double m10 = 0.0;
	double m01 = 0.0;
	double m20 = 0.0;
	double m11 = 0.0;
	double m02 = 0.0;
	double m30 = 0.0;
	double m21 = 0.0;
	double m12 = 0.0;
	double m03 = 0.0;

//	double m40 = 0.0;
//	double m31 = 0.0;
//	double m22 = 0.0;
//	double m13 = 0.0;
//	double m04 = 0.0;

/*	double m50 = 0.0;
	double m41 = 0.0;
	double m32 = 0.0;
	double m23 = 0.0;
	double m14 = 0.0;
	double m05 = 0.0;*/

	for (int i = 0; i < num_rows; ++i) {
		for (int j = 0; j < MM; ++j) {
			double tmp = pdbIndata[i*MM+j];	///BIG_NORMALIZER_VAL;	// 归一化，避免计算溢出
			m10 += tmp * i;
			m01 += tmp * j;
			m20 += tmp * i * i;
			m11 += tmp * i * j;
			m02 += tmp * j * j;
			m30 += tmp * i * i * i;
			m21 += tmp * i * i * j;
			m12 += tmp * i * j * j;
			m03 += tmp * j * j * j;

//			m40 += tmp * pow((double)i, 4.0);
//			m31 += tmp * pow((double)i, 3.0)*j;
//			m22 += tmp * i*i*j*j;
//			m13 += tmp * i*pow((double)j, 3.0);
//			m04 += tmp * pow((double)j, 4.0);

/*			m50 += tmp * pow((double)i, 5.0);
			m41 += tmp * pow((double)i, 4.0)*j;
			m32 += tmp * pow((double)i, 3.0)*j*j;
			m23 += tmp * i*i*pow((double)j, 3.0);
			m14 += tmp * i*pow((double)j, 4.0);
			m05 += tmp * pow((double)j, 5.0);*/

		}
	}
	double x_ = m10/m00;
	double y_ = m01/m00;

	pdbOutData[0] = m00;	// 00
	pdbOutData[1] = x_;
	pdbOutData[2] = y_;
	pdbOutData[3] = m20 - x_ * m10;	// 20
	pdbOutData[4] = m02 - y_ * m01;	// 02
	pdbOutData[5] = m11 - y_ * m10;	// 11
	pdbOutData[6] = m30 - 3 * x_ * m20 + 2 * x_ * x_ * m10;	// 30
	pdbOutData[7] = m03 - 3 * y_ * m02 + 2 * y_ * y_ * m01;	// 03
	pdbOutData[8] = m12 - 2 * y_ * m11 - x_ * m02 + 2 * y_ * y_ * m10;	// 12
	pdbOutData[9] = m21 - 2 * x_ * m11 - y_ * m20 + 2 * x_ * x_ * m01;	// 21

// 2013/02/27 发现 10 维效果稍好点
return;

/*	pdbOutData[10] = m40 - 4*x_*m30 + 6*x_*x_*m20 - 3*pow(x_, 3.0)*m10;	// 40
	pdbOutData[11] = m04 - 4*y_*m03 + 6*y_*y_*m02 - 3*pow(y_, 3.0)*m01;	// 04
	pdbOutData[12] = m31 - 3*x_*m21 + 3*x_*x_*m11 - pow(x_, 3.0)*m01 
					- y_*m30 + 3*x_*y_*m20 - 2*x_*x_*y_*m10;	// 31
	pdbOutData[13] = m13 - 3*y_*m12 + 3*y_*y_*m11 - pow(y_, 3.0)*m10 
					- x_*m03 + 3*y_*x_*m02 - 2*y_*y_*x_*m01;	// 13
	pdbOutData[14] = m22 - 2*y_*m21 + y_*y_*m20 
					- 2*x_*m12 + 4*x_*y_*m11 - 2*x_*y_*y_*m10 + x_*x_*m02 - x_*x_*y_*m01;	// 22*/
//
/*	pdbOutData[15] = m50 - 5*x_*m40 + 10*x_*x_*m30 - 9*pow(x_, 3.0)*m20 + 3*pow(x_, 4.0)*m10;	// 50
	pdbOutData[16] = m05 - 5*y_*m04 + 10*y_*y_*m03 - 9*pow(y_, 3.0)*m02 + 3*pow(y_, 4.0)*m01;	// 05
	pdbOutData[17] = ;	// 41
	pdbOutData[18] = ;	// 14
	pdbOutData[19] = ;	// 32
	pdbOutData[19] = ;	// 23*/
}


