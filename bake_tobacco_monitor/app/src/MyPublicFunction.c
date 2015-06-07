#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "MyPublicFunction.h"

//--------------------------------------DECLARATION VARIABLE--------------------------------//

//----------------------------------DECLARATION VARIABLE END --------------------------------//

/***********************************************************************
**Function Name	: CreateCRC16CheckCode_1(CRC-16)
**Description	: Create a crc check code
**Parameter		: pData - information to be created a crc check code,
**				: len - the length of information
**Return		: -1 - failed,other value - success
***********************************************************************/
int CreateCRC16CheckCode_1(unsigned char *pData, unsigned int len)
{	
	int crc_code = 0x00;
	int i;

	if( NULL == pData )
	{
		return -1;
	}
	
	while( len-- )
	{
		crc_code = crc_code ^ ( (int)(*pData) << 8 );
		pData++;
		
		for(i = 0; i < 8; ++i)
		{
			if( crc_code & 0x8000 )
			{
				crc_code <<= 1;
				crc_code ^= 0x1021;
			}
			else
			{
				crc_code <<= 1;
			}
		}
	}

	return crc_code;
}

/***********************************************************************
**Function Name	: MyStrStr
**Description	: search sub string
**Parameters	: pSrc - source string
				: SrcLen - the length of source string
				: pDst - sub string 
				: DstLen - the length of sub string
				: len - the remind length of source string
**Return		: NULL - not find, NONE NULL - The position of the first occurrence
***********************************************************************/
unsigned char* MyStrStr(unsigned char *pSrc, unsigned int SrcLen, const unsigned char *pDst, unsigned int DstLen, unsigned int *len)
{
	unsigned int src_i = 0;
	unsigned int dst_i = 0;

	*len = 0;

	if (NULL == pSrc || NULL == pDst)
	{		
		return 0;
	}
	
	for (src_i = 0; src_i < SrcLen; ++src_i)
	{
		if (pSrc[src_i] == pDst[dst_i])
		{
			dst_i++;
			if (DstLen == dst_i)
			{
				*len = SrcLen - src_i + 1;
				return (unsigned char*)&pSrc[src_i - dst_i + 1];
			}
		}
		else if (0 != dst_i)
		{
			dst_i = 0;
			src_i--;
		}
	}
		
	return 0;
}

/***********************************************************************
**Function Name	: BackupAsciiData
**Description	: backup data that the format is ascii.
**Parameter		: pFileName - file name.
				: pData - data.
**Return		: 0 - read ok,-1 - failed.
***********************************************************************/
int BackupAsciiData(const char *pFileName, unsigned char *pData)
{
    int res = 0;
    FILE *fp = NULL;	

	fp = fopen(pFileName, "a");
	
	if (NULL == fp)
	{
		printf("%s:open %s failed!\n", __FUNCTION__, pFileName);
		return -1;
	}
	
	fprintf(fp, "%s\n", pData);
	
	fclose(fp);
			
	return 0;
}

/***********************************************************************
**Function Name	: IsTimeout
**Description	: check whether interval time(start time ~ current time) is over threshold
				: note: min threshold value is 1ms
**Parameters	: StartTime - start time.
				: threshold - Limited
**Return		: 0 - not timeout, 1 - timeout
***********************************************************************/
int IsTimeout(int StartTime, unsigned int threshold)
{
	TIME start = *(struct timeval*)(StartTime);
	TIME end;
	unsigned long interval = 0;

	GET_SYS_CURRENT_TIME(end);
	
	interval = 	1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
   	interval = interval / 1000;
   	
   	//Delay_ms(1);
	
	return (interval > threshold ? 1 : 0);
}

/***********************************************************************
**Function Name	: Delay_ms
**Description	: delay ? ms,but it is not exact.
**Parameters	: xms - in.
**Return		: none.
***********************************************************************/
void Delay_ms(unsigned int xms)
{
	struct timeval delay;
	
	delay.tv_sec = 0;
	delay.tv_usec = xms * 1000;
	
	select(0, NULL, NULL, NULL, &delay);
}

/***********************************************************************
**Function Name	: ReadFileInfo
**Description	: read file information.
**Parameters	: pFileName - in.
				: pInfo - out.
**Return		: 0 - ok, -1 - failed.
***********************************************************************/
int ReadFileInfo(const char *pFileName, int *pInfo)
{
	struct stat stat_info;
	int size = 0;

	if (NULL == pFileName || NULL == pInfo)
	{
		return -1;
	}

	if (stat(pFileName, &stat_info))
	{
		return -1;
	}

	*pInfo = stat_info.st_size;

	return 0;
}

/***********************************************************************
**Function Name	: l_debug
**Description	: print debug information or save it.
**Parameters	: pLogPath - in.
				: fmt - format.
				: param - in.
**Return		: 0 - ok, -1 - failed.
***********************************************************************/
void l_debug(const char *pLogPath, char *fmt,...)
{
	char str[1024] 							= {0};
  	va_list ap;
  	FILE *fp 								= NULL;
  	time_t now_time 						= {0};
	struct tm *p_now_time 					= NULL;

  	va_start(ap, fmt);
  	vsnprintf(str, 250, fmt, ap);
  	va_end(ap);

	printf("%s", str);
	
	if (NULL != pLogPath)	//-- save debug information --//
	{		
		time(&now_time);
		p_now_time = localtime(&now_time);

		fp = fopen(pLogPath, "a");
		if (NULL == fp)
		{
			printf("%s:open %s!\n", __FUNCTION__, pLogPath);
			return;
		}
		
		fprintf(fp, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d %s", (p_now_time->tm_year + 1900),(p_now_time->tm_mon + 1),p_now_time->tm_mday,p_now_time->tm_hour,p_now_time->tm_min,p_now_time->tm_sec, str);
		
		fclose(fp);
	}
	
	return;
}

