#ifndef _AISLE_MANAGE_H_
#define _AISLE_MANAGE_H_

#include "MyPublicFunction.h"
#include "RemoteCmds.h"
#include "uart_api.h"
#include "sqlite3.h"

//------------------------------------------------------MACRO------------------------------------------------//

#define SLAVES_ADDR_CONF			"./conf/slaves_addr/aisle_"

#define UPLOAD_SER_SIZE		256
#define AISLE_LOG_DATA_SIZE	100 * 1024

#define PRO_DATA_OK_FLAG		0x01
#define PRO_DATA_FAILED_FLAG	0x02
#define PRO_FW_UPDATE_FLAG		0x04
#define NULL_DATA_FLAG			0x00

//-----------------------------------------------------MACRO END ---------------------------------------------//

//-------------------------------------------NEW TYPE------------------------------------//

typedef struct _FWInformation
{
	int 	m_SectionSum;
	int 	m_LastSectionSize;
	int 	m_Version;
}FWInformation;

typedef struct _AisleLogData
{
	int 	m_CurPos;
	int 	m_AvailableSpace;
	char	m_Data[AISLE_LOG_DATA_SIZE];
}AisleLogData;

typedef struct _AisleInfo
{
	unsigned char 	m_Flag;	
	int 			m_FwCount;
	int 			m_Aisle;
	unsigned int	m_CurSlavePosition;
	unsigned int	m_SlaveSum;	
	unsigned char	m_SlavesAddrTab[MAX_SLAVE_SUM][SLAVE_ADDR_LEN];
}AisleInfo;

//-----------------------------------------NEW TYPE END-----------------------------------//

//---------------------------------------DECLARATION VARIAVLE--------------------------------------------//

/*
Description			: to store fw information
Default value		: /.
The scope of value	: /.
First used			: /.
*/
extern FWInformation g_FWInfo;

/*
Description			: 1 - full lite mode,0 - half lite mode.
Default value		: 0
The scope of value	: /
First used			: /
*/
extern char g_IsFullMode;

/*
Description			: save some data from aisle.
Default value		: 0
The scope of value	: /
First used			: /
*/
extern AisleLogData g_AisleLogData;

//-------------------------------------DECLARATION VARIABLE END-------------------------------------------//

//--------------------------------------------------DECLARATION FUNCTION----------------------------------------//

extern void 			AisleManageInit();
extern void 			ProcAisleData(int aisle, unsigned char *pData, unsigned int len);
extern int 				IsFwUpdateSuccess(int aisle);
extern void 			ClearFwCount(int aisle);
extern int 				GetCurFwCount(int aisle);
extern void 			SetCurSlavePositionOnTab(int aisle, unsigned int position);
extern unsigned int 	GetCurSlavePositionOnTab(int aisle);
extern unsigned int 	GetSlaveSumOnAisle(int aisle);
extern void 			SetAisleFlag(int aisle, unsigned char flag);
extern unsigned char	GetAisleFlag(int aisle);
extern int 				GetSlavePositionOnTab(int addr, int *pPos ,int aisle);
extern int				GetSlaveAddrByPos(int pos, int aisle);

//-----------------------------------------------DECLARATION FUNCTION END--------------------------------------------//


#endif //-- _AISLE_MANAGE_H_ --//
