#ifndef _MY_PUBLIC_FUNCTION_H_
#define _MY_PUBLIC_FUNCTION_H_

#include <sys/time.h>
#include "MyClientSocket.h"
#include "sqlite3.h"

#ifndef NULL
#define NULL (void*)0
#endif

#define PLATFORM	"mips"

//--------------------------------------MACRO---------------------------------------//

#define TIME					struct timeval
#define GET_SYS_CURRENT_TIME(n)	gettimeofday(&n, NULL)
#define IS_TIMEOUT(n,m)			IsTimeout((int)&n, m)

#define CONV_TO_INT(a,b,c,d) ((a << 24) | (b << 16) | (c << 8) | d)

#if 1
#define L_DEBUG	printf
#else
#define L_DEBUG
#endif

#define MAX_SLAVE_SUM		64	//-- the max number of connecting slaves a aisle --//
#define SLAVE_ADDR_LEN		2	//-- the length of slave address --//
#define INVAILD_SLAVE_ADDR	65535

#define ERR_LOG				 "./data/error.log"
#define SER_ADDR			 "./conf/ser_ip"
#define MID_ID_PATH			 "./conf/mid_id"
#define AISLE_DATA_DB		 "./data/transfer_station.db"
#define AISLE_DATA_TABLE	 "aisle_data_"

//-----------------------------------MACRO END-------------------------------------//

//-------------------------------------------NEW TYPE------------------------------------//


//-----------------------------------------NEW TYPE END-----------------------------------//

//---------------------------------------DECLARATION VARIAVLE--------------------------------------------//

/*
Description			: middleware machine id
Default value		: /
The scope of value	: /
First used			: AppInit()
*/
extern unsigned char g_MyLocalID[11];

/*
Description			: partner machine id(last bit)
Default value		: 0
The scope of value	: /
First used			: AppInit()
*/
extern unsigned char g_PartnerId;

/*
Description			: connect server parameter.
Default value		: /
The scope of value	: /
First used			: AppInit();
*/
extern CNetParameter g_Param8124;

/*
Description			: connect server parameter.
Default value		: /
The scope of value	: /
First used			: AppInit();
*/
extern CNetParameter g_Param8125;

/*
Description			: sqlite db for aisle data.
Default value		: 0
The scope of value	: /
First used			: /
*/
extern sqlite3 *g_PSqlite3Db;

//-------------------------------------DECLARATION VARIABLE END-------------------------------------------//

//--------------------------------------------------DECLARATION FUNCTION----------------------------------------//

extern int 				IsTimeout(int StartTime, unsigned int threshold);
extern void 			Delay_ms(unsigned int xms);
extern unsigned char* 	MyStrStr(unsigned char *pSrc, unsigned int SrcLen, const unsigned char *pDst, unsigned int DstLen, unsigned int *len);
extern int 				CreateCRC16CheckCode_1(unsigned char *pData, unsigned int len);
extern int 				BackupAsciiData(const char *pFileName, unsigned char *pData);
extern int 				ReadFileInfo(const char *pFileName, int *pInfo);
extern void 			l_debug(const char *pLogPath, char *fmt,...);				

//-----------------------------------------------DECLARATION FUNCTION END--------------------------------------------//

#endif

