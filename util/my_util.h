
#ifndef __MY_UTIL_H001__
#define __MY_UTIL_H001__

// 设关键段检测窗口的宽度（即关键段宽度），以窗口跳步个数计（每跳步固定为 30 毫秒）。
void SetMaxMainSegSize(unsigned short usMaxMainSegSize);

// 输入参数：
//		选段的第一个采样在信号中的序号，和
//		选段包含的采样个数
//		信号采样率
// 输出参数：
//		生成的“选段起始时间和选段时长”字符串存放的位置（缓存不得小于 64 个字节！）
// 
void StartPosAndLengthText(unsigned long ss, unsigned long ll, int srate, char *psOut);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>	// for "FILE *"

// WAV file header
typedef struct {  
	char			root_chunk_id[4];	/* 'RIFF' */
	unsigned long	root_chunk_data_size;	/* length of root chunk */
	char			riff_type_id[4];		/* 'WAVE' */
	char			fmt_chunk_id[4];		/* 'fmt ' */
	unsigned long	fmt_chunk_data_size;	/* length of sub_chunk, always 16 bytes */
	unsigned short	compression_code;		/* always 1 = PCM-Code */
	unsigned short	num_of_channels;		/* 1 = Mono, 2 = Stereo */
	unsigned long	sample_rate;	/* Sample rate */
	unsigned long	byte_p_sec;	/* average bytes per sec */
	unsigned short	byte_p_sample;	/* Bytes per sample */
	unsigned short	bit_p_sample;	/* bits per sample, 8, 12, 16 */
} waveheader_t;

class CWFFile {
private:
	waveheader_t m_header;	// 基本文件头

	FILE *m_pfWaveR;

public:
	int OpenWaveFile(const char *psfn_waveR);
	int MakeTargetSamplesData(int fhw, int&);
	void CloseWaveFile();

public:
	CWFFile();
	virtual ~CWFFile();
};

#endif	// __MY_UTIL_H001__


