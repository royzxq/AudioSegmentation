
#ifndef	__MY_ORDERED_DOUBLES__
#define	__MY_ORDERED_DOUBLES__

class COrderedDoubles {
private:
	double*_list;
	int m_nNumCanHold;
	int m_nNumElems;
	bool _bheadLarge;

private:

public:
	void NewValueCome(double dbval);	// 新来的结果要插入队列
	void InitList();
	const double& ValAt(int idx) const { return _list[idx]; }
	void AverageVal(double& dbaverage);
	double AverageVal();

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

public:
	COrderedDoubles(int num_e, bool bheadLarge);
	virtual ~COrderedDoubles();
};

#endif	// #ifndef	__MY_ORDERED_DOUBLES__
