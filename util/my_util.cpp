
#include "stdafx.h"

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>	//  _S_IWRITE and _S_IREAD

#include "my_util.h"

// A return value of -1 indicates an error
int OpenFileWr(const char* psfn);
int OpenFileRd(const char* psfn);

// 输入参数：
//		选段的第一个采样在信号中的序号，和
//		选段包含的采样个数
//		信号采样率
// 输出参数：
//		生成的“选段起始时间和选段时长”字符串存放的位置（缓存不得小于 64 个字节！）
// 
void StartPosAndLengthText(unsigned long sss, unsigned long lll, int srate, char *psOut)
{
	// 检测出的关键段的起始位置（毫秒）和段的长度（毫秒）
	unsigned long ul_b;
	unsigned long ul = (double)sss/srate*1000;	// ms
	ul_b = ul/60000;
	sprintf(psOut, "%02d:", ul_b);	// minute
	ul = ul % 60000;

	ul_b = ul/1000;
	sprintf(psOut, "%s%02d:", psOut, ul_b); // second
	ul = ul % 1000;

	sprintf(psOut, "%s%03d; ", psOut, ul);	// ms

	// 段的长度（毫秒），。。。
	ul = (double)lll/srate*1000;	// ms
	ul_b = ul/60000;
	sprintf(psOut, "%s%02d:", psOut, ul_b); // minute
	ul = ul % 60000;

	ul_b = ul/1000;
	sprintf(psOut, "%s%02d:", psOut, ul_b); // second
	ul = ul % 1000;

	sprintf(psOut, "%s%03d", psOut, ul);	// ms
}

int OpenFileWr(const char* psfn)
{
	int fhw = _open(psfn, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, _S_IREAD | _S_IWRITE );
	return fhw;
}
int OpenFileRd(const char* psfn)
{
	int fhr = _open(psfn, _O_RDONLY | _O_BINARY, 0);
	return fhr;
}

/*


*/
