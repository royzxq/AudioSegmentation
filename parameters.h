
#ifndef	__PARAMETERS_H__
#define __PARAMETERS_H__
#include "mydefs.h"

// 找一个音频的关键段时，检测窗口跳步大小
#define MILISECONDS_DETECT_SKIP		30.0
// 关键段检测时，短于这个长度（以秒计）的音乐文件作废
#define MINIMUM_MUSIC_LENGTH_ALLOWED_IN_SECONDS		0.64	// 与 "MINIMUM_NUM_FRAMES_IN_UNIT" 对应！！！

// 信号值的绝对值平均值小于这个值即为“弱音”，会被“掐头去尾”！
#define MINIMUM_MEAN_ABS_SIGNAL_VALUE		81.9175	// 32767(the largest abs value of signal)*0.25%

// ////////////////////////////////////////////////////////////////////////////////////////////////

#define UNIFIED_SAMPLING_RATE		22050.0		// 统一采样频率（目标采样率）

#define	NumBinsInFftWinM			512	//1024
#define NumSamplesInFrameM			512	//882
#define	NumSamplesInFrameSkipM		441	// 必须小于等于 "NumSamplesInFrameM" ！！！

#define MILISECONDS_PER_FRAME_SKIP	20

#define NUMBER_OF_OCTAVES			7

//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

/*
#define CCCC_FACTOR				(1.0E+3)
*/

// 2013/02/27 发现 10 维效果稍好点
#define NumDimsAreaMomentsM		10	//15

#define NumDimsMfccM			13
#define NUM_MEL_BANDS			30	// number of Mel bands

#define NUM_DIMS_LPC			13

#ifdef __USE_STE_FEATURES
#define NUM_STE_FEATURES		5	//4
#endif

#ifdef __USE_SPECTRAL_SHAPE_FEATURES
#define NUM_DIMS_SPECTRAL_SHAPE		14	//7	//4
#endif

#ifdef __USE_BEAT_HISTOGRAM
#define NUM_DIMS_BH				13	//10
#endif

#ifdef __USE_MODULATION_SPECTRUM
#define NUM_DIMS_MSP			25
#endif	// __USE_MODULATION_SPECTRUM

#ifdef __USE_ZERO_CROSSING
#define NUM_DIMS_ZC				1
#endif	// __USE_ZERO_CROSSING

#ifdef __USE_SUB_BAND_ENERGY
#ifdef __USE_MEL_BAND_VALUE_AS_AM_SOURCE
#define NUM_DIMS_SBE			13
#else
#define NUM_DIMS_SBE			NUMBER_OF_OCTAVES
#endif	// __USE_MEL_BAND_VALUE_AS_AM_SOURCE
#endif	// __USE_SUB_BAND_ENERGY

#if defined(__USE_BEAT_HISTOGRAM) || defined(__USE_MODULATION_SPECTRUM)
#define BH_FFT_WIN_WIDTH		64	// 必须是 2 的正整数次方
#endif

// AM 源数据矩阵中包含的帧数
#define NumFramesAreaMomentsSrcM	8

// 注意：这个值必须是 "NumBinsInFftWinM" 的一半！！！
#define WIDTH_SOURCE_DATA_AM		256

//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

// 训练和分类必须用同样的参数值，更合理一些！
// 一个单元中包含的帧跳步个数
#define NumFrameSkipsInUnitM		60	// 帧跳步为 20 毫秒，合 1.2 秒

#ifdef __TRAINING_PHASE
// 从一个音频文件可能生成多个训练样例（每个单元对应一个训练样例）
#define NumFrameSkipsInUnitSkipM	60	// 单元跳步大小（以帧跳步数计）。帧跳步为 20 毫秒，合 1.2 秒
#define MINIMUM_NUM_FRAMES_IN_UNIT	30	// 帧数，训练样本的长度不能小于此值（否则舍弃）！
#else	// __TRAINING_PHASE
#define NumFrameSkipsInUnitSkipM	60	//20	// 单元跳步大小（以帧跳步数计）。帧跳步为 20 毫秒，合 0.4 秒
#define MAX_NUM_UNITS				9000	// 待分段音频长度限制（60分*60秒/分*1000毫秒/秒）/（20毫秒/帧*20帧）

// 这么多个 units 覆盖的范围（即对应的时长）
//	NumFrameSkipsInUnitM+NumFrameSkipsInUnitSkipM*(MINMUN_NUM_UNITS_IN_SEGMENT-1)
//	按各个相关参数值的当前设置，为 60+20*(3-1) = 100 帧跳，合 2.00 秒
// 段长小于这个值，就被认为是噪声，将被并入相邻段
#define MINMUN_NUM_FRAMES_IN_SEGMENT			100	//3
// 静音段的长度（以单元跳步个数计）大于这个值，才保留，否则将被合并入相邻段
#define NUM_FRAMES_IN_SILENT_SEGMENT_TO_SURVIVE	160	//8

// 存放待分段音频的帧能量值的文件
#define FN_TEMP_STE		"~ste"

#endif	// #ifdef __TRAINING_PHASE

// 一个单元中，最长静音段的长度大于单元总长的一定比例时，则判为静音单元
#define RATIO_PERCENT_SILENT_LENGTH		0.8

// 此值设定视将有效频率范围分为几个频带而定
#ifdef __USE_MEL_BAND_VALUE_AS_AM_SOURCE
#define NUM_DIMS_AM_SORCE_DATA			NUM_MEL_BANDS
#else
#define NUM_DIMS_AM_SORCE_DATA			NUMBER_OF_OCTAVES
#endif	// __USE_MEL_BAND_VALUE_AS_AM_SOURCE

#endif	// __PARAMETERS_H__


