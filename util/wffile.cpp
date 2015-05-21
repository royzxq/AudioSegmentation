
#include "stdafx.h"

#include <io.h>
// for _O_RDONLY etc.
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <FLOAT.H>

#include "parameters.h"

#include "my_util.h"

#include "main_seg.h"

#define BUFFER_SIZE_IN_SAMPLES_DS	8192	// 8k, for down sampling op. 

typedef struct {  
	char			data_chunk_id[4];	/* 'data' */
	unsigned long	data_chunk_size;	/* length of data */
} waveheader_ext_t;


CWFFile::CWFFile()
{
	m_pfWaveR = NULL;
}

CWFFile::~CWFFile()
{
	if (m_pfWaveR)
		fclose(m_pfWaveR);
}

void CWFFile::CloseWaveFile()
{
	if (m_pfWaveR) {
		fclose(m_pfWaveR);
		m_pfWaveR = NULL;
	}
}

// 读文件头，并检查文件是否合法
// 最后两个参数：消息缓存指针和缓存大小（字节数）
// 
int CWFFile::OpenWaveFile(const char *psfn_waveR)
{
	m_pfWaveR = NULL;

	FILE *pfWave = fopen(psfn_waveR, "rb");	// read from a binary file
	if (pfWave == NULL) {
		printf("Can't open the wf file %s!\n", psfn_waveR);
		return -10;
	}

// 得到文件长度，。。。
	fseek(pfWave, 0, SEEK_END);	// 指针移到文件尾
	long llen_data;
	llen_data = ftell(pfWave);
	fseek(pfWave, 0, SEEK_SET);	// 指针重回文件头
//

	fread(&m_header, sizeof(waveheader_t), 1, pfWave);
	if ( strncmp(m_header.root_chunk_id, "RIFF", 4) != 0 || strncmp(m_header.riff_type_id, "WAVE", 4) != 0) {
	// not a wave file
		fclose(pfWave);
		printf("Not a wave file, abort !\n");
		return -20;
	}
	if (m_header.fmt_chunk_data_size > 16) {
		fseek(pfWave, m_header.fmt_chunk_data_size-16, SEEK_CUR);
	}

	waveheader_ext_t header_ext;	// 扩展文件头
	fread(&header_ext, sizeof(waveheader_ext_t), 1, pfWave);
	long lLenWaveHeader = sizeof(waveheader_t)+m_header.fmt_chunk_data_size-16+sizeof(waveheader_ext_t);

	////////////////////////////////////////////////////////////////////////////////////////////////////

	llen_data -= lLenWaveHeader;

	printf("\nwave header length : %u\n", lLenWaveHeader);
	printf("wave data length : %u\n\n", llen_data);

//	char			root_chunk_id[4];		// 'RIFF'
//	unsigned long	root_chunk_data_size;	// length of root chunk
	printf("root_chunk_data_size : %u\n", m_header.root_chunk_data_size);
//	char			riff_type_id[4];		// 'WAVE'
//	char			fmt_chunk_id[4];		// 'fmt '
//	unsigned long	fmt_chunk_data_size;	// length of sub_chunk, always 16 bytes
	printf("fmt_chunk_data_size(16) : %u\n", m_header.fmt_chunk_data_size);
//	unsigned short	compression_code;		// always 1 = PCM-Code
	printf("compression_code(1) : %d\n", m_header.compression_code);

//	unsigned short	num_of_channels;		// 1 = Mono, 2 = Stereo
	printf("num_of_channels(1 = Mono, 2 = Stereo) : %d\n", m_header.num_of_channels);

//	unsigned long	sample_rate;			// Sample rate
	printf("sample_rate : %u\n", m_header.sample_rate);

//	unsigned long	byte_p_sec;				// average bytes per sec
	printf("byte_p_sec(average bytes per sec) : %u\n", m_header.byte_p_sec);

//	unsigned short	byte_p_sample;			// Bytes per sample, including the sample's data for all channels!
	printf("byte_p_sample : %d\n", m_header.byte_p_sample);
//	unsigned short	bit_p_sample;			// bits per sample, 8, 12, 16
	printf("bit_p_sample(8, 12, or 16) : %d\n", m_header.bit_p_sample);

//	char			data_chunk_id[4];		// 'data'
//	unsigned long	data_chunk_size;		// length of data
	printf("data_chunk_size : %u\n", header_ext.data_chunk_size);
	if ((unsigned long)llen_data != header_ext.data_chunk_size) {
		printf("llen_data = %u, not equal to data_chunk_size !\n", (unsigned long)llen_data);
//		assert(0);
	}

	if (m_header.compression_code != 1) {
		fclose(pfWave);
		printf("Not in the reqired compression code, abort !\n");
		return -30;
	}
	if (m_header.bit_p_sample != 16) {
		fclose(pfWave);
		printf("Bits per sample is not 16, abort !\n");
		return -40;
	}
	if (m_header.sample_rate < UNIFIED_SAMPLING_RATE) {
		fclose(pfWave);
		printf("Sampling rate is less than required, abort !\n");
		return -50;
	}
	if (m_header.num_of_channels > 2) {
		fclose(pfWave);
		printf("More than 2 channels, abort !\n");
		return -60;
	}

	// number of original samples in the audio file !!!
//	m_lNumSamplesInFile = m_header_ext.data_chunk_size/m_header.byte_p_sample;
	// number of selected samples
//	m_lNumSamplesInFile /= DOWN_SAMPLING_RATE;

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	m_pfWaveR = pfWave;
	fseek(m_pfWaveR, lLenWaveHeader, SEEK_SET);	// 将文件指针移到 Waveform 数据开始处

	return 0;	// OK
}

// 按统一采样率做（欠）采样，并将多声道（多个采样值）合并成一个声道（一个采样值）
// "maxAbs" 信号采样绝对值的最大值
int CWFFile::MakeTargetSamplesData(int fhw, int& maxAbs)
{
	assert(m_header.bit_p_sample == 16 || m_header.bit_p_sample == 8);
	// "m_header.sample_rate" : 原始采样频率
	// 原始采样率必须高于或等于 "UNIFIED_SAMPLING_RATE"（此即目标采样率！！！）
	if (m_header.sample_rate < UNIFIED_SAMPLING_RATE) {
		printf("m_header.sample_rate(%u) < UNIFIED_SAMPLING_RATE !\n", m_header.sample_rate);
		return -1;
	}

	short *NewlyMadeSampsBuff = NULL;
	unsigned char *szOrginalSampsBuffer = NULL;
	// buffer to hold newly made down-sampling samples, 16 bits per sample!
	NewlyMadeSampsBuff = (short *)malloc(sizeof(short)*BUFFER_SIZE_IN_SAMPLES_DS+
										(m_header.byte_p_sample)*BUFFER_SIZE_IN_SAMPLES_DS);
	if (NewlyMadeSampsBuff == NULL) {
		printf("No memory !\n");
		return -2;
	}
	szOrginalSampsBuffer = (unsigned char *)(NewlyMadeSampsBuff+BUFFER_SIZE_IN_SAMPLES_DS);

// ////////////////////////////////////////////////////////////////////////////////////////

	maxAbs = 0;
	int xxx;	// OK

// 注意：欠采样信号和原采样信号对应的时长是一样的！！！

	// “原采样缓存”中第一个位置所存的采样的全局下标
	long iOriginalSampStartG = 0;
	// 本批生成的第一个欠采样点（即“欠采样缓存”中第一个点）的全局下标
	long iTargetSampStartG = 0;
	// total number of down-sampling samples made 
	unsigned long nNumNewlyMadeSampsTotal = 0;

#if defined(__USE_WHOLE_SIGNAL)
	int& sr = xxx;
	sr = UNIFIED_SAMPLING_RATE;
	_write(fhw, &sr, sizeof(int));	// 采样率
	_write(fhw, &iTargetSampStartG, sizeof(unsigned long));	// 起始采样下标
	_write(fhw, &nNumNewlyMadeSampsTotal, sizeof(unsigned long));	// 目标采样个数
#else
	CMainSegment* pMainSeg = new CMainSegment();
	if (pMainSeg == NULL || !pMainSeg->IsCreatedOK()) {
		printf("pMainSeg == NULL || !pMainSeg->IsCreatedOK() !\n");
		free(NewlyMadeSampsBuff);
		return -3;
	}
	// 关键段限长！！！
	xxx = NumFrameSkipsInUnitM*NumSamplesInFrameSkipM+NumSamplesInFrameM;	// 信号采样个数
	xxx = (double)xxx/UNIFIED_SAMPLING_RATE*1000.0/MILISECONDS_DETECT_SKIP;	// 换算成检测跳步个数
	pMainSeg->SetMaxMainSegSize(xxx);
#endif

	xxx = 0;
	// 每从原始采样数据文件读“一批”原采样数据，就生成一批欠采样。
	while ( !feof(m_pfWaveR) ) {
		int nNumNewlyMadeSamps = 0;	// 本批（即从本次读取的原始信号数据）生成的欠采样点个数
		//（一）从当前歌曲音频数据文件读一块数据，。。。
		size_t uNumOrigSampsRead;
		uNumOrigSampsRead = fread(szOrginalSampsBuffer, 
					m_header.byte_p_sample,	// 一个“采样”包含了一个采样时间点所有声道的数据
					BUFFER_SIZE_IN_SAMPLES_DS,	// 要读取的采样个数
					m_pfWaveR);
		if (uNumOrigSampsRead == 0) {// no content read
			break;
		}

		if (iTargetSampStartG == 0) {
		// 无论如何做欠采样，第一个原始采样总是要直接拷贝的。也就是说，欠采样信号的第一个样本点就是原信号的第一个
		// 样本点！
			assert(nNumNewlyMadeSamps == 0);
			// 第一个采样值照搬
// clrc_001
#ifdef __COMBINE_L_R_CHANNELS
// 取各个声道的平均
			if (m_header.num_of_channels > 1) {
				long lval;	// 用 "long" 保证计算过程不溢出
				if (m_header.bit_p_sample == 16) {
					lval = *((short *)szOrginalSampsBuffer);
					lval += *(((short *)szOrginalSampsBuffer)+1);
				} else if (m_header.bit_p_sample == 8) {
					lval = *((char *)szOrginalSampsBuffer);
					lval += *(((char *)szOrginalSampsBuffer)+1);
					lval *= 256;
				}
				lval /= 2;
				NewlyMadeSampsBuff[nNumNewlyMadeSamps] = lval;
			} else {// 只有一个声道时，。。。
				if (m_header.bit_p_sample == 16) {
					NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((short *)szOrginalSampsBuffer);
				} else if (m_header.bit_p_sample == 8) {
					NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((char *)szOrginalSampsBuffer);
					NewlyMadeSampsBuff[nNumNewlyMadeSamps] *= 256;
				}
			}
#else
// 只取第一个声道
			if (m_header.bit_p_sample == 16) {
				NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((short *)szOrginalSampsBuffer);	// 第一个采样值照搬
			} else if (m_header.bit_p_sample == 8) {
				NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((char *)szOrginalSampsBuffer);	// 第一个采样值照搬
				NewlyMadeSampsBuff[nNumNewlyMadeSamps] *= 256;
			}
#endif
// clrc_001
			// 找采样的绝对值最大值
			if (maxAbs < abs(NewlyMadeSampsBuff[nNumNewlyMadeSamps]))
				maxAbs = abs(NewlyMadeSampsBuff[nNumNewlyMadeSamps]);
			nNumNewlyMadeSamps++;
		}

		//（二）生成对应的新采样，。。。
		while (1) {
		// 核心是将欠采样点对应到某个原采样点（只有左采样点），或两个相邻的原采样点（即左右两个采样点）之间！！！

			// 与待求的欠采样点对应的原始采样点的全局下标（理论上不一定是整数）
			long iOriginalSampG;
			long iTargetSampG;
			// 根据欠采样点下标，。。。
			iTargetSampG = iTargetSampStartG+nNumNewlyMadeSamps;
			// 计算其对应的原采样点下标
			iOriginalSampG = (double)iTargetSampG*m_header.sample_rate/UNIFIED_SAMPLING_RATE+0.5;
			if (iOriginalSampG > iOriginalSampStartG+uNumOrigSampsRead-1) {
			// 还未读到“目标原采样点”，。。。
				iOriginalSampStartG += uNumOrigSampsRead;
				break;
			}
			// 目标原采样点在缓存中的位置
			const unsigned char *puchar = szOrginalSampsBuffer+
				(m_header.byte_p_sample)*(iOriginalSampG-iOriginalSampStartG);
// clrc_002
#ifdef __COMBINE_L_R_CHANNELS
// 取各个声道的平均
			if (m_header.num_of_channels > 1) {
				long lval;
				if (m_header.bit_p_sample == 16) {// 16 位采样
					lval = *((short *)puchar);	// 本采样的第一个声道
					lval += *(((short *)puchar)+1);	// 本采样的第二个声道
				} else if (m_header.bit_p_sample == 8) {// 8 位采样
					lval = *((char *)(puchar));	// 本采样的第一个声道
					lval += *(((char *)puchar)+1);	// 本采样的第二个声道
					lval *= 256;
				}
				lval /= 2;
				NewlyMadeSampsBuff[nNumNewlyMadeSamps] = lval;
			} else {// 只有一个声道时，。。。
				if (m_header.bit_p_sample == 16) {
					NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((short *)puchar);
				} else if (m_header.bit_p_sample == 8) {
					NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((char *)puchar);
					NewlyMadeSampsBuff[nNumNewlyMadeSamps] *= 256;
				}
			}
#else
// 只取第一个声道
			if (m_header.bit_p_sample == 16) {
				NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((short *)puchar);
			} else if (m_header.bit_p_sample == 8) {
				NewlyMadeSampsBuff[nNumNewlyMadeSamps] = *((char *)puchar);
				NewlyMadeSampsBuff[nNumNewlyMadeSamps] *= 256;
			}
#endif
// clrc_002
			// 找采样的绝对值最大值
			if (maxAbs < abs(NewlyMadeSampsBuff[nNumNewlyMadeSamps]))
				maxAbs = abs(NewlyMadeSampsBuff[nNumNewlyMadeSamps]);
			nNumNewlyMadeSamps++;	// 本批欠采样个数计数
			
		}	// end of "while (1) {"
#if defined(__USE_WHOLE_SIGNAL)
		//（三）将本批新生成的采样（经“欠采样”和声道合并后的采样数据）写入输出文件
		if (_write(fhw, NewlyMadeSampsBuff, nNumNewlyMadeSamps*sizeof(short)) != nNumNewlyMadeSamps*sizeof(short)) {
			xxx = -1;
			break;
		}
#else
		pMainSeg->DetectInWindow(NewlyMadeSampsBuff, iTargetSampStartG, nNumNewlyMadeSamps);
#endif	// #if defined(__USE_WHOLE_SIGNAL)

		//（四）准备从音乐文件读入下一批原采样数据，。。。
		iTargetSampStartG += nNumNewlyMadeSamps;
		nNumNewlyMadeSampsTotal += nNumNewlyMadeSamps;
	}	// end of "while ( !feof(m_pfWaveR) ) {"
//
	free(NewlyMadeSampsBuff);
//
#if defined(__USE_WHOLE_SIGNAL)
	_lseek(fhw, sizeof(int)+sizeof(unsigned long), SEEK_SET);	// 从文件头，跳过前两个参数
	_write(fhw, &nNumNewlyMadeSampsTotal, sizeof(unsigned long));	// 采样个数
#else
	if (pMainSeg) {
		pMainSeg->WriteMainSegToFile(fhw, NULL, NULL);
		delete pMainSeg;
	}
#endif
	printf("Length of clip in seconds : %f\n", (double)nNumNewlyMadeSampsTotal/UNIFIED_SAMPLING_RATE);

	return xxx;	// OK
}

