#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include "uart_api.h"
#include "AisleManage.h"
#include "MyPublicFunction.h"
#include "MyClientSocket.h"
#include "AsyncEvents.h"
#include "EventActionSet.h"
#include "RemoteCmds.h"
#include "json.h"
#include "xProtocol.h"


//----------------------Define macro for xxx-------------------//

#define IDLE_THRD_NUM 					0		
#define REC_UART0_DATA_NUM				1
//#define REC_UART1_DATA_NUM 				2
#define SEND_UART0_DATA_NUM		 		3
//#define SEND_UART1_DATA_NUM				4
#define UPLOAD_AISLE_DATA_THRD_NUM		5
#define THREAD_SIZE      				4

//---------------------------end-------------------------------//

//------------Declaration static function for xxx--------------//

//--- thread function ---//

static void* 	Thrds(void *pArg);
static void* 	IdleThrd(void *pArg);

//--- general function ---//

static void 	AppInit();
static void 	ReadConfInfo();
static void 	RecUartData(int fd);
static void 	TimerCallback(int SigNum);
static void 	GetRemoteCmd();
static void 	UploadAisleData();

//--------------------Define variable for xxx------------------//

/*
Description			: connect server parameter.
Default value		: /
The scope of value	: /
First used			: AppInit();
*/
CNetParameter g_Param8124 = {0};

/*
Description			: connect server parameter.
Default value		: /
The scope of value	: /
First used			: AppInit();
*/
CNetParameter g_Param8125 = {0};

/*
Description			: middleware machine id
Default value		: 0
The scope of value	: /
First used			: AppInit()
*/
unsigned char g_MyLocalID[11] = {0};

/*
Description			: sqlite db for aisle data.
Default value		: 0
The scope of value	: /
First used			: /
*/
sqlite3 *g_PSqlite3Db;

/*
Description			: partner machine id(last bit)
Default value		: 0
The scope of value	: /
First used			: AppInit()
*/
unsigned char g_PartnerId = 0;

//---------------------------end-------------------------------//


//---------------------------end-------------------------------//

int main()
{	
    AppInit();
    
    return 0;
}

/***********************************************************************
**Function Name	: AppInit
**Description	: initialze some variable and device
**Parameter		: none.
**Return		: none.
***********************************************************************/
static void AppInit()
{
    pthread_t thread[THREAD_SIZE];
#if (COM_TYPE == GNR_COM)
	int target_com[USER_COM_SIZE] = {TARGET_COM_PORT0};
#else
	int target_com[USER_COM_SIZE] = {HOST_COM_PORT0}; 
#endif
	int thrd_num[THREAD_SIZE] = {REC_UART0_DATA_NUM, SEND_UART0_DATA_NUM, UPLOAD_AISLE_DATA_THRD_NUM, IDLE_THRD_NUM};
    int i 			= 0;
    int res 		= 0;
    void *thrd_ret;
    FILE *fp 		= NULL;

    //--- open aisle_data.db ---//
	res = sqlite3_open(AISLE_DATA_DB, &g_PSqlite3Db);
	
	if(res)
    {
    	l_debug(ERR_LOG, "sqlite3 open the %s failed\n", AISLE_DATA_DB);

        sqlite3_close(g_PSqlite3Db);

        exit(1);
    }
    else
    {
        l_debug(NULL, "sqlite3 open the %s successfully\n", AISLE_DATA_DB);
    }
   	
   	//--- open uart ---//
	for (i = 0; i < USER_COM_SIZE; ++i)
	{	
		if((g_UartFDS[i] = open_port(target_com[i])) < 0)
		{
		   g_UartFDS[i] = -1;
		   exit(1);
		}
	

		if( -1 != g_UartFDS[i]) 
		{
		    set_com_config(g_UartFDS[i], 9600, 8, 'N', 1);
		}
	}
	//--- end ---//
	
	AisleManageInit();
	
	if (AsyncEventsInit())
	{
		l_debug(ERR_LOG, "async cmds init failed!\n");
		exit(1);
	}

	if (HttpServerInit(8080))
	{
		l_debug(ERR_LOG, "http server init failed!\n");
		exit(1);		
	}
	
	ReadConfInfo();

	//--- set alarm signal to send local id to server per 1 min ---//
	{
		struct sigaction sa;
		struct sigaction sig;

		sa.sa_handler = SIG_IGN;
		sig.sa_handler = TimerCallback;

		sigemptyset(&sig.sa_mask);
		sig.sa_flags = 0;

		sigaction(SIGALRM, &sa, 0);
		sigaction(SIGALRM, &sig, 0);

		alarm(5);
	}
	//--- end ---//

	for (i = 0; i < (THREAD_SIZE - 1); ++i)
	{
		res = pthread_create(&thread[i], NULL, Thrds, (void*)thrd_num[i]);
		if (0 != res)
		{
			l_debug(ERR_LOG, "%s:create rec data thread failed!\n",__FUNCTION__);
			exit(1);
		}
	}

    res = pthread_create(&thread[i], NULL, IdleThrd, (void*)thrd_num[i]);

    if (0 != res)
    {
		l_debug(ERR_LOG, "%s:create idle thread failed!\n",__FUNCTION__);
        exit(1);
    }

    for (i = 0; i < THREAD_SIZE; i++)
    {
        res = pthread_join(thread[i], &thrd_ret);
    }

}

/***********************************************************************
**Function Name	: ReadConfInfo
**Description	: read conf information.
**Parameter		: none.
**Return		: none.
***********************************************************************/
static void ReadConfInfo()
{
	char conf_info[120] = {0};
	FILE *fp 					= NULL;
	struct json_object *my_conf	= NULL;
	struct json_object *my_obj 	= NULL;
	
	fp = fopen(SER_ADDR,"r");
	
	if (NULL == fp)
	{
		l_debug(ERR_LOG, "%s:open %s failed!\n", __FUNCTION__, SER_ADDR);
		
		exit(1);
	}
	
	fscanf(fp, "%s", conf_info);
	fclose(fp);
	
	my_conf = json_tokener_parse(conf_info);
	my_obj = json_object_object_get(my_conf, "ip");

	memcpy(g_Param8124.m_IPAddr, json_object_get_string(my_obj), strlen(json_object_get_string(my_obj)));
	g_Param8124.m_Port = 8124;
	
	memcpy(g_Param8125.m_IPAddr, g_Param8124.m_IPAddr, strlen(g_Param8124.m_IPAddr));
	g_Param8125.m_Port = 8125;

	json_object_put(my_obj);
	json_object_put(my_conf);
	
	fp = fopen(MID_ID_PATH, "r");
	
	if (NULL == fp)
	{
		l_debug(ERR_LOG, "%s:open %s failed!\n", __FUNCTION__, MID_ID_PATH);
		
		exit(1);
	}
	
	fscanf(fp, "%s", conf_info);
	fclose(fp);	

	my_conf = json_tokener_parse(conf_info);
	
	my_obj = json_object_object_get(my_conf, "id");	//-- my id --//
	memcpy(g_MyLocalID, json_object_get_string(my_obj), strlen(json_object_get_string(my_obj)));
	
	json_object_put(my_obj);
	
	my_obj = json_object_object_get(my_conf, "partnerId");	//-- partner id --//
	memcpy(&g_PartnerId,  json_object_get_string(my_obj), strlen(json_object_get_string(my_obj)));
	
	json_object_put(my_obj);
	json_object_put(my_conf);	
	
	L_DEBUG("server ip %s\nmiddleware id %s\npartner id %c\n", g_Param8124.m_IPAddr,g_MyLocalID,g_PartnerId);
}

/***********************************************************************
**Function Name	: RecUartData
**Description	: received uart data.
**Parameter		: fd - in.
**Return		: none.
***********************************************************************/
static void RecUartData(int fd)
{
    unsigned char buff[BUFFER_SIZE];
    int max_fd = 0;
    int res = 0;
    int real_read = 0;
    int i = 0;
    struct timeval tv;
    fd_set inset;
    fd_set tmp_inset;
    
    FD_ZERO(&inset); 
	FD_SET(fd, &inset);
    
	max_fd = fd;
    
	tv.tv_sec = TIME_DELAY;
	tv.tv_usec = 0;   
	
    while (1)
    {
    	while ((FD_ISSET(fd, &inset)))
    	{   
    		tmp_inset = inset;
    		res = select(max_fd + 1, &tmp_inset, NULL, NULL, NULL);

    		switch(res)
    		{
    			case -1:
    			{
    				perror("select");
    			}
    			break;
    			
    			case 0: 
    			{
    				perror("select time out");
    			}
    			break;
    			
    			default:
    			{
					l_debug(NULL, "----------------------------------%d\n",fd);
					if (FD_ISSET(fd, &tmp_inset))
					{
						memset(buff, 0, BUFFER_SIZE);
						real_read = read(fd, buff, BUFFER_SIZE);
#if 1
						{
						    int j = 0;
						    
						    for (; j < real_read; ++j)
						    {
						        L_DEBUG("0x%.2x,",buff[j]);
						    }
						    L_DEBUG("\n");
						}
#endif
                        if (0 < real_read)
                        {
                            ProcAisleData(fd, buff, real_read);
                        }//-- end of if (0 < real_read) --//
					}
    			} 
    		}
    	} 
    }

}

/***********************************************************************
**Function Name	: Thrds
**Description	: this is thread.
**Parameter		: pArg - unkown.
**Return		: none.
***********************************************************************/
static void* Thrds(void *pArg)
{
   	int thrd_num = (int)pArg;

	L_DEBUG("Thrds %d\n", thrd_num);	
	
	switch (thrd_num)
	{
		case REC_UART0_DATA_NUM:
			RecUartData(g_UartFDS[0]);
			break;
		case UPLOAD_AISLE_DATA_THRD_NUM:
			UploadAisleData();
			break;
		default:
			break;
	}

    pthread_exit(NULL);
}

/***********************************************************************
**Function Name	: IdleThrd
**Description	: this is a idle thread.
**Parameter		: pArg - unkown.
**Return		: none.
***********************************************************************/
static void* IdleThrd(void *pArg)
{
	int thrd_num = (int)pArg;
#if 1	
	unsigned char state_dat[11] = {0,50,0,44,0,25,0,25,1,255,3};
	unsigned char alert_dat[4] = {7,5,0,0};
	char addr_buffer[UPLOAD_SER_SIZE] = {0};
	unsigned int len = 0;
	int i = 0;
#endif
    //--- do here ---//
	L_DEBUG("idle thread %d\n", thrd_num);
	
	while (1)
	{
#if 0	
		L_DEBUG("SEND STATE SIMULATION DATA!\n");

		if(0 == ConnectServer(CONNECT_TIMEOUT, g_Param))
		{		
			sprintf(addr_buffer, "{\"midAddress\":\"0000000000\",\"type\":0,\"address\":\"00401\",\"data\":[0,0,0,0,1,90,1,90]}");	
			SendDataToServer(addr_buffer, strlen(addr_buffer));	
			sleep(1);
	
			sprintf(addr_buffer, "{\"midAddress\":\"0000000000\",\"type\":0,\"address\":\"00402\",\"data\":[0,0,0,0,2,26,2,26]}");	
			SendDataToServer(addr_buffer, strlen(addr_buffer));	
			sleep(1);
			LogoutClient();
		}
		//-----------------------------------------------//
		
		sleep(1*60);

		L_DEBUG("SEND ALERT SIMULATION DATA!\n");
		if(0 == ConnectServer(CONNECT_TIMEOUT, g_Param))
		{
		
			sprintf(addr_buffer, "{\"midAddress\":\"0000000000\",\"type\":2,\"address\":\"00401\",\"isBelow\":0,\"data\":[1,90,1,90,1,90,1,90,0,20,0]}");	
			SendDataToServer(addr_buffer, strlen(addr_buffer));	
			sleep(1);
	
			sprintf(addr_buffer, "{\"midAddress\":\"0000000000\",\"type\":2,\"address\":\"00402\",\"isBelow\":0,\"data\":[2,26,2,26,2,26,2,26,0,20,0]}");	
			SendDataToServer(addr_buffer, strlen(addr_buffer));	
			sleep(1);
				
			LogoutClient();
		}

		sleep(1*60);

		L_DEBUG("SEND CURVE SIMULATION DATA!\n");
		if(0 == ConnectServer(CONNECT_TIMEOUT, g_Param))
		{		
			sprintf(addr_buffer, "{\"midAddress\":\"0000000000\",\"type\":9,\"address\":\"00401\",\"data\":[1,[36,38,40,42,46,48,50,54,60,68],[34,36,37,37.5,38,38,39,40,42],[2,2,15,4,2,4,2,8,4,4,2,4,2,5,5,6,2,8,12],0]}");	
			SendDataToServer(addr_buffer, strlen(addr_buffer));	
			sleep(1);
	
			sprintf(addr_buffer, "{\"midAddress\":\"0000000000\",\"type\":9,\"address\":\"00402\",\"data\":[1,[36,38,40,42,46,48,50,54,60,68],[34,36,37,37.5,38,38,39,40,42],[2,2,15,4,2,4,2,8,4,4,2,4,2,5,5,6,2,8,12],0]}");
			SendDataToServer(addr_buffer, strlen(addr_buffer));	
			sleep(1);
				
			LogoutClient();	
		}	
#endif	
		sleep(2*60);
	}

    pthread_exit(NULL);
}

/***********************************************************************
**Function Name	: TimerCallback
**Description	: alarm() event.
**Parameter		: SigNum - unkown.
**Return		: none.
***********************************************************************/
static void TimerCallback(int SigNum)
{
	static int s_sec 				= 0;
	int async_cmd_start_interval[3] = {80, 240, 480}; //-- alert, status, curves --//
	char evt_types[3] 				= {ALERT_DATA_TYPE, STATUS_DATA_TYPE, CURVE_DATA_TYPE};
	AsyncEvent evt 					= {0};
	int i 							= 0;

	if (SIGALRM == SigNum)
	{	
		GetRemoteCmd();
		//-----------------------------------------------------------------//
	
		if (0 == g_IsFullMode)
		{	
			s_sec++;
			
			for (i = 0; i < 3; ++i)
			{
				if (0 == (s_sec % async_cmd_start_interval[i]))
				{
					evt.m_Action = SendDataReq;
					evt.m_Params.m_Aisle = g_UartFDS[0];
					evt.m_Params.m_Type = evt_types[i];
					evt.m_Params.m_Body.m_Id = 0x0000ffff;
					evt.m_Priority = LEVEL_1;
					
					AddAsyncEvent(evt);
				}
			}
			
			s_sec = s_sec % 700;
		}
				
		//---------------------------------------------------------------//				
	} //--- end of if (SIGALRM == SigNum) ---//
	
	alarm(1);	
}

/***********************************************************************
**Function Name	: GetRemoteCmd
**Description	: get a remote cmd.
**Parameter		: none.
**Return		: none.
***********************************************************************/
static void GetRemoteCmd()
{
	int socket_fd 					= -1;
	static int s_sync_counter		= 0;
	char remote_cmd[256*5]			= {0};
	RemoteCmdInfo cmd;	
	struct json_object *my_json 	= NULL;
	struct json_object *my_array	 = NULL;
	
	my_json = json_object_new_object();
	my_array = json_object_new_array();
	
	json_object_object_add(my_json, "address", json_object_new_string(g_MyLocalID));
	json_object_object_add(my_json, "type", json_object_new_int(3));
	
	json_object_array_add(my_array, json_object_new_int(1));
	json_object_array_add(my_array, json_object_new_int(1));
	json_object_array_add(my_array, json_object_new_int(1));
	json_object_array_add(my_array, json_object_new_int(1));
	json_object_array_add(my_array, json_object_new_int(1));
	json_object_array_add(my_array, json_object_new_int(1));
	json_object_array_add(my_array, json_object_new_int(1));
	json_object_array_add(my_array, json_object_new_int(1));
	json_object_array_add(my_array, json_object_new_int(1));

	if (0 == s_sync_counter)
	{
		json_object_array_add(my_array, json_object_new_int(1));	
	}
	else
	{
		json_object_array_add(my_array, json_object_new_int(0));
	}

	s_sync_counter = (++s_sync_counter) % 3600;
	
	json_object_object_add(my_json, "data", my_array);
	
	socket_fd = ConnectServer(1, g_Param8125);	
	if (0 <= socket_fd)
	{
		SendDataToServer(socket_fd, (unsigned char*)json_object_to_json_string(my_json), strlen(json_object_to_json_string(my_json)));
		
		if (!RecDataFromServer(socket_fd, remote_cmd, (256*5), 3)) //-- the network is 
		{
			L_DEBUG("remote cmd is %s\n", remote_cmd);
			ProRemoteCmd(g_UartFDS[0], remote_cmd);

			sleep(1);
			LogoutClient(socket_fd, g_Param8125);
			
		}
		else
		{
			L_DEBUG("%s:get remote cmd timeout!\n", __FUNCTION__);

			Delay_ms(100);
			LogoutClient(socket_fd, g_Param8125);
		}
	}

	json_object_put(my_json);
	json_object_put(my_array);
}

/***********************************************************************
**Function Name	: UploadAisleData
**Description	: upload aisle data to server.
**Parameter		: none.
**Return		: none.
***********************************************************************/
static void UploadAisleData()
{
	int socket_fd					= 0;
	int upload_sum					= 150;	//-- 1s --//
	int i 							= 0;
	char sql[UPLOAD_SER_SIZE + 40] 	= {0};
	char **p_data 					= NULL;
	int row = 0;
	int col = 0;

	while (1)
	{	
		socket_fd = ConnectServer(1, g_Param8124);
		if (0 <= socket_fd)
		{
			for (i = 0; i < USER_COM_SIZE; ++i)
			{
				upload_sum = 150;

				do
				{
					sprintf(sql, "select * from %s%.2d limit 0,1;", AISLE_DATA_TABLE, i);			

					p_data = NULL;

					if (!sqlite3_get_table(g_PSqlite3Db, sql, &p_data, &row, &col, NULL)) //-- get one data --//
					{
						if (0 != row && 0 != col)
						{						
							L_DEBUG("%s:upload data(%d): %s\n",__FUNCTION__, strlen(p_data[1]), p_data[1]);

							SendDataToServer(socket_fd, (unsigned char*)p_data[1], strlen(p_data[1]));	//-- upload data --//								

							sprintf(sql, "delete from %s%.2d where data='%s';", AISLE_DATA_TABLE, i, p_data[1]);	//-- delete one data --//
							sqlite3_exec(g_PSqlite3Db, sql, NULL, NULL, NULL);

							sqlite3_free_table(p_data);

						}
						else
						{
							//-- no data --//
							L_DEBUG("transfer station: get data end from %s%.2d!\n", AISLE_DATA_TABLE, i);	
							sqlite3_free_table(p_data);
							break;
						}
					}
					else
					{
						//-- get data failed --//
						L_DEBUG("%s:get data failed from transfer station\n", __FUNCTION__);
						break;
					}

				}while(--upload_sum);

			}

			LogoutClient(socket_fd, g_Param8124);			
		}

		sleep(2);
	}
}































