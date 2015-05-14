#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "EventActionSet.h"
#include "AsyncEvents.h"
#include "MyPublicFunction.h"
#include "AisleManage.h"
#include "xProtocol.h"

//----------------------Define macro for-------------------//

#define COUNTER(N,M) \
{\
	if (N > M)\
	{\
		counter++;\
	}\
}\

//---------------------------end---------------------------//


//--------------------Define variable for------------------//

/*
Description			: save tmp log.
Default value		: \
The scope of value	: \
First used			: \
*/
unsigned char g_TmpLog[80] = {0};

//---------------------------end--------------------------//


//------------Declaration static function for--------------//

static void 	UploadData(int aisle);
static int	 	UploadFeedbacks(int type, unsigned char *pData, unsigned int len, int aisle, int socektFd);

//---------------------------end---------------------------//

/***********************************************************************
**Function Name	: UploadFeedbacks
**Description	: upload feedbacks to server.
**Parameter		: type - in.
				: pFileName - backup file name.
				: pData - data.
				: len - the length of data.
				: aisle - in.
				: socketFd - in.
**Return		: 0 - ok, ohter - failed.
***********************************************************************/
static int UploadFeedbacks(int type, unsigned char *pData, unsigned int len, int aisle, int socektFd)
{
	unsigned char json_str[100] = {0};
	char data_file_name[20] 	= {0};
	int slave_address 			= 0;
	int res 					= 0;
	int i 						= 0;
	
	slave_address = pData[0];
	slave_address <<= 8;
	slave_address |= pData[1];
	
	sprintf(json_str, "{\"midAddress\":\"%s\",\"type\":%d,\"address\":\"%.5d\",\"data\":[", g_MyLocalID, type, slave_address);
		
	len = len - 2;
	
	for (i = 0; i < len; ++i)
	{
		res = strlen(json_str);
		sprintf(&json_str[res],"%d,", pData[2 + i]);
	}
	
	res = strlen(json_str);
	
	json_str[res - 1] = ']';
	json_str[res] = '}';
	
	if (0 <= socektFd)
	{
		res = SendDataToServer(socektFd, json_str, (res + 1));
	}
	
	if (0 != res)
	{
		sprintf(data_file_name, "%s%.2d", DATA_FILE, aisle);
		BackupAsciiData(data_file_name, json_str);
	}
		
	return res;
}


/***********************************************************************
**Function Name	: UploadData
**Description	: upload data to server.
**Parameter		: aisle - in.
**Return		: none.
***********************************************************************/
static void UploadData(int aisle)
{
	FILE *fp 									= NULL;
	unsigned char upload_buf[UPLOAD_SER_SIZE] 	= {0};
	int socket_fd 								= -1;
	char data_file_name[20] 					= {0};
	
	L_DEBUG("start a connection!\n");
		
	socket_fd = ConnectServer(1, g_Param8124);	

	if(0 <= socket_fd)
	{
		sprintf(data_file_name, "%s%.2d", DATA_FILE, aisle);

		fp = fopen(data_file_name, "r");

		if (NULL != fp)
		{
			do
			{
				fscanf(fp, "%s\n", upload_buf);
	
				if (0 != upload_buf[0])
				{
					SendDataToServer(socket_fd, upload_buf, strlen(upload_buf));
				}
	
			}while(!feof(fp));

			fclose(fp);
	
			remove(data_file_name);				
		}

		SendDataToServer(socket_fd, upload_buf, strlen(upload_buf));
		if(RecDataFromServer(socket_fd, upload_buf, 256, 1)) //-- wait server closing connection --//
		{
			sleep(1);
			LogoutClient(socket_fd);
		}	
	}	
}

/***********************************************************************
**Function Name	: GetSlaveBaseInfo
**Description	: send get request.
**Parameters	: arg - in.
**Return		: 0 - ok, other value - error.
***********************************************************************/
void SendDataReq(int arg)
{
	EventParams param 				= (*(EventParams*)arg); 
	unsigned int res 				= 0;
	unsigned int slave_sum 			= 0;
	unsigned int position 			= 0;
	unsigned int send_again_counter = 0;
	unsigned int timeout 			= 0;
	unsigned int counter 			= 0;
	unsigned char address[SLAVE_ADDR_LEN] 	= {0};
	unsigned char data_status 				= 0;
	TIME start;
	
	SetCurSlavePositionOnTab(param.m_Aisle, 0);
	SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);

	//--- get slaver sum and start position on param.m_Aisle ---//
	if (0x0000ffff != param.m_Body.m_Id)
	{
		if (!GetSlavePositionOnTab(param.m_Body.m_Id, &position, param.m_Aisle))
		{
			slave_sum = position + 1;
		}
		else
		{
			position = slave_sum;
		}
	}
	else
	{
		slave_sum = GetSlaveSumOnAisle(param.m_Aisle);
		position = GetCurSlavePositionOnTab(param.m_Aisle);	
	}

	while (position < slave_sum)
	{	
	
		SetCurSlavePositionOnTab(param.m_Aisle, position);
		
		res = GetSlaveAddrByPos(position, param.m_Aisle);
					
		L_DEBUG("send a req(%d) to %.5d by %d aisle\n", param.m_Type, res, param.m_Aisle);
		
		address[0] = (unsigned char)(res >> 8);
		address[1] = (unsigned char)res;
		
		SendCommunicationRequest(param.m_Aisle, address, param.m_Type);			
					
		GET_SYS_CURRENT_TIME(start);
		
		do
		{	
			Delay_ms(5);
			
			if ((FORCE_END_ASYNC_EVT_FLAG & g_AsyncEvtFlag))
			{
				L_DEBUG("force end cur cmd\n");
				break;
			}
			
			data_status = GetAisleFlag(param.m_Aisle);
			
			if (NULL_DATA_FLAG == data_status)
			{

				res = IS_TIMEOUT(start, (1500));	//-- we will send notice again slave, if not receive ack in (2)s --//
				if (0 != res)
				{
					printf("%s:receive %.5d req(%d) ack timeout!\n", __FUNCTION__,((int)(address[0] << 8) | address[1]), param.m_Type);
					sprintf(g_TmpLog, "receive %.5d req(%d) ack timeout!</br>",((int)(address[0] << 8) | address[1]), param.m_Type);
					SaveTmpData(g_TmpLog);
					SendCommunicationRequest(param.m_Aisle, address, param.m_Type);
					GET_SYS_CURRENT_TIME(start);
					send_again_counter++;
				}

			}
			else if (PRO_DATA_FAILED_FLAG == data_status)
			{
				SendCommunicationRequest(param.m_Aisle, address, param.m_Type);
				GET_SYS_CURRENT_TIME(start);
				SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
			}
			else
			{
				
				while (!(PRO_DATA_OK_FLAG & GetAisleFlag(param.m_Aisle)))	//-- wait --//
				{
					Delay_ms(5);
				}
				
				break;	//-- receive ack --//
			}
					
		}while(3 > send_again_counter);  //-- we will notice next slave, if we have sent 4 times --//

		if ((FORCE_END_ASYNC_EVT_FLAG & g_AsyncEvtFlag))
		{
			break;
		}
							
		position++;
		COUNTER(3, send_again_counter);
		send_again_counter = 0;	
		SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);		
	}

	SetCurSlavePositionOnTab(param.m_Aisle, 0);
	SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
	
	UploadData(param.m_Aisle);
	
	L_DEBUG("===========================================\n");
	L_DEBUG("%d slaves req data successful by %d aisle!\n", counter, param.m_Aisle);
	L_DEBUG("===========================================\n");		
}

/***********************************************************************
**Function Name	: SendConfigData
**Description	: configure slave.
**Parameter		: arg - in.
**Return		: none.
***********************************************************************/
void SendConfigData(int arg)
{
	EventParams param 		= (*(EventParams*)arg); 
	unsigned int res 		= 0;
	unsigned int slave_sum 	= 0;
	unsigned int position 	= 0;
	unsigned int send_again_counter 		= 0;
	unsigned char data[1 + SLAVE_ADDR_LEN] 	= {0};
	unsigned char address[SLAVE_ADDR_LEN] 	= {0};
	unsigned int counter 		= 0;
	unsigned char data_status 	= 0;
	int	socket_fd				= 0;
	char evt_types[3] 			= {ALERT_DATA_TYPE, STATUS_DATA_TYPE, CURVE_DATA_TYPE};
	TIME start;
	EventParams next_evt_param;
	
	res = param.m_Body.m_RemoteCmd.m_Addr[0];
	res <<= 8;
	res |= param.m_Body.m_RemoteCmd.m_Addr[1];
	
	SetCurSlavePositionOnTab(param.m_Aisle, 0);
	SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);

	//--- get slave sum and start position on param.m_Aisle ---//	
	if (0x0000ffff != res)
	{
		if (!GetSlavePositionOnTab((int)res, &position, param.m_Aisle))
		{
			slave_sum = position + 1;
		}
		else
		{
			position = slave_sum;
		}
	}
	else
	{
		slave_sum = GetSlaveSumOnAisle(param.m_Aisle);
		position = GetCurSlavePositionOnTab(param.m_Aisle);		
	}

	socket_fd = ConnectServer(1, g_Param8124);

	while (position < slave_sum)
	{	
	
		SetCurSlavePositionOnTab(param.m_Aisle, position);	//-- set the current slave address table position --//	
		
		res = GetSlaveAddrByPos(position, param.m_Aisle);
			
		L_DEBUG("send conf data to %.5d by %d aisle\n",res, param.m_Aisle);
		
		address[0] = (unsigned char)(res >> 8);
		address[1] = (unsigned char)res;
		
		memcpy(data, address, SLAVE_ADDR_LEN);	
		
		SendConfigration(param.m_Aisle, address, param.m_Body.m_RemoteCmd.m_Type, param.m_Body.m_RemoteCmd.m_Data, param.m_Body.m_RemoteCmd.m_DataLen);
					
		GET_SYS_CURRENT_TIME(start);
		
		do
		{
			Delay_ms(5);
			
			if ((FORCE_END_ASYNC_EVT_FLAG & g_AsyncEvtFlag))
			{
				L_DEBUG("force end cur cmd\n");
				break;
			}	
			
			data_status = GetAisleFlag(param.m_Aisle);
			
			if (NULL_DATA_FLAG == data_status)
			{
				res = IS_TIMEOUT(start, (1500));	//-- we will send request again slave, if not receive ack in 5s --//
				if (0 != res)
				{
					printf("%s:receive %.5d conf ack timeout!\n", __FUNCTION__, ((int)(address[0] << 8) | address[1]));
					sprintf(g_TmpLog, "receive %.5d conf ack timeout!</br>",((int)(address[0] << 8) | address[1]));
					SaveTmpData(g_TmpLog);
					
					SendConfigration(param.m_Aisle, address, param.m_Body.m_RemoteCmd.m_Type, param.m_Body.m_RemoteCmd.m_Data, param.m_Body.m_RemoteCmd.m_DataLen);
					
					GET_SYS_CURRENT_TIME(start);
					send_again_counter++;
					
					data[2] = 0;	//-- conf failed --//
				}
			}
			else if (PRO_DATA_FAILED_FLAG == data_status)
			{
				SendConfigration(param.m_Aisle, address, param.m_Body.m_RemoteCmd.m_Type, param.m_Body.m_RemoteCmd.m_Data, param.m_Body.m_RemoteCmd.m_DataLen);
				GET_SYS_CURRENT_TIME(start);
				SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
			}
			else
			{
				data[2] = 1;	//-- conf ok --//
				
				while (!(PRO_DATA_OK_FLAG & GetAisleFlag(param.m_Aisle)))	//-- wait --//
				{
					Delay_ms(5);
				}
				
				break;	//-- receive ack --//
			}
				
		}while(3 > send_again_counter);  //-- we will notice next slave, if we have sent 3 times --//

		if ((FORCE_END_ASYNC_EVT_FLAG & g_AsyncEvtFlag))
		{
			break;
		}
					
		//--- tell server what we modify ok ---//
		res = UploadFeedbacks(param.m_Body.m_RemoteCmd.m_Type, data, (1 + SLAVE_ADDR_LEN), param.m_Aisle, socket_fd);
		//--- end ---//	
		
		SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
		position++;
		COUNTER(3, send_again_counter);
		send_again_counter = 0;
	}

	SetCurSlavePositionOnTab(param.m_Aisle, 0);
	SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
	
	L_DEBUG("===========================================\n");
	L_DEBUG("%d slaves configure successful by %d aisle!\n", counter, param.m_Aisle);
	L_DEBUG("===========================================\n");

	Delay_ms(1500);
	LogoutClient(socket_fd);

	if (counter)
	{
		next_evt_param.m_Aisle = param.m_Aisle;	
		next_evt_param.m_Body.m_Id = param.m_Body.m_RemoteCmd.m_Addr[0];
		next_evt_param.m_Body.m_Id <<= 8;
		next_evt_param.m_Body.m_Id |= param.m_Body.m_RemoteCmd.m_Addr[1];

		for (res = 0; res < 3; ++res)
		{
			next_evt_param.m_Type = evt_types[res];
			
			SendDataReq((int)&next_evt_param);
		}		
	}
	
}

/***********************************************************************
**Function Name	: SendFwUpdateNotice
**Description	: send a fw update notic to slaver.
**Parameter		: arg - in.
**Return		: none.
***********************************************************************/
void SendFwUpdateNotice(int arg)
{
	EventParams param 			= (*(EventParams*)arg); 
	unsigned int slave_sum 		= 0;
	unsigned int position 		= 0;
	int res 					= 0;
	int fw_count 				= 0;
	int socket_fd				= 0;
	int send_again_counter 		= 0;
	unsigned int counter 		= 0;
	unsigned char data[1 + SLAVE_ADDR_LEN] 				= {0};
	unsigned char notice[9 + SLAVE_ADDR_LEN] 			= {0};
	unsigned char send_again_notice[5 + SLAVE_ADDR_LEN] = {0};
	unsigned char address[SLAVE_ADDR_LEN] 				= {0};
	TIME start;
	TIME start1;
	EventParams next_evt_param;
	
	SetCurSlavePositionOnTab(param.m_Aisle, 0);
	SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);

	//--- get slaver sum and start position on param.m_Aisle ---//	
	if (0x0000ffff != param.m_Body.m_Id)
	{
		if (!GetSlavePositionOnTab(param.m_Body.m_Id, &position, param.m_Aisle))
		{
			slave_sum = position + 1;
		}
		else
		{
			position = slave_sum;
		}
	}
	else
	{
		slave_sum = GetSlaveSumOnAisle(param.m_Aisle);
		position = GetCurSlavePositionOnTab(param.m_Aisle);	
	}
	//--- end of get slaver sum and start position on param.m_Aisle ---// 
	
	//--- fill send_again_notice ---//
	send_again_notice[0] = FW_UPDATE_FLAG0;
	send_again_notice[1] = FW_UPDATE_FLAG1;
	send_again_notice[2] = ACK_FW_DATA_FAILED_TYPE;
	send_again_notice[3 + SLAVE_ADDR_LEN] = FW_UPDATE_FLAG1;
	send_again_notice[4 + SLAVE_ADDR_LEN] = FW_UPDATE_FLAG0;
	//--- end of fille send_again_notice ---//
	
	//--- fill notice content ---//
	notice[0] = FW_UPDATE_FLAG0;
	notice[1] = FW_UPDATE_FLAG1;
	notice[2] = NOTICE_UPDATE_TYPE;
	notice[3 + SLAVE_ADDR_LEN] = g_FWInfo.m_SectionSum;
	notice[4 + SLAVE_ADDR_LEN] = (unsigned char)(g_FWInfo.m_LastSectionSize >> 8);
	notice[5 + SLAVE_ADDR_LEN] = (unsigned char)g_FWInfo.m_LastSectionSize;
	notice[6 + SLAVE_ADDR_LEN] = (unsigned char)g_FWInfo.m_Version;
	notice[7 + SLAVE_ADDR_LEN] = FW_UPDATE_FLAG1;
	notice[8 + SLAVE_ADDR_LEN] = FW_UPDATE_FLAG0;
	
	L_DEBUG("VER = %d\n", notice[6 + SLAVE_ADDR_LEN]);
	//--- end of fill notice content ---//

	socket_fd = ConnectServer(1, g_Param8124);
	
	while(position < slave_sum)	
	{
		SetCurSlavePositionOnTab(param.m_Aisle, position);	//-- set the current slave address table position --//
		
		res = GetSlaveAddrByPos(position, param.m_Aisle);	
		
		address[0] = (unsigned char)(res >> 8);
		address[1] = (unsigned char)res;
		
		memcpy(&notice[3], address, SLAVE_ADDR_LEN);	//-- fill slave address --//
		memcpy(data, address, SLAVE_ADDR_LEN);			
		data[SLAVE_ADDR_LEN] = 1; //-- default update ok! --//
		
		SetAisleFlag(param.m_Aisle, PRO_FW_UPDATE_FLAG);		//-- allow processing update ack data from slave --//

		write(param.m_Aisle, notice, 11);			//-- send notice updated to slave --//
		
		L_DEBUG("send update notice to %.5d by %d aisle (position = %d)\n",res, param.m_Aisle,(position+1));	
		GET_SYS_CURRENT_TIME(start1);
		
		while (IsFWUpdateSuccess(param.m_Aisle))		
		{
			res = IS_TIMEOUT(start1, (10 * 60 * 1000 ));	//-- we will send fw again, if not receive ack in 10 minutes --//		
			if (6 <= send_again_counter || 0 != res) //-- if we send the same section fw 12 times or we can not update ok in 10 minutes, we will give up --//
			{
				send_again_counter = 6;
				printf("%s:%.5d update timeout!\n",__FUNCTION__,((int)(address[0] << 8) | address[1]));
				data[SLAVE_ADDR_LEN] = 0;
				break;
			}
			else if (fw_count != GetCurFwCount(param.m_Aisle))
			{
				send_again_counter = 0;
			}
			
			GET_SYS_CURRENT_TIME(start);
			
			while (!(PRO_DATA_OK_FLAG & GetAisleFlag(param.m_Aisle)))
			{
				Delay_ms(5);
				
				res = IS_TIMEOUT(start, (10 * 1000 + 5));	//-- we will send fw again, if not receive ack in 10s --//
				if (0 != res)
				{	
					send_again_counter++;				
									
					fw_count = GetCurFwCount(param.m_Aisle);
					if (-1 == fw_count)
					{
						if (6 <= send_again_counter)
						{
							break;
						}
						
						printf("%s:rec %.5d update ack timeout, send notice again!\n",__FUNCTION__,	((int)(address[0] << 8) | address[1]));
						
						write(param.m_Aisle, notice, 11);			//-- send notice updated to slave again --//
					}
					else
					{
						memcpy(&send_again_notice[3], address, SLAVE_ADDR_LEN);
						ProcAisleData(param.m_Aisle, send_again_notice, 7); 
						printf("%s:rec %.5d update ack timeout, send fw again!\n",__FUNCTION__, ((int)(address[0] << 8) | address[1]));
					}
					
					GET_SYS_CURRENT_TIME(start);
				} //-- end of if (0 != res) --//
			}
	
			SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
			SetAisleFlag(param.m_Aisle, PRO_FW_UPDATE_FLAG);
			
			Delay_ms(1);			
		}
		
		//--- tell server what we have updated a machine ---//
		res = UploadFeedbacks(5, data, (1 + SLAVE_ADDR_LEN), param.m_Aisle, socket_fd);
		//--- end ---//	
		
		COUNTER(6, send_again_counter);
		send_again_counter = 0;
		ClearFwCount(param.m_Aisle);
		SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
		position++;	
		send_again_counter = 0;			
		sleep(10);
	
	}
	
	SetCurSlavePositionOnTab(param.m_Aisle, 0);	//-- reset --//
	SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
	
	L_DEBUG("===========================================\n");
	L_DEBUG("%d slaves update successful by %d aisle\n", counter, param.m_Aisle);
	L_DEBUG("===========================================\n");
	
	LogoutClient(socket_fd);

	if (counter)
	{
		next_evt_param.m_Aisle = param.m_Aisle;
		next_evt_param.m_Type = RESTART_SLAVE_DATA_TYPE;
		next_evt_param.m_Body.m_Id = param.m_Body.m_Id;
		
		SendDataReq((int)&next_evt_param);		
	}
}













