#include <stdio.h>
#include <string.h>
#include "xProtocol.h"
#include "uart_api.h"
#include "MyPublicFunction.h"

//----------------------Define macro for-------------------//
//---------------------------end---------------------------//


//--------------------Define variable for------------------//

/*
Description			:
Default value		:
The scope of value	:
First used			:
*/

//---------------------------end--------------------------//


//------------Declaration static function for--------------//
//---------------------------end---------------------------//


/***********************************************************************
**Function Name	: CheckCommuData
**Description	: detecting whether the data is correctly.
**Parameter		: pData - in.
				: *pType - out.
**Return		: 0 > - error, other - the length of vaild data.
***********************************************************************/
int CheckCommuData(unsigned char *pData, unsigned char *pType)
{
    int crc_code = 0;
    int len = 0;
    
    if (NULL == pData)
    {
        return -1;
    } 
	
	//--- the length of vaild data ---//
	len = (int)(pData[7] << 8);
	len |= (int)pData[8];

	//--- check end flag ---//
	if (ACK_MSG_FLAG0 != pData[6] || ACK_MSG_FLAG1 != pData[5])
	{
		return -2;
	}

	if (ACK_MSG_FLAG0 != pData[12 + len] || ACK_MSG_FLAG1 != pData[11 + len])
	{
		return -3;
	}

	//--- crc check code ---//
	crc_code = (int)(pData[9 + len] << 8);
	crc_code |= (int)pData[10 + len];

	if (crc_code != (0x0000ffff & CreateCRC16CheckCode_1(&pData[9], len)))
	{
		return -4;
	}

	*pType = pData[4];
	
	return len;
}


/***********************************************************************
**Function Name	: SendFwToSlave
**Description	: send fw to slave.
**Parameter		: pInfo - fw update information.
**Return		: 0 > send failed, 0 - success.
***********************************************************************/
int SendFwToSlave(AisleInfo *pInfo)
{
	FILE *p_read_fw = 0;
	int real_read = 0;
	int read_len = 0;
	int crc_code = 0;
	int res = 0;
	int tmp = 0;
	char fw_path[34] = {0};
	unsigned char fw_buff[AVG_SECTION_FW_SIZE + 10 + SLAVE_ADDR_LEN] = {0};
	
	//--- head flag ---//
	fw_buff[0] = FW_UPDATE_FLAG0;
	fw_buff[1] = FW_UPDATE_FLAG1;
	
	//--- data type ---//
	fw_buff[2] = FW_DATA_TYPE;
	
	//--- slaver address ---//
	memcpy(&fw_buff[3], pInfo->m_SlavesAddrTab[pInfo->m_CurSlavePosition], SLAVE_ADDR_LEN);
	tmp = 3 + SLAVE_ADDR_LEN;
	
	//--- part fw data ---//
	sprintf(fw_path, "%s0.%.3d.fw", FW_0_PATH, g_FWInfo.m_Version);
	
	L_DEBUG("fw_path = %s\n",fw_path);
	
	p_read_fw = fopen(fw_path, "rb");
	
	if (NULL == p_read_fw)
	{
		l_debug(ERR_LOG, "open file failed!\n");
		return -1;
	}
	
	read_len = (pInfo->m_FwCount + 1) < g_FWInfo.m_SectionSum ? AVG_SECTION_FW_SIZE : g_FWInfo.m_LastSectionSize;

	fseek(p_read_fw, (AVG_SECTION_FW_SIZE * pInfo->m_FwCount), SEEK_SET);

	real_read = fread(&fw_buff[tmp], 1, read_len, p_read_fw);

	if (read_len != real_read)
	{
		l_debug(ERR_LOG, "%dth read %s error!\n",pInfo->m_FwCount+1,fw_path);
		fclose(p_read_fw);
		return -2;
	}

	fclose(p_read_fw);

	//--- crc code ---//
	crc_code = CreateCRC16CheckCode_1(&fw_buff[tmp], read_len);
	
	fw_buff[read_len + tmp] = (unsigned char)(crc_code >> 8);
	fw_buff[read_len + tmp + 1] = (unsigned char)crc_code; 
	
	//--- end flag ---//
	fw_buff[read_len + tmp + 2] = FW_UPDATE_FLAG1;
	fw_buff[read_len + tmp + 3] = FW_UPDATE_FLAG0;
	
	//--- send fw ---//
	read_len = (read_len + tmp + 4);
	res = write(pInfo->m_Aisle, fw_buff, read_len);
	L_DEBUG("send fw = %d byte\n",res);
	
	if (res != read_len)
	{
		l_debug(ERR_LOG, "send data %d th failed!\n", pInfo->m_FwCount+1);
		return -3;
	}
	
	return 0;
}

/***********************************************************************
**Function Name	: SendCommunicationRequest
**Description	: send request message to slave.
**Parameter		: fd - in.
				: pSlaveAddr - slave address.
				: type - request type.
**Return		: -1 - failed, 0 - success
***********************************************************************/
int SendCommunicationRequest(int fd, unsigned char *pSlaveAddr, unsigned char type)
{
	unsigned char req_data[5 + SLAVE_ADDR_LEN] = {0};
	int res = 0;

	if (NULL == pSlaveAddr)
	{
		l_debug(ERR_LOG, "%s:param error!\n",__FUNCTION__);
		return -1;
	}  
	
	req_data[0] = REQ_MSG_FLAG0;
	req_data[1] = REQ_MSG_FLAG1;
	
	memcpy(&req_data[2], pSlaveAddr, SLAVE_ADDR_LEN);

    req_data[2 + SLAVE_ADDR_LEN] = type;
    
    req_data[3 + SLAVE_ADDR_LEN] = REQ_MSG_FLAG1;
    req_data[4 + SLAVE_ADDR_LEN] = REQ_MSG_FLAG0;
	
	//--- call uart send api ---//
    res = write(fd, req_data, 7);
	if (7 != res)
	{
		l_debug(ERR_LOG, "%s:send %d request failed!\n",__FUNCTION__, type);
		return -1;
	}
	
	return 0;
}

/***********************************************************************
**Function Name	: SendConfigration
**Description	: send configration to slave.
**Parameter		: fd - in.
				: pSlaveAddr - slave address.
				: type - configuration type.
				: pData - in.
				: len - the length of data.
**Return		: -1 - failed, 0 - success
***********************************************************************/
int SendConfigration(int fd, unsigned char *pSlaveAddr, unsigned char type, unsigned char *pData, unsigned int len)
{
	unsigned char conf_data[111 + SLAVE_ADDR_LEN] = {0};
	int res = 0;
	int tmp = 0;

	if (NULL == pSlaveAddr)
	{
		l_debug(ERR_LOG, "%s:param error!\n",__FUNCTION__);
		return -1;
	}
	
	if (NULL == pData)
	{
		l_debug(ERR_LOG, "%s:param error!\n", __FUNCTION__);
		
		return -1;
	}  
	
	conf_data[0] = REQ_MSG_FLAG0;
	conf_data[1] = REQ_MSG_FLAG1;
	
	memcpy(&conf_data[2], pSlaveAddr, SLAVE_ADDR_LEN);

    conf_data[2 + SLAVE_ADDR_LEN] = type;
    
    conf_data[3 + SLAVE_ADDR_LEN] = REQ_MSG_FLAG1;
    conf_data[4 + SLAVE_ADDR_LEN] = REQ_MSG_FLAG0;
    
    conf_data[5 + SLAVE_ADDR_LEN] = (unsigned char)(len >> 8);
    conf_data[6 + SLAVE_ADDR_LEN] = (unsigned char)len;
    
    memcpy(&conf_data[7 + SLAVE_ADDR_LEN], pData, len);
    
    tmp = CreateCRC16CheckCode_1(pData, len);	//-- get crc code --//
    
    conf_data[7 + SLAVE_ADDR_LEN + len] = (unsigned char)(tmp >> 8);
    conf_data[8 + SLAVE_ADDR_LEN + len] = (unsigned char)tmp;
    
    conf_data[9 + SLAVE_ADDR_LEN + len] = REQ_MSG_FLAG1;
    conf_data[10 + SLAVE_ADDR_LEN + len] = REQ_MSG_FLAG0;
		
	//--- call uart send api ---//
	tmp = 11 + SLAVE_ADDR_LEN + len;
    res = write(fd, conf_data, tmp);
	if (tmp != res)
	{
		l_debug(ERR_LOG, "%s:send %d request failed!\n",__FUNCTION__, type);
		return -1;
	}
	
	return 0;
}





