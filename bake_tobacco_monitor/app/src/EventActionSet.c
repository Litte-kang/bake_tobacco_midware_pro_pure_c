#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
Description			: event action flags.
Default value		: EVT_ACTION_NULL_FLAG.
The scope of value	: /.
First used			: /
*/
char g_EvtActionFlag = EVT_ACTION_NULL_FLAG;

//---------------------------end--------------------------//


//------------Declaration static function for--------------//

//---------------------------end---------------------------//


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
	unsigned char logs[50]					= {0};
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

				res = IS_TIMEOUT(start, (2000));	//-- we will send notice again slave, if not receive ack in (2)s --//
				if (0 != res)
				{
					sprintf(logs, "receive %.5d %d type ack timeout!</br>\n", ((int)(address[0] << 8) | address[1]), param.m_Type);
					L_DEBUG("%s:%s", __FUNCTION__, logs);
					
					SaveAisleLog(logs);
					
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
							
		position++;
		COUNTER(3, send_again_counter);
		send_again_counter = 0;	
		SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);		

		if ((FORCE_END_ASYNC_EVT_FLAG & g_AsyncEvtFlag))
		{
			break;
		}
	}

	SetCurSlavePositionOnTab(param.m_Aisle, 0);
	SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
	
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
	unsigned char logs[50]		= {0};
	unsigned int counter 		= 0;
	unsigned char data_status 	= 0;
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
				res = IS_TIMEOUT(start, (3000));	//-- we will send request again slave, if not receive ack in 5s --//
				if (0 != res)
				{					
					sprintf(logs, "receive %.5d %d type ack timeout!</br>\n", ((int)(address[0] << 8) | address[1]), param.m_Type);
					L_DEBUG("%s:%s", __FUNCTION__, logs);
					
					SaveAisleLog(logs);

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
					
		//--- tell server what we modify ok ---//
		res = WriteRemoteCmdFeedbacksToLocal(param.m_Body.m_RemoteCmd.m_Type, data, (1 + SLAVE_ADDR_LEN), param.m_Aisle);
		//--- end ---//	
		
		SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
		position++;
		COUNTER(3, send_again_counter);
		send_again_counter = 0;

		if ((FORCE_END_ASYNC_EVT_FLAG & g_AsyncEvtFlag))
		{
			break;
		}
	}

	SetCurSlavePositionOnTab(param.m_Aisle, 0);
	SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
	
	L_DEBUG("===========================================\n");
	L_DEBUG("%d slaves configure successful by %d aisle!\n", counter, param.m_Aisle);
	L_DEBUG("===========================================\n");

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
	int n 						= 0;
	int fw_count 				= 0;
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
	slave_sum = (unsigned int)param.m_Body.m_RemoteCmd.m_Data[0];
	slave_sum <<= 8;
	slave_sum |= (unsigned int)param.m_Body.m_RemoteCmd.m_Data[1];
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
	
	L_DEBUG("slave version = %.3d\n", notice[6 + SLAVE_ADDR_LEN]);
	//--- end of fill notice content ---//
	n = 9;
	while(slave_sum--)	
	{
		res = param.m_Body.m_RemoteCmd.m_Data[n++];
		res <<= 8;
		res |= param.m_Body.m_RemoteCmd.m_Data[n++];

		if (GetSlavePositionOnTab(res, &position, param.m_Aisle))
		{
			continue;
		}

		SetCurSlavePositionOnTab(param.m_Aisle, position);	//-- set the current slave address table position --//
		
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
				L_DEBUG("%s:%.5d update timeout!\n",__FUNCTION__,((int)(address[0] << 8) | address[1]));
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
						
						L_DEBUG("%s:rec %.5d update ack timeout, send notice again!\n",__FUNCTION__,	((int)(address[0] << 8) | address[1]));
						
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
		res = WriteRemoteCmdFeedbacksToLocal(5, data, (1 + SLAVE_ADDR_LEN), param.m_Aisle);
		//--- end ---//	
		
		COUNTER(6, send_again_counter);
		send_again_counter = 0;
		ClearFwCount(param.m_Aisle);
		SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
		send_again_counter = 0;			
		sleep(10);
	
	}
	
	SetCurSlavePositionOnTab(param.m_Aisle, 0);	//-- reset --//
	SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
	
	L_DEBUG("===========================================\n");
	L_DEBUG("%d slaves update successful by %d aisle\n", counter, param.m_Aisle);
	L_DEBUG("===========================================\n");
	
	if (counter)
	{
		next_evt_param.m_Aisle = param.m_Aisle;
		next_evt_param.m_Type = RESTART_SLAVE_DATA_TYPE;
		next_evt_param.m_Body.m_Id = param.m_Body.m_Id;
		
		SendDataReq((int)&next_evt_param);		
	}
}

/***********************************************************************
**Function Name	: SendTimeData
**Description	: sync slave time.
**Parameter		: arg - in.
**Return		: none.
***********************************************************************/
void SendTimeData(int arg)
{
	EventParams param 		= (*(EventParams*)arg); 
	unsigned int res 		= 0;
	unsigned int slave_sum 	= 0;
	unsigned int position 	= 0;
	unsigned int send_again_counter 		= 0;
	unsigned char address[SLAVE_ADDR_LEN] 	= {0};
	unsigned char time_value[6]				= {0};
	unsigned char logs[50] 					= {0};
	char data_status						= 0;
	unsigned int counter 					= 0;
	time_t now_time 						= {0};
	struct tm *p_now_time 					= NULL;
	TIME start;
	
	SetCurSlavePositionOnTab(param.m_Aisle, 0);
	SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);

	//--- get slave sum and start position on param.m_Aisle ---//	
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
		SetCurSlavePositionOnTab(param.m_Aisle, position);	//-- set the current slave address table position --//	
		
		res = GetSlaveAddrByPos(position, param.m_Aisle);
		
		address[0] = (unsigned char)(res >> 8);
		address[1] = (unsigned char)res;

		time(&now_time);
		p_now_time = localtime(&now_time);

		time_value[0] = (unsigned char)(p_now_time->tm_year % 100);
		time_value[1] = (unsigned char)(p_now_time->tm_mon + 1);
		time_value[2] = (unsigned char)p_now_time->tm_mday;
		time_value[3] = (unsigned char)p_now_time->tm_hour;
		time_value[4] = (unsigned char)p_now_time->tm_min;	

		L_DEBUG("send time data(20%d-%.2d-%.2d %.2d:%.2d) to %.5d by %d aisle\n", time_value[0], time_value[1], time_value[2], time_value[3], time_value[4], res, param.m_Aisle);
		
		SendConfigration(param.m_Aisle, address, param.m_Type, time_value, 5);
					
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
				res = IS_TIMEOUT(start, (3000));	//-- we will send request again slave, if not receive ack in 5s --//
				if (0 != res)
				{					
					sprintf(logs, "receive %.5d %d type ack timeout!</br>\n", ((int)(address[0] << 8) | address[1]), param.m_Type);
					L_DEBUG("%s:%s", __FUNCTION__, logs);
					
					SaveAisleLog(logs);

					SendConfigration(param.m_Aisle, address, param.m_Type, time_value, 5);
					
					GET_SYS_CURRENT_TIME(start);
					send_again_counter++;
				}
			}
			else if (PRO_DATA_FAILED_FLAG == data_status)
			{
				SendConfigration(param.m_Aisle, address, param.m_Type, time_value, 5);
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
				
		}while(3 > send_again_counter);  //-- we will notice next slave, if we have sent 3 times --//
					
		SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);
		position++;
		COUNTER(3, send_again_counter);
		send_again_counter = 0;

		if ((FORCE_END_ASYNC_EVT_FLAG & g_AsyncEvtFlag))
		{
			break;
		}
	}

	SetCurSlavePositionOnTab(param.m_Aisle, 0);
	SetAisleFlag(param.m_Aisle, NULL_DATA_FLAG);

	g_EvtActionFlag ^= EVT_ACTION_SYNC_TIME_FLAG; 
	
	L_DEBUG("===========================================\n");
	L_DEBUG("%d slaves sync time successful by %d aisle!\n", counter, param.m_Aisle);
	L_DEBUG("===========================================\n");

}











