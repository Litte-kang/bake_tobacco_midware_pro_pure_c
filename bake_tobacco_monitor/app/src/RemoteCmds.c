#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>
#include "RemoteCmds.h"
#include "MyClientSocket.h"
#include "AisleManage.h"
#include "MyPublicFunction.h"
#include "uart_api.h"
#include "AsyncEvents.h"
#include "xProtocol.h"
#include "EventActionSet.h"
#include "json.h"

//----------------------Define macro for-------------------//

#define FW_0_VER			"./fws/fw_0/0.version"
#define FW_1_VER			"./fws/fw_1/1.version"

//---------------------------end---------------------------//


//--------------------Define variable for------------------//

/*
Description			: do somethings.
Default value		: REMOTE_CMD_NULL_FLAG
The scope of value	: /
First used			: /
*/
char g_RemoteCmdFlag = REMOTE_CMD_NULL_FLAG;

//---------------------------end--------------------------//


//------------Declaration static function for--------------//

static void*	DownloadFw(void* pArg);
static int 		CheckFwVersion(RemoteCmdInfo CMDInfo);
static int 		RemoteCmd_NewFwNotice(int fd, RemoteCmdInfo CMDInfo);
static int 		RemoteCmd_ConfigSlave(int fd, RemoteCmdInfo CMDInfo);
static int 		RemoteCmd_ConfigMidParam(int fd, RemoteCmdInfo CMDInfo);
static int 		RemoteCmd_ConfigMidwareTime(int fd, RemoteCmdInfo CMDInfo);
static int 		RemoteCmd_GetSlaveData(int fd, RemoteCmdInfo CMDInfo);

//---------------------------end---------------------------//

/***********************************************************************
**Function Name	: DownloadFw
**Description	: get fw from server.
**Parameter		: pArg - in.
**Return		: none.
***********************************************************************/
static void* DownloadFw(void* pArg)
{
	RemoteCmdInfo param = (*(RemoteCmdInfo*)pArg);
	pid_t pid_id;
	pid_t child_id;
	char args[100] 		= {0};
	char fw_type 		= 0;
	int fw_size 		= 0;
	int fw_ver			= 0;
	int fd				= 0;
	int tmp 			= 0;
	AsyncEvent evt 		= {0};
	FILE *fp			= NULL;
	
	g_RemoteCmdFlag |= REMOTE_CMD_START_FW_DOWNLOAD_FLAG;

	fw_type = param.m_Data[6];
	fw_size = CONV_TO_INT(param.m_Data[0], param.m_Data[1], param.m_Data[2], param.m_Data[3]);
	fw_ver = CONV_TO_INT(0, 0, param.m_Data[4], param.m_Data[5]);
	
	fd = CONV_TO_INT(param.m_Data[7], param.m_Data[8], param.m_Data[9], param.m_Data[10]);

	pid_id = fork();
	
	if (0 > pid_id)
	{
		printf("%s:create child process failed!\n",__FUNCTION__);
	}
	else if (0 == pid_id) //-- download fw process --//
	{		
		sprintf(args, "./fws/fw_%d", fw_type); //-- make a director --//
		mkdir(args);
		
		if (100 == fw_type)
		{
			sprintf(args, "-P ./fws/fw_%d ftp://cs_innotek:cs_innotek@%s/mips/%d/%d.%.3d.fw", fw_type, g_Param8124.m_IPAddr, fw_type, fw_type, fw_ver);
		}
		else
		{
			sprintf(args, "-P ./fws/fw_%d ftp://cs_innotek:cs_innotek@%s/%d/%d.%.3d.fw", fw_type, g_Param8124.m_IPAddr,  fw_type, fw_type, fw_ver);
		}

		L_DEBUG("args is %s\n", args);

		execl("./bin/download_fw", "./bin/download_fw", args, NULL);
	}
	else
	{
		//--- wati fw is downloaded ---//
		do
		{
			child_id = waitpid(pid_id, NULL, WNOHANG);
			
			sleep(1);
		}while(0 == child_id);
		
		if (pid_id == child_id)
		{	
			sprintf(args, "./fws/fw_%d/%d.%.3d.fw", fw_type, fw_type, fw_ver);

			if (!ReadFileInfo(args, &tmp))
			{
				if (tmp == fw_size)
				{
					printf("%s:down fw successful!\n", __FUNCTION__);

					sprintf(args, "{\"version\":\"%.3d\"}", fw_ver);
					
					fp = fopen (FW_0_VER, "w");
					
					fprintf(fp, "%s", args);
					
					fclose(fp);
					
					evt.m_Params.m_Aisle = fd;
					evt.m_Params.m_Type = param.m_Type;
					evt.m_Params.m_Body.m_Id = (int)param.m_Addr[0];
					evt.m_Params.m_Body.m_Id <<= 8;
					evt.m_Params.m_Body.m_Id |= (int)param.m_Addr[1];
					evt.m_Action = SendFwUpdateNotice;
					evt.m_Priority = LEVEL_0;
					
					AddAsyncEvent(evt);

					tmp = 1;
				}
			}
			else
			{
				printf("%s:down fw failed!\n", __FUNCTION__);
				tmp = 0;
			}

			fd = ConnectServer(1, g_Param8124);

			if (0 <= fd)
			{
				sprintf(args, "{\"type\":4,\"address\":\"%s\",\"data\":[%d]}",g_MyLocalID, tmp);
				SendDataToServer(fd, args, strlen(args));
				LogoutClient(fd);
				sleep(1);
			}
		}
		else
		{
			printf("%s:child process failed!\n", __FUNCTION__);
		}
	}
	
	pthread_exit(NULL);
}

/***********************************************************************
**Function Name	: CheckFwVersion
**Description	: check fw version.
**Parameter		: CMDInfo - in.
**Return		: -1 - not exist verison information file, 
				:  0 - new fw version is older, 
				:  1 - new fw version is current version.
				:  2 - new fw version is latest.
***********************************************************************/
static int CheckFwVersion(RemoteCmdInfo CMDInfo)
{
	int new_fw_ver				= 0;
	int cur_fw_ver				= 0;
	FILE *fp 					= NULL;
	struct json_object *my_ver = NULL;
	struct json_object *my_obj 	= NULL;
	char version_infos[100]      = {0}; 
	
	fp = fopen(FW_0_VER, "r");
	
	if (NULL == fp)
	{
		printf("%s:open %s failed\n", __FUNCTION__, FW_0_VER);
		return -1;
	}
	
	fscanf(fp, "%s\n", version_infos);
	
	fclose(fp);
	
	my_ver = json_tokener_parse(version_infos);
	my_obj = json_object_object_get(my_ver, "version");
	cur_fw_ver = json_object_get_int(my_obj);
	
	json_object_put(my_ver);
	json_object_put(my_obj);
	
	new_fw_ver = (int)CMDInfo.m_Data[4];
	new_fw_ver <<= 8;
	new_fw_ver |= (int)CMDInfo.m_Data[5];
	
	if (new_fw_ver < cur_fw_ver)
	{
		L_DEBUG("%s:fw is older version\n",__FUNCTION__);
		return 0;
	}
	else if (new_fw_ver == cur_fw_ver)
	{
		L_DEBUG("%s:fw is current version\n",__FUNCTION__);
		return 1;
	}
	else
	{
		L_DEBUG("%s:fw is new version\n",__FUNCTION__);

		sprintf(version_infos,"./fws/fw_%d/%d.%.3d.fw", CMDInfo.m_Data[6], CMDInfo.m_Data[6], cur_fw_ver);

		L_DEBUG("%s:delete older ver %s\n",__FUNCTION__, version_infos);

		remove(version_infos);

		return 2;
	}
		
	return 0;
}


/***********************************************************************
**Function Name	: RemoteCMD_NewFwNotice
**Description	: notice middleware to update slave.
**Parameters	: fd - in.
				: CMDInfo - in.
**Return		: 0
***********************************************************************/
static int RemoteCMD_NewFwNotice(int fd, RemoteCmdInfo CMDInfo)
{
	int res 				= 0;
	int fw_size 			= 0;
	int sections			= 0;
	int last_section_size 	= 0;
	int socket_fd			= 0;
	pthread_t thread;
	pthread_attr_t attr;
	void *thrd_ret 					= NULL;
	AsyncEvent evt 					= {0};
	unsigned char feedback[50] 		= {0};

	res = CheckFwVersion(CMDInfo);
	
	if (0 == res) //--- older version ---//
	{
		socket_fd = ConnectServer(1, g_Param8124);

		if (0 <= socket_fd)
		{
			sprintf(feedback, "{\"type\":4,\"address\":\"%s\",\"data\":[2]}",g_MyLocalID);
			SendDataToServer(socket_fd, feedback, strlen(feedback));
			LogoutClient(socket_fd);
			sleep(1);
		}

		return 0;
	}
	
	fw_size = CONV_TO_INT(CMDInfo.m_Data[0], CMDInfo.m_Data[1], CMDInfo.m_Data[2], CMDInfo.m_Data[3]);	
	sections = fw_size / (AVG_SECTION_FW_SIZE);
	last_section_size = fw_size % (AVG_SECTION_FW_SIZE);
	
	g_FWInfo.m_Version = (int)CMDInfo.m_Data[4];
	g_FWInfo.m_Version <<=8;
	g_FWInfo.m_Version |= (int)CMDInfo.m_Data[5];
	g_FWInfo.m_SectionSum = (last_section_size) ? ((sections) + 1) : sections;	//-- N + 1 --//
	g_FWInfo.m_LastSectionSize = last_section_size;

	L_DEBUG("fw size = %d, sections = %d\n", fw_size, g_FWInfo.m_SectionSum);
	
	if (1 == res)	//--- the same version ---//
	{
		socket_fd = ConnectServer(1, g_Param8124);

		if (0 <= socket_fd)
		{
			sprintf(feedback, "{\"type\":4,\"address\":\"%s\",\"data\":[1]}",g_MyLocalID);
			SendDataToServer(socket_fd, feedback, strlen(feedback));
			LogoutClient(socket_fd);
			sleep(1);
		}

		evt.m_Params.m_Aisle = fd;
		evt.m_Params.m_Type = CMDInfo.m_Type;
		evt.m_Params.m_Body.m_Id = (int)CMDInfo.m_Addr[0];
		evt.m_Params.m_Body.m_Id <<= 8;
		evt.m_Params.m_Body.m_Id |= (int)CMDInfo.m_Addr[1];
		evt.m_Action = SendFwUpdateNotice;
		evt.m_Priority = LEVEL_0;
		
		AddAsyncEvent(evt);
			
		return 0;
	}

	//--- the latest version ---//	
	res = pthread_attr_init(&attr);
	if (0 != res)
	{
		printf("%s:create thread attribute failed!\n",__FUNCTION__);
		return -2;
	}

	res = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	if (0 != res)
	{
		printf("%s:bind attribute failed!\n", __FUNCTION__);
	}
	
	res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (0 != res)
	{
		printf("%s:setting attribute failed!\n",__FUNCTION__);
		return -3;
	}

	//--- write fd to CMDInfo ---//
	CMDInfo.m_Data[7] = (char)(fd >> 24);
	CMDInfo.m_Data[8] = (char)(fd >> 16);
	CMDInfo.m_Data[9] = (char)(fd >> 8);
	CMDInfo.m_Data[10] = (char)fd;			
	
	res = pthread_create(&thread, &attr, DownloadFw, (void*)&(CMDInfo));
	if (0 != res)
	{
		printf("%s:create connect \"ReadAsyncCmdsThrd\" failed!\n",__FUNCTION__);
		return -4;
	}
	
	pthread_attr_destroy(&attr);

	while (!(REMOTE_CMD_START_FW_DOWNLOAD_FLAG & g_RemoteCmdFlag))
	{
		Delay_ms(5);
	}

	g_RemoteCmdFlag ^= REMOTE_CMD_START_FW_DOWNLOAD_FLAG;

	return 0;
}

/***********************************************************************
**Function Name	: RemoteCmd_ConfigMidParam
**Description	: configure middleware(mid id, slaves address, server ip and so on).
**Parameters	: fd - in.
				: CMDInfo - in.
**Return		: 0
***********************************************************************/
static int RemoteCmd_ConfigMidParam(int fd, RemoteCmdInfo CMDInfo)
{

	return 0;
}

/***********************************************************************
**Function Name	: RemoteCmd_ConfigMidwareTime
**Description	: configure middleware time to sync server time.
**Parameters	: fd - in.
				: CMDInfo - in.
**Return		: 0
***********************************************************************/
static int RemoteCmd_ConfigMidwareTime(int fd, RemoteCmdInfo CMDInfo)
{
	struct tm set_tm;
	struct timeval tv;
	time_t set_time;
	int year = 0;

	year = (int)CMDInfo.m_Data[5];
	year <<= 8;
	year |= (int)CMDInfo.m_Data[6];
	
	set_tm.tm_year = year;
	set_tm.tm_mon = (int)CMDInfo.m_Data[3] - 1;
	set_tm.tm_mday = (int)CMDInfo.m_Data[4];
	set_tm.tm_hour = (int)CMDInfo.m_Data[0];
	set_tm.tm_min = (int)CMDInfo.m_Data[1];
	set_tm.tm_sec = (int)CMDInfo.m_Data[2];
	
	set_time = mktime(&set_tm);
	
	tv.tv_sec = set_time;
	tv.tv_usec = 0;
	
	if (settimeofday(&tv, (struct timezone *)0))
	{
		printf("%s:sync server time failed!\n", __FUNCTION__);
		return 0;
	}

	g_RemoteCmdFlag |= REMOTE_CMD_SYNC_SERVER_TIME_FLAG;

	return 0;
}

/***********************************************************************
**Function Name	: RemoteCMD_ConfigSlave
**Description	: configure slave.
**Parameters	: fd - in.
				: CMDInfo - in.
**Return		: 0
***********************************************************************/
static int RemoteCMD_ConfigSlave(int fd, RemoteCmdInfo CMDInfo)
{
	AsyncEvent evt;
	
	evt.m_Action = SendConfigData;
	evt.m_Params.m_Aisle = fd;
	evt.m_Params.m_Type = CMDInfo.m_Type;
	
	memcpy(&evt.m_Params.m_Body.m_RemoteCmd, &CMDInfo, sizeof(RemoteCmdInfo));
	
	evt.m_Priority = LEVEL_0;
	
	AddAsyncEvent(evt);	
	
	return 0;
}

/***********************************************************************
**Function Name	: RemoteCMD_GetSlaveData
**Description	: get slave data.
**Parameters	: fd - in.
				: CMDInfo - in.
**Return		: 0
***********************************************************************/
static int 	RemoteCMD_GetSlaveData(int fd, RemoteCmdInfo CMDInfo)
{
	AsyncEvent evt = {0};

	//--- do here ---//

	evt.m_Action = SendDataReq;
	evt.m_Params.m_Aisle = fd;
	evt.m_Params.m_Type = CMDInfo.m_Type;

	memcpy(&evt.m_Params.m_Body.m_RemoteCmd, &CMDInfo, sizeof(RemoteCmdInfo));

	evt.m_Priority = LEVEL_0;
	
	AddAsyncEvent(evt);	

	return 0;
}

/***********************************************************************
**Function Name	: ProRemoteCmd
**Description	: process remote cmd.
**Parameters	: fd - in.
				: pCmd - in.
**Return		: 0
***********************************************************************/
int ProRemoteCmd(int fd, char *pCmd)
{
	RemoteCmdInfo cmd_info	 	= {0};
	int slave_id 				= 0;
	int i						= 0;
	int n  						= 0;
	int cmd_sum					= 0;
	struct json_object *my_cmds	= NULL;
	struct json_object *my_cmd 	= NULL;
	struct json_object *my_obj 	= NULL;
	struct json_object *my_data = NULL;

	my_cmds = json_tokener_parse(pCmd);
	cmd_sum = json_object_array_length(my_cmds);

	for (i = 0; i < cmd_sum; ++i)
	{		
		//--- parse cmd data ---//
		my_cmd = json_object_array_get_idx(my_cmds, i);				//-- get a cmd --//

		my_obj = json_object_object_get(my_cmd, "type");
		cmd_info.m_Type = json_object_get_int(my_obj);					//-- data type --//
		json_object_put(my_obj);
		
		my_data = json_object_object_get(my_cmd, "data");	
		cmd_info.m_DataLen = json_object_array_length(my_data) - 2;	//-- data length --//
		my_obj = json_object_array_get_idx(my_data, 0);
		slave_id = json_object_get_int(my_obj);
		cmd_info.m_Addr[0] = (char)slave_id;
		json_object_put(my_obj);
		my_obj = json_object_array_get_idx(my_data, 1);
		slave_id = json_object_get_int(my_obj);		
		cmd_info.m_Addr[1] = (char)slave_id;
		json_object_put(my_obj);
		
		for (n = 0; n < cmd_info.m_DataLen; ++n)
		{
			my_obj = json_object_array_get_idx(my_data, (n + 2));
			cmd_info.m_Data[n] = json_object_get_int(my_obj);
			json_object_put(my_obj);
		}
		
		json_object_put(my_data);
		json_object_put(my_cmd);

		//--- add event ---//
		switch (cmd_info.m_Type)
		{
			case REMOTE_CMD_NEW_FW_NOTICE:
				L_DEBUG("UPDATE CMD\n");
				RemoteCMD_NewFwNotice(fd, cmd_info);
				return 0;
			case RMMOTE_CMD_CONFIG_MID_PARAM:
				L_DEBUG("CONFIG MID PARAM\n");
				RemoteCmd_ConfigMidParam(fd, cmd_info);
				return 0;
			case REMOTE_CMD_CONFIG_MID_TIME:
				L_DEBUG("CONFIG MID TIME\n");
				RemoteCmd_ConfigMidwareTime(fd, cmd_info);
				return 0;
			default:
				break;
		}
		
		if (2 > cmd_info.m_DataLen) //-- request get slave data handle --//
		{	
			L_DEBUG("GET SLAVES DATA(%d) CMD\n", cmd_info.m_Type);
			RemoteCMD_GetSlaveData(fd, cmd_info);
		}
		else if (2 <= cmd_info.m_DataLen)	//-- request modify slave data handle --//
		{
			L_DEBUG("CONFIG SLAVES DATA(%d) CMD\n", cmd_info.m_Type);
			RemoteCMD_ConfigSlave(fd, cmd_info);		
		}
	}

	json_object_put(my_cmds);
		
	return 0;
}





