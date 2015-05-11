#ifndef _REMOTE_CMD_PRO_H_
#define _REMOTE_CMD_PRO_H_


//----------------------Define macro for-------------------//

#define REMOTE_CMD_NEW_FW_NOTICE			4
#define REMOTE_CMD_SEARCH_SLAVE_STATUS		8
#define REMOTE_CMD_CONFIG_SLAVE_CURVE		12
#define REMOTE_CMD_CONFIG_SLAVE_TOBA_SIZE	13
#define REMOTE_CMD_CONFIG_SLAVE_TIME		14
#define REMOTE_CMD_SET_SLAVE_ADDR_TAB		15
#define REMOTE_CMD_CONFIG_SLAVE_STAGE		16

//---------------------------end---------------------------//


//---------------------Define new type for-----------------//

typedef struct _RemoteCmdInfo
{
    int		m_Type;
    char	m_Addr[2];
    char	m_Data[100];
    int		m_DataLen;
}RemoteCmdInfo;

//---------------------------end---------------------------//


//-----------------Declaration variable for----------------//


//---------------------------end---------------------------//


//-------------------Declaration funciton for--------------//

extern int		ProRemoteCmd(int fd, char *pCmdInfo);

//---------------------------end---------------------------//

#endif	//--_REMOTE_CMD_PRO_H_--//
