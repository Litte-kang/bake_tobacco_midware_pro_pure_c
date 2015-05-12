#include "AisleManage.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "MyClientSocket.h"
#include "xProtocol.h"

#define INVAILD_SLAVE_ADDR		65535

//----------------------------------------DECLARATION FUNCIONT---------------------------------//

static void 	AlertTypeEvent(int aisle, unsigned char *pData, unsigned int len);
static void 	AckDataTypeEvent(int aisle, unsigned char *pData, unsigned int len);
static void 	StatusTypeEvent(int aisle, unsigned char *pData, unsigned int len);
static void 	UpdateAckTypeEvent(int aisle, unsigned char *pData, unsigned int len);
static void 	CurveTypeEvent(int aisle, unsigned char *pData, unsigned int len);
static int		UpdateSlaveFw(AisleInfo *pInfo);		
static int 		WriteDataToLocal(const char *pFileName, unsigned char *pData, unsigned int len);
static int 		GetAislePositionOnTab(int aisle);

//------------------------------------DECLARATION FUNCIONT END-------------------------------//

//--------------------------------------DECLARATION VARIABLE--------------------------------//


/*
Description			: to store fw information
Default value		: /.
The scope of value	: /.
First used			: AisleManageInit()
*/
FWInformation g_FWInfo = {0};

/*
Description			: 1 - full lite mode,0 - half lite mode.
Default value		: 0
The scope of value	: /
First used			: /
*/
char g_IsFullMode = 0;

/*
Description			: aisles.
Default value		: /.
The scope of value	: /.
First used			: ProtocolEventInit()
*/
static AisleInfo g_AisleInfo[USER_COM_SIZE];

//----------------------------------DECLARATION VARIABLE END --------------------------------//

/***********************************************************************
**Function Name	: AisleManageInit
**Description	: initialize some variable.
**Parameter		: none.
**Return		: none.
***********************************************************************/
void AisleManageInit()
{
	unsigned int slave_sum = 0;
	int slave_addr = 0;
	char slaves_addr_conf[38] = {0};
	int i = 0;
	int n = 0;
	FILE *fp = NULL;
	
	SaveTmpData((void*)0);
	
	//--- get aisle configration information ---//	
	for (i = 0; i < USER_COM_SIZE; ++i)
	{
		memset(g_AisleInfo[i].m_SlavesAddrTab, 0xff, (MAX_SLAVE_SUM*SLAVE_ADDR_LEN));
		
		sprintf(slaves_addr_conf, "%s%.2d", SLAVES_ADDR_CONF, i);
		
		fp = fopen(slaves_addr_conf, "r");
		if (NULL == fp)
		{
			printf("%s:get slaves address failed(%s)!\n", __FUNCTION__, slaves_addr_conf);
			exit(1);
		}
		
		//--- manage slave address ---//
		while(!feof(fp))
		{
			if (0 >= fscanf(fp, "%d\n", &slave_addr))
			{
				printf("slave address is empty!\n");
				break;
			}
			
			if (INVAILD_SLAVE_ADDR == slave_addr)
			{
				printf("a invaild slave addr!\n");
				continue;
			}
			
			g_AisleInfo[i].m_SlavesAddrTab[n][0] = slave_addr >> 8;
			g_AisleInfo[i].m_SlavesAddrTab[n][1] = slave_addr;
		
			L_DEBUG("%.5d\n",slave_addr);	
			n++;
			slave_sum++;
		}		
		
		g_AisleInfo[i].m_Flag = NULL_DATA_FLAG;
		g_AisleInfo[i].m_FwCount = -1;
		g_AisleInfo[i].m_Aisle = g_UartFDS[i];
		g_AisleInfo[i].m_SlaveSum = slave_sum;
		g_AisleInfo[i].m_CurSlavePosition = 0;
		
		L_DEBUG("slave_sum[%d] = %d\n", i, g_AisleInfo[i].m_SlaveSum);
		
		fclose(fp);
	} 
	
}

/***********************************************************************
**Function Name	: UpdateSlaveFw
**Description	: update slave a section fw.
**Parameter		: pInfo - in.
**Return		: ?.
***********************************************************************/
static int	UpdateSlaveFw(AisleInfo *pInfo)
{
	int ret = 0;
	
	ret = SendFwToSlave(pInfo);
	
	return ret;
}

/***********************************************************************
**Function Name	: GetAisleTabPosition
**Description	: get aisle position in g_AisleInfo.
**Parameter		: pData - in.
**Return		: -1 - not exist aisle in g_AisleInfo, i - position.
***********************************************************************/
static int GetAislePositionOnTab(int aisle)
{
	int i = 0;

	for (i = 0; i < USER_COM_SIZE; ++i)
	{
		if (g_AisleInfo[i].m_Aisle == aisle)
		{
			return i;
		}
	}

	return -1;
}

/***********************************************************************
**Function Name	: AlertTypeEvent
**Description	: process alert event.
**Parameter		: aisle - in.
				: pData - alert data.
				: len - the length of valid data.
**Return		: none.
***********************************************************************/
static void AlertTypeEvent(int aisle, unsigned char*pData, unsigned int len)
{
	int pos 				= 0;
    int slave_position 		= 0;
    char data_file_name[20] = {0};
    
    L_DEBUG("Alert data!\n");  
     
	pos = GetAislePositionOnTab(aisle);
	
	slave_position = GetCurSlavePositionOnTab(aisle);
	
	if (0 != memcmp(g_AisleInfo[pos].m_SlavesAddrTab[slave_position], &pData[2], SLAVE_ADDR_LEN) && 0 == g_IsFullMode)
	{
		return;
	}
	
	L_DEBUG("slave(%.5d) alert ack from %d aisle!\n",((int)(pData[2] << 8) | pData[3]),aisle);

	sprintf(data_file_name, "%s%.2d", DATA_FILE, aisle);
	WriteDataToLocal(data_file_name, pData, len);
	
	g_AisleInfo[pos].m_Flag |= PRO_DATA_OK_FLAG;	
}

/***********************************************************************
**Function Name	: AckDataTypeEvent
**Description	: process ack event.
**Parameter		: aisle - in.
				: pData - alert data.
				: len - the length of valid data.
**Return		: none.
***********************************************************************/
static void AckDataTypeEvent(int aisle, unsigned char*pData, unsigned int len)
{
	L_DEBUG("ack data!\n");
	
	{
		int type 			= 0;
		int res 			= 0;
		int slave_position 	= 0;
		
		type = (int)pData[9];
		
		switch (type)
		{
			case CONF_TIME_DATA_TYPE:
			case CONF_TOBA_SIZE_DATA_TYPE:
			case CONF_CURVE_DATA_TYPE:
				{
					res = GetAislePositionOnTab(aisle);
					
					slave_position = GetCurSlavePositionOnTab(aisle);
					
					if (0 == memcmp(g_AisleInfo[res].m_SlavesAddrTab[slave_position], &pData[2], SLAVE_ADDR_LEN))
					{
						L_DEBUG("slave(%.5d) conf ack from %d aisle!\n",((int)(pData[2] << 8) | pData[3]),aisle);
						
						if (1 == pData[10])	//-- configure slave failed --//
						{
							g_AisleInfo[res].m_Flag |= PRO_DATA_FAILED_FLAG;
						}
						else //-- ok --//
						{						
							g_AisleInfo[res].m_Flag |= PRO_DATA_OK_FLAG;
						}
					}
				}
				break;
			case RESTART_SLAVE_DATA_TYPE:
				{
					res = GetAislePositionOnTab(aisle);
					
					slave_position = GetCurSlavePositionOnTab(aisle);
					
					if (0 == memcmp(g_AisleInfo[res].m_SlavesAddrTab[slave_position], &pData[2], SLAVE_ADDR_LEN))
					{
						L_DEBUG("slave(%.5d) restart notice ack from %d aisle!\n",((int)(pData[2] << 8) | pData[3]),aisle);
							
						g_AisleInfo[res].m_Flag |= PRO_DATA_OK_FLAG;
					}					
				}
				break;
			default:
				break;
		}
	}
}

/***********************************************************************
**Function Name	: StatusTypeEvent
**Description	: process status event.
**Parameter		: aisle - in.
				: pData - alert data.
				: len - the length of valid data.
**Return		: none.
***********************************************************************/
static void StatusTypeEvent(int aisle, unsigned char*pData, unsigned int len)    
{
    int pos 				= 0;
    int slave_position 		= 0;
    char data_file_name[20] = {0};
    
    L_DEBUG("Status data!\n");
    
	pos = GetAislePositionOnTab(aisle);
	
	slave_position = GetCurSlavePositionOnTab(aisle);
	
	if (0 != memcmp(g_AisleInfo[pos].m_SlavesAddrTab[slave_position], &pData[2], SLAVE_ADDR_LEN) && 0 == g_IsFullMode)
	{
		return;
	}
	
	L_DEBUG("slave(%.5d) status ack from %d aisle!\n",((int)(pData[2] << 8) | pData[3]),aisle);

	sprintf(data_file_name, "%s%.2d", DATA_FILE, aisle);
	WriteDataToLocal(data_file_name, pData, len);
	
	g_AisleInfo[pos].m_Flag |= PRO_DATA_OK_FLAG;	
}

/***********************************************************************
**Function Name	: UpdateAckTypeEvent
**Description	: process update ack event.
**Parameter		: aisle - in.
				: pData - alert data.
				: len - the length of valid data.
**Return		: none.
***********************************************************************/
static void UpdateAckTypeEvent(int aisle, unsigned char *pData, unsigned int len)
{
	unsigned char flag[2] = {FW_UPDATE_FLAG0, FW_UPDATE_FLAG1};
	unsigned char *p = NULL;
	AisleInfo *p_info = NULL;
	int res = 0;
	int tmp = 0;
	int type = 0;

	if (NULL == pData)
	{
		return;
	}
	
	p = pData;
	
	res = GetAislePositionOnTab(aisle);
	
	p_info = &g_AisleInfo[res];

	while (NULL != p)
	{
		p = MyStrStr(p, len, flag, 2, &len);
			
		if (NULL != p)
		{						
			if (FW_UPDATE_FLAG1 == p[5] && FW_UPDATE_FLAG0 == p[6] 
				&& (0 == memcmp(p_info->m_SlavesAddrTab[p_info->m_CurSlavePosition] ,&p[3], SLAVE_ADDR_LEN)))
			{
				L_DEBUG("update type = 0x%x\n", p[2]);
				
				type = (int)p[2];
				
				res = 1; //-- default correct type --//

				switch (type)
				{
					case ACK_NOTICE_UPDATE_YES_TYPE:
						p_info->m_FwCount = 0;
						tmp = UpdateSlaveFw(p_info);
						L_DEBUG("--------------------------sent %d th section\n",p_info->m_FwCount+1);
						break;
					case ACK_FW_DATA_SUC_TYPE:
						p_info->m_FwCount++;					
						tmp = UpdateSlaveFw(p_info);
						L_DEBUG("--------------------------sent %d th section\n",p_info->m_FwCount+1);
						break;
					case ACK_FW_DATA_FAILED_TYPE:
						L_DEBUG("--------------------------sent %d th failed ,send againt\n",p_info->m_FwCount+1);
						tmp = UpdateSlaveFw(p_info);					
						break;
					case UPDATE_SUC_TYPE:
						L_DEBUG("--------------------------send end! total %d sector\n",p_info->m_FwCount+1);
						p_info->m_FwCount++;	
						break;
					case ACK_NOTICE_UPDATE_NO_TYPE:
						L_DEBUG("--------------------------%.5d is latest fw\n",((int)(p[3] << 8) | p[4]));
						tmp = 2;																					
						break;
					default:
						res = -1;	//-- error type --//
						break;
				}
			}//-- if (FW_UPDATE_FLAG1 == p[5] && FW_UPDATE_FLAG0 == p[6]) --//
			
			if (1 == res)
			{
				
				if (2 == tmp)
				{
					p_info->m_FwCount = g_FWInfo.m_SectionSum;
				}
				
				if (0 == memcmp(p_info->m_SlavesAddrTab[p_info->m_CurSlavePosition] ,&p[3], SLAVE_ADDR_LEN))
				{
					L_DEBUG("slave(%.5d) update ack from %d aisle!\n", ((int)(p[3] << 8) | p[4]), p_info->m_Aisle);
						
					p_info->m_Flag |= PRO_DATA_OK_FLAG;
				}
									
				break;
			}
			
			p++;
			len--;
		} //-- if (NULL != p) --//
	} //-- end of while (NULL != p) --//
}

/***********************************************************************
**Function Name	: CurveTypeEvent
**Description	: process curve ack event.
**Parameter		: aisle - in.
				: pData - alert data.
				: len - the length of valid data.
**Return		: none.
***********************************************************************/
static void CurveTypeEvent(int aisle, unsigned char *pData, unsigned int len)
{
    int pos 				= 0;
    int slave_position 		= 0;
    char data_file_name[20] = {0};
    
    L_DEBUG("Curve data!\n");
    
	pos = GetAislePositionOnTab(aisle);
	
	slave_position = GetCurSlavePositionOnTab(aisle);
	
	if (0 != memcmp(g_AisleInfo[pos].m_SlavesAddrTab[slave_position], &pData[2], SLAVE_ADDR_LEN) && 0 == g_IsFullMode)
	{
		return;
	}
	
	L_DEBUG("slave(%.5d) curve ack from %d aisle!\n",((int)(pData[2] << 8) | pData[3]),aisle);

	sprintf(data_file_name, "%s%.2d", DATA_FILE, aisle);
	WriteDataToLocal(data_file_name, pData, len);
	
	g_AisleInfo[pos].m_Flag |= PRO_DATA_OK_FLAG;		
}

/***********************************************************************
**Function Name	: UploadDataToServer
**Description	: upload data to server.
**Parameter		: pFileName - file name.
				: pData - data.
				: len - the length of data.
**Return		: 0 - ok, ohter - failed.
***********************************************************************/
static int WriteDataToLocal(const char *pFileName, unsigned char *pData, unsigned int len)
{
	unsigned int tmp = 0;
	unsigned char upload_buff[UPLOAD_SER_SIZE] = {0};
	unsigned int slave_addr = 0;
	
	tmp = (unsigned int)pData[4];
	
	slave_addr = (unsigned int)(pData[2] << 8);
	slave_addr |= (unsigned int)pData[3];
	
	switch (tmp)
	{
		case ALERT_DATA_TYPE:	//-- alert data --//
			{
				sprintf(upload_buff, "{\"midAddress\":\"%s\",\"type\":%d,\"address\":\"%.5d\",\"data\":[%d,%d,%d,%d,%d,%d,%d,%d]}", 
				g_MyLocalID, tmp, slave_addr, pData[9], pData[10], pData[11], pData[12], pData[16], pData[15], pData[14], pData[13]);
			}
			break;
		case STATUS_DATA_TYPE:	//-- status data --//
			{
				sprintf(upload_buff, "{\"midAddress\":\"%s\",\"type\":%d,\"address\":\"%.5d\",\"isBelow\":%d,\"data\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}", 
				g_MyLocalID, tmp, slave_addr, ((pData[19] & 0x08) >> 3), pData[9], pData[10], pData[11], pData[12], pData[13], pData[14], pData[15], pData[16], pData[17], pData[18], pData[19]);
			}
			break;
		case CURVE_DATA_TYPE:	//-- curve data --//
			{
				float wet_value[10] = {0};
				int i = 0;
				
				for (i = 0; i < 20; i += 2)
				{
					tmp = (int)(pData[20 + i] << 8);
					tmp |= (int)pData[20 + i + 1];
					wet_value[i / 2] = (float)tmp / 10;
				}				
				
				tmp = 9;
				
				sprintf(upload_buff, "{\"midAddress\":\"%s\",\"type\":%d,\"address\":\"%.5d\",\"data\":[%d,[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],[%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f],[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],%d]}", 
									g_MyLocalID, tmp, slave_addr, ((pData[9] >> 3) == 20 ? 19 : (pData[9] >> 3)), 
									pData[10], pData[11], pData[12], pData[13], pData[14], pData[15], pData[16], pData[17], pData[18], pData[19],
									wet_value[0],wet_value[1],wet_value[2],wet_value[3],wet_value[4],wet_value[5],wet_value[6],wet_value[7],wet_value[8],wet_value[9],
									pData[40],pData[41],pData[42],pData[43],pData[44],pData[45],pData[46],pData[47],pData[48],pData[49],pData[50],pData[51],pData[52],
									pData[53],pData[54],pData[55],pData[56],pData[57],pData[58],(pData[9] & 0x07));
			}		
			break;
		default:
			return -1;
			break;
	}
	
	tmp = strlen(upload_buff);
	
	L_DEBUG("json(%d):%s\n", tmp, upload_buff);
	
	BackupAsciiData(pFileName, upload_buff);

	memcpy(&upload_buff[tmp], "</br>", 5);
	SaveTmpData(upload_buff);
	
	return tmp;
}


/***********************************************************************
**Function Name	: ProcAisleData
**Description	: process data from aisle(uart).
**Parameter		: aisle - in
				: pData - protocol data.
                : len - the length of uart data.
**Return		: none.
***********************************************************************/
void ProcAisleData(int aisle, unsigned char *pData, unsigned int len)
{
    char flag[2] = {ACK_MSG_FLAG0,ACK_MSG_FLAG1}; 
	unsigned char *p = NULL;
	int res = 0;
	unsigned char type = 0;

	if (NULL == pData)
    {
        return;
    }
    
	p = pData;
		
	res = GetAislePositionOnTab(aisle);
	
	if (-1 != res)
	{
		if (PRO_FW_UPDATE_FLAG == (PRO_FW_UPDATE_FLAG & g_AisleInfo[res].m_Flag))
		{					
			UpdateAckTypeEvent(aisle, p, len);	
			
			return;
		}
	}
	
 	while (NULL != p)
	{
        p = MyStrStr(p, len, flag, 2, &len);
         
		res = CheckCommuData(p, &type);
		
		L_DEBUG("vaild data len = %d\n", res);

        if (0 <= res)
        {
            L_DEBUG("communication type = %d \n",type);
            
			switch (type)
			{
				case ALERT_DATA_TYPE:
					{					
						AlertTypeEvent(aisle, p, res);
					}											
					break;
				case ACK_DATA_TYPE:
					{
						AckDataTypeEvent(aisle, p, res);
					}
					break;
				case STATUS_DATA_TYPE:
					{
						StatusTypeEvent(aisle, p, res);
					}
					break;
				case CURVE_DATA_TYPE:
					{
						CurveTypeEvent(aisle, p, res);
					}
					break;
				default:
					break;   
			}
			
			p = p + 12 + res;		//-- p = p + (13 -1) + res --//
			len = len - 12 - res;	//-- len = len - (13 - 1) - res --//
        } //-- end of if (0 <= res) --//
         
        if (NULL != p)
        {
            p++;
            len--;
        }		
	}	
}

/***********************************************************************
**Function Name	: IsFWUpdateSuccess
**Description	: check whether update is successful by aisle.
**Parameter		: aisle - in
**Return		: 1 - updating, 0 - update successful, -1 - error aisle.
***********************************************************************/
int IsFWUpdateSuccess(int aisle)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);

	if (-1 == res)
	{
		printf("error aisle\n");
		return -1;
	}

	return (g_AisleInfo[res].m_FwCount < g_FWInfo.m_SectionSum ? 1 : 0);
}

/***********************************************************************
**Function Name	: ClearFwCount
**Description	: clear fw count by aisle
**Parameter		: aisle - in
**Return		: none.
***********************************************************************/
void ClearFwCount(int aisle)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("error aisle\n");
		
		return;
	}
	
	g_AisleInfo[res].m_FwCount = -1;
}

/***********************************************************************
**Function Name	: GetCurFwCount
**Description	: get fw count by aisle
**Parameter		: aisle - in
**Return		: -1 - failed, count.
***********************************************************************/
int GetCurFwCount(int aisle)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("error aisle\n");
		
		return;
	}	
	
	return g_AisleInfo[res].m_FwCount;
}

/***********************************************************************
**Function Name	: SetAisleFlag
**Description	: set aisle flag.
**Parameter		: aisle - in.
				: flag - in.
**Return		: none.
***********************************************************************/
void SetAisleFlag(int aisle, unsigned char flag)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("error aisle\n");
		return;
	}
	
	if (NULL_DATA_FLAG == flag)
	{
		g_AisleInfo[res].m_Flag = NULL_DATA_FLAG;
	}
	else
	{
		g_AisleInfo[res].m_Flag |= flag;
	}
}

/***********************************************************************
**Function Name	: SetCurSlavePositionOnTab
**Description	: set current slave table position by aisle.
**Parameter		: aisle - in.
				: position - in.
**Return		: none.
***********************************************************************/
void SetCurSlavePositionOnTab(int aisle, unsigned int position)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("SetCurCommuInfoTabPosition:error aisle\n");
		return;
	}
	
	g_AisleInfo[res].m_CurSlavePosition = position;	
}

/***********************************************************************
**Function Name	: GetCurSlavePositionOnTab
**Description	: get current slaver table position.
**Parameter		: aisle - in.
**Return		: none.
***********************************************************************/
unsigned int GetCurSlavePositionOnTab(int aisle)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("GetCurCommuInfoTabPosition:error aisle\n");
		return -1;
	}
	
	return g_AisleInfo[res].m_CurSlavePosition;		
}

/***********************************************************************
**Function Name	: GetSlaveSumOnAisle
**Description	: get the number of slave on a aisle.
**Parameter		: aisle - in.
**Return		: none.
***********************************************************************/
unsigned int GetSlaveSumOnAisle(int aisle)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("GetCommuInfoTabSlaverSum:error aisle\n");
		return -1;
	}
	
	return g_AisleInfo[res].m_SlaveSum;	
}

/***********************************************************************
**Function Name	: GetAisleFlag
**Description	: get aisle flag.
**Parameter		: aisle - in.
**Return		: aisle flag.
***********************************************************************/
unsigned char GetAisleFlag(int aisle)
{
	int res = 0;

	res = GetAislePositionOnTab(aisle);
	
	if (-1 == res)
	{
		printf("IsRecCommuDat:error aisle\n");
		return -1;
	}
	
	return g_AisleInfo[res].m_Flag;		
}

/***********************************************************************
**Function Name	: GetSlavePositionOnTab
**Description	: find position of slave addr in g_AisleInfo[i].m_SlavesAddrTab.
**Parameter		: addr - in.
				: pPos - position.
				: aisle - in.
**Return		: -1 - failed, 0 - success
***********************************************************************/
int GetSlavePositionOnTab(int addr, int *pPos ,int aisle)
{
	int i = 0;
	int pos = 0;
	unsigned char tmp_addr[SLAVE_ADDR_LEN] = {0};
	
	if (NULL == pPos)
	{
		return -1;
	}
	
	tmp_addr[0] = (unsigned char)(addr >> 8);
	tmp_addr[1] = (unsigned char)addr;
	
	pos = GetAislePositionOnTab(aisle);
	
	for (i = 0; i < g_AisleInfo[pos].m_SlaveSum; ++i)
	{
		if (0 == memcmp(g_AisleInfo[pos].m_SlavesAddrTab[i], tmp_addr, SLAVE_ADDR_LEN))
		{
			*pPos = i;
			return 0;
		}
	}
	
	return -1;
}

/***********************************************************************
**Function Name	: SaveTmpData
**Description	: save 1000 data.
**Parameter		: pData - in.
**Return		: none.
***********************************************************************/
void SaveTmpData(unsigned char *pData)
{
	FILE *fp = NULL;
	static short counter = 1000;
	
	counter++;
		
	if (1000 >= counter)
	{
		BackupAsciiData("/tmp/tmp.log", pData);
	}
	else
	{
		fp = fopen("/tmp/tmp.log", "w");
		fprintf(fp, "{\"tmp.log\"}</br>\n");
		fclose(fp);
		counter = 0;
	}
}

/***********************************************************************
**Function Name	: GetSlaveAddrByPos
**Description	: get a slave address by position.
**Parameter		: pos - in.
				: ailse - in.
**Return		: slave address.
***********************************************************************/
int	GetSlaveAddrByPos(int pos, int aisle)
{
	int n = 0;
	int address = 0;
	
	
	n = GetAislePositionOnTab(aisle);
	
	address = (int)g_AisleInfo[n].m_SlavesAddrTab[pos][0];
	address <<= 8;
	address |= (int)g_AisleInfo[n].m_SlavesAddrTab[pos][1];
	
	return address;
}










