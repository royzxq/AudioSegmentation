

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

#include "parameters.h"
#include "order_list.h"

// 一般排序和插入方法
// 
void COrderedDoubles::NewValueCome(double dbval)
{
	// 找插入点，。。。
	int iElem;
	for (iElem=0; iElem<m_nNumElems; iElem++) {
		if (_bheadLarge) {// 从大到小排序，。。。
			if (dbval > _list[iElem]) break;
		} else {// 从小到大排序，。。。
			if (dbval < _list[iElem]) break;
		}
	}
	// "iElem" 插入点
	for (int iElem02=m_nNumElems-1; iElem02>=iElem; iElem02--) {// 只移已有元素，。。。
		if (iElem02+1 < m_nNumCanHold) {// 表满时，最后一个元素不移（故被冲）
			_list[iElem02+1] = _list[iElem02];
		}
	}
	// 如还有空位，则插入
	if (iElem < m_nNumCanHold) {// 还有空位，。。。
		_list[iElem] = dbval;
		if (m_nNumElems < m_nNumCanHold) {
			m_nNumElems++;
		}
	}
}

void COrderedDoubles::AverageVal(double& dbaverage)
{
	if (m_nNumElems == 0) return;

	dbaverage = 0.0;
	int iElem;
	for (iElem=0; iElem<m_nNumElems; iElem++) {
		dbaverage += _list[iElem];
	}
	dbaverage /= m_nNumElems;
}

double COrderedDoubles::AverageVal()
{
	assert(m_nNumElems > 0);

	double dbaverage = 0.0;
	int iElem;
	for (iElem=0; iElem<m_nNumElems; iElem++) {
		dbaverage += _list[iElem];
	}
	dbaverage /= m_nNumElems;

	return dbaverage;
}

// bheadLarge : 排序方法：true, 从大到小; false, 从小到大
// 
COrderedDoubles::COrderedDoubles(int num_elems, bool bheadLarge)
{
	_list = Malloc(double, num_elems);
	if (_list) {
		m_nNumCanHold = num_elems;		
	} else {
		m_nNumCanHold = 0;
	}
	_bheadLarge = bheadLarge;
	m_nNumElems = 0;
}

// 清除表中内容，但不释放内存
// 
void COrderedDoubles::InitList()
{
	m_nNumElems = 0;
}

COrderedDoubles::~COrderedDoubles()
{
	if (_list)
		free(_list);
}

