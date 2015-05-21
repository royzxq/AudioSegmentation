

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

#include "procobj.h"

#define Idx1D(hh, ww, width) (hh*width+ww)

// 计算二维离散余弦变换
// HH 信号高度
// WW 信号宽度
// HHO 结果高度，参数个数
// WWO 结果宽度，参数个数；最终计算出的参数个数为 HHO*WWO。
// 
void DCT2D(const double ff[], int HH, int WW, double dct[], int HHO, int WWO)
{
	if (HHO > HH) { 
		printf("Hight exceeds limit !\n"); HHO = HH; }
	if (WWO > WW) { 
		printf("Width exceeds limit !\n"); WWO = WW; }

	int hho, wwo;	// 结果下标
	int hh, ww;	// 输入信号下标
	for (hho=0; hho<HHO; hho++) { for (wwo=0; wwo<WWO; wwo++) {
///////////////////////////////////////////////////////////////////////
			dct[Idx1D(hho, wwo, WWO)] = 0.0;
			for (hh=0; hh<HH; hh++) { for (ww=0; ww<WW; ww++) {
					dct[Idx1D(hho, wwo, WWO)] += ff[Idx1D(hh, ww, WW)]*
						cos(MY_PI*hho/HH*(0.5+hh))*
						cos(MY_PI*wwo/WW*(0.5+ww));
			}}
			if (hho == 0) ;	//dct[Idx1D(hho, wwo, WWO)] *= sqrt(1.0/HH);
			else dct[Idx1D(hho, wwo, WWO)] *= sqrt(2.0);	///HH);
			if (wwo == 0) ;	//dct[Idx1D(hho, wwo, WWO)] *= sqrt(1.0/WW);
			else dct[Idx1D(hho, wwo, WWO)] *= sqrt(2.0);	///WW);
////////////////////////////////////////////////////////////////////////
	}}
}
