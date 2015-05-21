

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

#include "parameters.h"
#include "procobj.h"
#include "fft\fft.h"

#ifndef __TRAINING_PHASE

extern FILE* g_pfwInfo;

int JoinSegments(SEG_INFO_T* pSegInfo, int& nNumSegs, int lenMin)
{
// 标记（仅做标记，还不删除）将被消去的段（噪声段），。。。

	int nNumSegsDeleted = nNumSegs;	// 最初段数
	int iSeg;
	int iSeg02 = -1;	// 前一个保留段的下标
	for (iSeg=0; iSeg<nNumSegs; iSeg++) {
		if (iSeg == 0) {// 第一段总是要暂时保留！
			iSeg02 = iSeg;	// 记下最近保留段的下标
			continue;
		}
//
		if (iSeg02 >= 0 &&	// 当前段的左边有保留段，。。。
			pSegInfo[iSeg02].classNo == pSegInfo[iSeg].classNo) {// 且保留段的类别与当前段一样，则并入保留段
			pSegInfo[iSeg02].numFrames += pSegInfo[iSeg].numFrames;
			pSegInfo[iSeg].classNo = -(pSegInfo[iSeg].classNo+1);	// 本段打特殊标记（必须加一以避免 0 值的影响！）
			continue;	// 必须必！！！只有当不能并入左邻时，才会考虑并入右邻！！！
		}

// 以下为非同类段之间的合并，。。。

	// （+）此自然段足够长，则保留它，。。。
		if (pSegInfo[iSeg].numFrames >= lenMin) {// 只要够长（也可是静音段！！！），。。。
			// 先吸收左邻连续出现的噪声段，有多少吸收多少
			for (int iSeg03=iSeg-1; iSeg03>=0; iSeg03--) {
				if (pSegInfo[iSeg03].classNo < 0) break;	// 已被其左段吸收（其当然是噪声段），。。。
//
				if (pSegInfo[iSeg03].numFrames >= lenMin) break;	// 一旦碰到非噪声段（长段），就退出循环，。。。
//
				// 是噪声段，则吸之，。。。
				pSegInfo[iSeg].numFrames += pSegInfo[iSeg03].numFrames;
				pSegInfo[iSeg03].classNo = -(pSegInfo[iSeg03].classNo+1);	// 打删除标记（必须加一以避免 0 值的影响！）
			}
//
			iSeg02 = iSeg;	// 记下（最近保留段的）下标
			continue;
		}

	// （-）此自然段不够长，将被处理，。。。

		// 检查左邻保留段，。。。
		// 可见，一个足够长的保留段可以吸收连续多个（有多少就吸收多少）噪声段
		if (iSeg02 >= 0 && // 当前段的左边有保留段，。。。
			pSegInfo[iSeg02].numFrames >= lenMin) {// 左保留段够长，能吸收此段，。。。
			pSegInfo[iSeg02].numFrames += pSegInfo[iSeg].numFrames;	// 将当前段并入左保留，必须调整左保留段的结束位置！！！
			pSegInfo[iSeg].classNo = -(pSegInfo[iSeg].classNo+1);	// 本段打特殊标记（必须加一以避免 0 值的影响！）
			continue;	// 必须必！！！只有当不能并入左邻时，才会考虑并入右邻！！！
		}

		// 此段虽不够长，但因无法并入左邻段，故暂且保留！
		iSeg02 = iSeg;	// 记下最近保留段的下标
	}

// （三）真正挤掉（即删除）被打了标记的段，。。。

	iSeg02 = 0;
	for (iSeg=0; iSeg<nNumSegs; iSeg++) {
		if (pSegInfo[iSeg].classNo < 0) continue;
		pSegInfo[iSeg02].numFrames = pSegInfo[iSeg].numFrames;
		pSegInfo[iSeg02].classNo = pSegInfo[iSeg].classNo;
		iSeg02++;
	}
	nNumSegs = iSeg02;	// 修改段数

// ////////////////////////////////////////////////////////////////////////////////////////////

// 合并同类段，。。。

	int iClassNoPre;
	iSeg02 = 0;
	for (iSeg=0; iSeg<nNumSegs; iSeg++) {
		if (iSeg == 0 || pSegInfo[iSeg].classNo != iClassNoPre) {// 新开段（包括第一段），。。。			
			pSegInfo[iSeg02].numFrames = pSegInfo[iSeg].numFrames;
			pSegInfo[iSeg02].classNo = pSegInfo[iSeg].classNo;
			iSeg02++;

			iClassNoPre = pSegInfo[iSeg].classNo;
		} else {// 同类段，并入前段，只改前段之右界
			pSegInfo[iSeg02-1].numFrames += pSegInfo[iSeg].numFrames;	// 注意下标，"iSeg02-1" !!!
		}
	}
	nNumSegs = iSeg02;	// 修改段数

// ////////////////////////////////

	nNumSegsDeleted -= nNumSegs;
	return nNumSegsDeleted;
}

#endif	// #ifndef __TRAINING_PHASE

