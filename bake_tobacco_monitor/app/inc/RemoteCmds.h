#ifndef _REMOTE_CMD_PRO_H_
#define _REMOTE_CMD_PRO_H_


//----------------------Define macro for-------------------//

//--- cmd type ---//
#define REMOTE_CMD_NEW_FW_NOTICE			4
#define RMMOTE_CMD_CONFIG_MID_PARAM			8
#define REMOTE_CMD_CONFIG_SLAVE_CURVE		12
#define REMOTE_CMD_CONFIG_SLAVE_TOBA_SIZE	13
#define REMOTE_CMD_CONFIG_MID_TIME			14 //-- sync server time --//
#define REMOTE_CMD_CONFIG_SLAVE_STAGE		16

//--- cmd flag ---//
#define REMOTE_CMD_NULL_FLAG					0x00
#define REMOTE_CMD_START_FW_DOWNLOAD_FLAG		0x01
#define REMOTE_CMD_SYNC_SERVER_TIME_FLAG		0x02

//---------------------------end---------------------------//


//---------------------Define new type for-----------------//

typedef struct _RemoteCmdInfo
{
    int				m_Type;
    unsigned char	m_Addr[2];
    unsigned char	m_Data[100];
    int				m_DataLen;
}RemoteCmdInfo;

//---------------------------end---------------------------//


//-----------------Declaration variable for----------------//

/*
Description			: do somethings.
Default value		: REMOTE_CMD_NULL_FLAG
The scope of value	: /
First used			: /
*/
char g_RemoteCmdFlag;

//---------------------------end---------------------------//


//-------------------Declaration funciton for--------------//

extern int		ProRemoteCmd(int fd, char *pCmdInfo);
extern int	 	WriteRemoteCmdFeedbacksToLocal(int type, unsigned char *pData, unsigned int len, int aisle);

//---------------------------end---------------------------//

#endif	//--_REMOTE_CMD_PRO_H_--//
