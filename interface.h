
#ifndef	__MY_WAVE_DATA__INTERFACE__
#define	__MY_WAVE_DATA__INTERFACE__

class CWavPreproc {
private:
	int m_fhw;
	unsigned short bit_p_sample_;	/* bits per sample, 8, 12, 16 */
	unsigned long sample_rate_;	/* Sample rate */
	unsigned short byte_p_sample_;	/* Bytes per sample, including data for all channels */
	unsigned short num_of_channels_;		/* 1 = Mono, 2 = Stereo */

	// total number of down-sampling samples made 
	unsigned long m_nNumNewlyMadeSampsTotal;
	// “原采样缓存”中第一个位置的采样点的全局下标
	long m_iOriginalSampStartG;
	// 本批生成的第一个欠采样点（即“欠采样缓存”中第一个采样点）的全局下标
	long m_iTargetSampStartG;

	short *m_NewlyMadeSampsBuff;

private:
	int MakeTargetSamplesData(int fhw, int&);

public:
	void CreateSampDataFile(const char* psfn);
	void DataCome(const unsigned char* pSamps, int nNumSamps);

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

public:
	CWavPreproc();
	virtual ~CWavPreproc();
};

#endif	// #ifndef	__MY_WAVE_DATA__INTERFACE__
