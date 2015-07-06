#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "AisleManage.h"
#include "HttpServer.h"
#include "json.h"
#include "MyPublicFunction.h"
#include "mongoose.h"
#include "uart_api.h"
#include "xProtocol.h"
#include "AsyncEvents.h"
#include "EventActionSet.h"
#include "AisleManage.h"

//----------------------Define macro for-------------------//

#define MG_SEND_FILE 	SendWebFile
#define MG_SEND_DATA	SendWebData	

//---------------------------end---------------------------//


//--------------------Define variable for------------------//

/*
Description			: flag http server status.
Default value		: HTTP_SERVER_INIT_STATUS
The scope of value	: /
First used			: /
*/
static char g_HttpServerFlag = HTTP_SERVER_INIT_STATUS;

//---------------------------end--------------------------//


//------------Declaration static function for--------------//

static void*	HttpServerThrd(void *pArg);
static int 		EvtHandler(struct mg_connection *pConn, enum mg_event evt);
static int 		ProPostRequest(struct mg_connection *pConn);
static int 		ProGetRequest(struct mg_connection *pConn);
static int 		ConfigMidIdInfo(char *pConf, int len);
static int 		ConfigSlaveAddrTable(char *pConf, int len);
static int 		GetMidIdInfo(char *pConf);
static int 		GetSlaveAddr(char *pConf);
static int 		GetMidVersion(char *pConf);
static int 		GetSlaveData(char *pConf);
static void 	SendWebFile(struct mg_connection *pConn, int statusCode, const char *pPath);
static void 	SendWebData(struct mg_connection *pConn, int statusCode, const char *pData);

//---------------------------end---------------------------//

/***********************************************************************
**Function Name	: HttpServerInit
**Description	: create a http server.
**Parameters	: port - in.
**Return		: 0 - ok, other value - failed.
***********************************************************************/
int	HttpServerInit(int port)
{
	pthread_t thread;
	pthread_attr_t thread_attr;
	void *thrd_ret 	= NULL;
	int res 		= 0;

	res = pthread_attr_init(&thread_attr);
	if (0 != res)
	{
		printf("%s: thread attr init failed!\n", __FUNCTION__);
		return -1;
	}

	res = pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM);
	if (0 != res)
	{
		printf("%s:set scope failed!\n", __FUNCTION__);
		return -1;		
	}

	res = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	if (0 != res)
	{
		printf("%s:set detach state failed\n", __FUNCTION__);
		return -1;
	}

	res = pthread_create(&thread, &thread_attr, HttpServerThrd, (void*)port);
	if (0 != res)
	{
		printf("%s:create thread task failed!\n", __FUNCTION__);
		return -1;
	}

	pthread_attr_destroy(&thread_attr);

	while (HTTP_SERVER_CREATE_OK != g_HttpServerFlag)
	{
		Delay_ms(5);
	}

	return 0;
}

/***********************************************************************
**Function Name	: SendWebData
**Description	: send some data to web app.
**Parameters	: pConn - in.
				: statusCode - in.
				: pPath - in.
**Return		: none.
***********************************************************************/
static void SendWebFile(struct mg_connection *pConn, int statusCode, const char *pPath)
{
	mg_send_header(pConn, "Content-Type", "text/html");
	mg_send_header(pConn, "Cache-Control", "no-cache");
	mg_send_status(pConn, statusCode);
	mg_send_file(pConn, pPath, NULL);
}

/***********************************************************************
**Function Name	: SendWebData
**Description	: send some data to web app.
**Parameters	: pConn - in.
				: statusCode - in.
				: pData - in.
**Return		: none.
***********************************************************************/
static void SendWebData(struct mg_connection *pConn, int statusCode, const char *pData)
{
	mg_send_header(pConn, "Content-Type", "application/string");
	mg_send_header(pConn, "Cache-Control", "no-cache");
	mg_send_status(pConn, statusCode);
	mg_printf_data(pConn, pData);
}

/***********************************************************************
**Function Name	: HttpServerThrd
**Description	: create thread task to listen http request.
**Parameters	: pArg - in.
**Return		: none.
***********************************************************************/
static void* HttpServerThrd(void *pArg)
{
	struct mg_server *p_server = NULL;
	char port[5] = {0};

	sprintf(port, "%d", (int)pArg);

	//--- Create and configure the server ---//
	p_server = mg_create_server(NULL, EvtHandler);

	if (NULL != p_server)
	{
		g_HttpServerFlag = HTTP_SERVER_CREATE_OK;

		mg_set_option(p_server, "listening_port", port);	
	
		L_DEBUG("Http server port %s\n", mg_get_option(p_server, "listening_port"));

		while (1)
		{
			mg_poll_server(p_server, 1000);
		}

		mg_destroy_server(&p_server);
	}	

	pthread_exit(NULL);
}

/***********************************************************************
**Function Name	: EvtHandler
**Description	: http request event.
**Parameters	: pConn - in.
				: evt - evt type.
**Return		: /
***********************************************************************/
static int EvtHandler(struct mg_connection *pConn, enum mg_event evt)
{
	if (NULL != pConn->request_method)
	{
		L_DEBUG("%s http://%s:%d%s\n",pConn->request_method,pConn->local_ip,pConn->local_port,pConn->uri);
	}
	
	switch (evt)
	{
		case MG_AUTH: 
			return MG_TRUE;
		case MG_REQUEST:
			
			if (!strcmp(pConn->request_method, "GET"))
			{
				return ProGetRequest(pConn);
			}
			else if (!strcmp(pConn->request_method, "POST"))
			{
				return ProPostRequest(pConn);
			}
			
			return MG_TRUE;
		case MG_RECV:
			//printf("%s\n", conn->content);
			return MG_TRUE;
		default: return MG_FALSE;
	}
}

/***********************************************************************
**Function Name	: ProGetRequest
**Description	: process get http request event.
**Parameters	: pConn - in.
**Return		: /
***********************************************************************/
static int ProGetRequest(struct mg_connection *pConn)
{
	char conf_info[MAX_SLAVE_SUM * 5 + 200]	= {0};
	int status_code							= 200;

	if (!strcmp(pConn->uri, "/"))
	{
		MG_SEND_FILE(pConn, status_code, "./web/index.html");		
		return MG_MORE;
	}
	else if (!strcmp(pConn->uri, "/readme.html"))
	{
		MG_SEND_FILE(pConn, status_code, "./web/readme.html");
		return MG_MORE;		
	}
	else if (!strcmp(pConn->uri, "/imgs/readme_1.png"))
	{
		MG_SEND_FILE(pConn, status_code, "./web/public/imgs/readme_1.png");
		return MG_MORE;		
	}
	else if (!strcmp(pConn->uri, "/imgs/readme_2.png"))
	{
		MG_SEND_FILE(pConn, status_code, "./web/public/imgs/readme_2.png");
		return MG_MORE;		
	}
	else if (!strcmp(pConn->uri, "/imgs/readme_3.png"))
	{
		MG_SEND_FILE(pConn, status_code, "./web/public/imgs/readme_3.png");
		return MG_MORE;		
	}
	else if (!strcmp(pConn->uri, "/mid_id"))
	{
		if(0 != GetMidIdInfo(conf_info))
		{
			status_code = 404;
		}

		MG_SEND_DATA(pConn, status_code, conf_info);
	}
	else if (!strcmp(pConn->uri, "/cur_slave_addrs_00"))
	{
		if (0 != GetSlaveAddr(conf_info))
		{
			status_code = 404;
		}

		MG_SEND_DATA(pConn, status_code, conf_info);	
	}
	else if (!strcmp(pConn->uri, "/version"))
	{
		if (0 != GetMidVersion(conf_info))
		{
			status_code = 404;
		}

		MG_SEND_DATA(pConn, status_code, conf_info);
	}
	else if(!strcmp(pConn->uri, "/slave_data"))
	{
		GetSlaveData(conf_info);

		MG_SEND_DATA(pConn, status_code, conf_info);
	}
	else if (!strcmp(pConn->uri, "/tmp_log"))
	{
		MG_SEND_DATA(pConn, status_code, g_AisleLogData.m_Data);
	}
	else if (!strcmp(pConn->uri, "/favicon.ico"))
	{
		mg_send_status(pConn, status_code);
		mg_send_file(pConn, "./web/favicon.ico", NULL);
		
		return MG_MORE;
	}
	
	return MG_TRUE;
}

/***********************************************************************
**Function Name	: ProPostRequest
**Description	: process post http request event.
**Parameters	: pConn - in.
**Return		: /
***********************************************************************/
static int ProPostRequest(struct mg_connection *pConn)
{
	int status_code 		= 200;
	
	if (!strcmp(pConn->uri, "/mid_id"))
	{
		if(ConfigMidIdInfo(pConn->content, (pConn->content_len - 1)))
		{
			status_code = 404;
		}
	}
	else if (!strcmp(pConn->uri, "/slave_addrs"))
	{
		if (ConfigSlaveAddrTable(pConn->content, (pConn->content_len - 1)))
		{
			status_code = 404;
		}
	}
	
	mg_send_header(pConn, "Content-Type", "application/string");
	mg_send_header(pConn, "Cache-Control", "no-cache");	
	mg_send_status(pConn, status_code);
	mg_printf_data(pConn, (status_code == 200 ? "设置成功" : "设置失败"));

	return MG_TRUE;
}

/***********************************************************************
**Function Name	: GetMidIdInfo
**Description	: get middleware id.
**Parameters	: pConf - out.
**Return		: /
***********************************************************************/
static int 	GetMidIdInfo(char *pConf)
{
	FILE *fp 								= NULL;
	struct json_object *my_conf				= NULL;
	struct json_object *my_obj 				= NULL;
	int tmp 								= 0;

	if (NULL == pConf)
	{
		l_debug(ERR_LOG, "%s:param invaild\n", __FUNCTION__);
		return -1;
	}

	fp = fopen(MID_ID_PATH, "r");
	if (NULL == fp)
	{
		l_debug(ERR_LOG, "%s:open %s failed\n", __FUNCTION__, MID_ID_PATH);
		return -1;
	}
	else
	{
		fscanf(fp, "%s", pConf);
		fclose(fp);

		tmp = strlen(pConf);

		my_conf = json_tokener_parse(pConf);
		
		memset(pConf, 0, tmp);

		my_obj = json_object_object_get(my_conf, "id");	//-- my id --//
		tmp = strlen(json_object_get_string(my_obj));
		memcpy(pConf, json_object_get_string(my_obj), tmp);
		
		json_object_put(my_obj);
		
		my_obj = json_object_object_get(my_conf, "name");	//-- name --//
		memcpy(&pConf[tmp],  json_object_get_string(my_obj), strlen(json_object_get_string(my_obj)));
		
		json_object_put(my_obj);
		json_object_put(my_conf);
	}

	return 0;
}

/***********************************************************************
**Function Name	: GetSlaveAddr
**Description	: get slaves id.
**Parameters	: pConf - out.
**Return		: /
***********************************************************************/
static int 	GetSlaveAddr(char *pConf)
{
	FILE *fp 			= NULL;
	int n 				= 0;
	int slave_addr		= 0;

	if (NULL == pConf)
	{
		l_debug(ERR_LOG, "%s:invaild param!\n", __FUNCTION__);
		return -1;
	}

	fp = fopen("./conf/slaves_addr/aisle_00", "r");
	if (NULL == fp)
	{
		l_debug(ERR_LOG, "%s:open ./conf/slaves_addr/aisle_00 failed\n", __FUNCTION__);
		return -1;
	}
	else
	{
		while(!feof(fp))
		{
			if (0 >= fscanf(fp, "%d\n", &slave_addr))
			{
				l_debug(ERR_LOG, "slave address is empty!\n");
				break;
			}
			
			if (INVAILD_SLAVE_ADDR == slave_addr)
			{
				l_debug(ERR_LOG, "a invaild slave addr!\n");
				continue;
			}

			sprintf(&pConf[n], "%.5d ", slave_addr);
			n += 6;
		}

		fclose(fp);
	}

	sprintf(&pConf[n], "</br></br> 当前自控仪数量：%d", (n / 6));

	return 0;
}

/***********************************************************************
**Function Name	: GetMidVersion
**Description	: get middleware version.
**Parameters	: pConf - out.
**Return		: /
***********************************************************************/
static int 	GetMidVersion(char *pConf)
{
	FILE *fp 								= NULL;
	struct json_object *my_conf				= NULL;
	struct json_object *my_obj 				= NULL;
	int tmp 								= 0;

	if (NULL == pConf)
	{
		l_debug(ERR_LOG, "%s:param invaild\n", __FUNCTION__);
		return -1;
	}

	fp = fopen("./conf/version", "r");
	if (NULL == fp)
	{
		l_debug(ERR_LOG, "%s:open ./conf/version failed\n", __FUNCTION__);
		return -1;
	}
	else
	{
		fscanf(fp, "%s", pConf);
		fclose(fp);

		tmp = strlen(pConf);

		my_conf = json_tokener_parse(pConf);

		my_obj = json_object_object_get(my_conf, "version");	//-- my version --//
		tmp = strlen(json_object_get_string(my_obj));
		memcpy(pConf, json_object_get_string(my_obj), tmp);
		
		json_object_put(my_obj);
		json_object_put(my_conf);
	}

	return 0;
}

/***********************************************************************
**Function Name	: GetSlaveData
**Description	: get slaves data(alert, status, curves).
**Parameters	: pConf - out.
**Return		: /
***********************************************************************/
static int 	GetSlaveData(char *pConf)
{
	AsyncEvent evt = {0};

	//--- do here ---//
	evt.m_Action = SendDataReq;
	evt.m_Params.m_Aisle = g_UartFDS[0];
	evt.m_Params.m_Type = ALERT_DATA_TYPE;
	evt.m_Params.m_Body.m_Id = 0x0000ffff;
	evt.m_Priority = LEVEL_1;
	
	AddAsyncEvent(evt);	

	evt.m_Params.m_Type = STATUS_DATA_TYPE;
	AddAsyncEvent(evt);

	evt.m_Params.m_Type = CURVE_DATA_TYPE;
	AddAsyncEvent(evt);	

	return 0;
}

/***********************************************************************
**Function Name	: ConfigMidIdInfo
**Description	: set slaves mid id.
**Parameters	: pConf - in.
				: len - in.
**Return		: /
***********************************************************************/
static int 	ConfigMidIdInfo(char *pConf, int len)
{
	char conf_info[120] 		= {0};
	char mid_id[11]				= {0};
	struct json_object *my_conf	= NULL;
	struct json_object *my_obj 	= NULL;
	FILE *fp 					= NULL;

	if (NULL == pConf)
	{
		l_debug(ERR_LOG, "%s:invaild param!\n", __FUNCTION__);
		return -1;
	}

	memcpy(&conf_info[1], pConf, len);
	conf_info[0] = '{';

	my_conf = json_tokener_parse(conf_info);
	
	memset(conf_info, 0, 100);

	my_obj = json_object_object_get(my_conf, "midId");	//-- my id --//
	memcpy(mid_id, json_object_get_string(my_obj), 10);
	sprintf(conf_info, "{\"id\":\"%s\",\"name\":", mid_id);
	
	json_object_put(my_obj);
	
	my_obj = json_object_object_get(my_conf, "midIdCh");	//-- name --//
	sprintf(&conf_info[strlen(conf_info)],"\"%s\",\"partnerId\":\"0\"}", json_object_get_string(my_obj));

	json_object_put(my_obj);
	json_object_put(my_conf);

	fp = fopen(MID_ID_PATH, "w");
	if (NULL == fp)
	{
		l_debug(ERR_LOG, "%s:open %s failed!\n", __FUNCTION__, MID_ID_PATH);
		return -1;
	}

	fprintf(fp, "%s", conf_info);

	fclose(fp);

	memcpy(g_MyLocalID, mid_id, 10);

	L_DEBUG("new local id %s\n", g_MyLocalID);

	return 0;
}

/***********************************************************************
**Function Name	: ConfigSlaveAddrTable
**Description	: set slaves table.
**Parameters	: pConf - in.
				: len - in.
**Return		: /
***********************************************************************/
static int 	ConfigSlaveAddrTable(char *pConf, int len)
{
	char conf_info[MAX_SLAVE_SUM * 5 + 100]	= {0};
	struct json_object *my_conf				= NULL;
	struct json_object *my_address			= NULL;
	struct json_object *my_obj 				= NULL;
	FILE *fp 								= NULL;
	char is_add_slave						= 0;
	int slave_sum 							= 0;
	int aisle 								= 0;
	int address0 							= 0;
	int address1							= 0;
	int i 									= 0;

	if (NULL == pConf)
	{
		l_debug(ERR_LOG, "%s:invaild param!\n", __FUNCTION__);
		return -1;
	}

	memcpy(&conf_info[1], pConf, len);
	conf_info[0] = '{';

	my_conf = json_tokener_parse(conf_info);
	
	memset(conf_info, 0, (MAX_SLAVE_SUM * 5 + 100));

	my_obj = json_object_object_get(my_conf, "aisle");	//-- aisle --//
	aisle = atoi(json_object_get_string(my_obj));
	sprintf(conf_info, "%s%.2d", SLAVES_ADDR_CONF, aisle);
	json_object_put(my_obj);

	my_obj = json_object_object_get(my_conf, "action");	//-- action --//
	is_add_slave = (char)json_object_get_int(my_obj);
	json_object_put(my_obj);

	my_address = json_object_object_get(my_conf, "addresses");	//-- address --//
	slave_sum = json_object_array_length(my_address);

	if (1 == is_add_slave)
	{
		fp = fopen(conf_info, "a");
		if (NULL == fp)
		{
			l_debug(ERR_LOG, "%s:open %s failed\n", __FUNCTION__, conf_info);

			json_object_put(my_conf);
			json_object_put(my_address);

			return -1;
		}

		for (i = 0; i < slave_sum; ++i)
		{
			my_obj = json_object_array_get_idx(my_address, i);
			fprintf(fp, "%.5d\n",  json_object_get_int(my_obj));
			json_object_put(my_obj);
		}		
	}
	else 
	{
		slave_sum = GetSlaveSumOnAisle(g_UartFDS[aisle]);

		fp = fopen(conf_info, "w");
		if (NULL == fp)
		{
			l_debug(ERR_LOG, "%s:open %s failed\n", __FUNCTION__, conf_info);

			json_object_put(my_conf);
			json_object_put(my_address);

			return -1;
		}
		
		my_obj = json_object_array_get_idx(my_address, 0);
		address0 = json_object_get_int(my_obj);
		
		json_object_put(my_obj);

		for (i = 0; i < slave_sum; ++i)
		{
			address1 = GetSlaveAddrByPos(i, g_UartFDS[aisle]);
			if (address0 != address1)
			{
				fprintf(fp, "%.5d\n",  address1);	
			}			
		}
	}

	fclose(fp);

	json_object_put(my_address);

	json_object_put(my_conf);

	AisleManageInit();

	return 0;	
}































