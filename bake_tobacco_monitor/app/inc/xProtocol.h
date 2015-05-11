#ifndef _X_PROTOCOL_H_
#define _X_PROTOCOL_H_

#include "AisleManage.h"

//----------------------Define macro for-------------------//

/****************************************************************
*					update								        *
*****************************************************************/
#define	FW_UPDATE_FLAG0 'R'
#define FW_UPDATE_FLAG1 'E'

//--- data type ---//
#define NOTICE_UPDATE_TYPE			0x01
#define ACK_NOTICE_UPDATE_YES_TYPE	0x02
#define	FW_DATA_TYPE				0x03
#define	ACK_FW_DATA_SUC_TYPE		0x04
#define ACK_FW_DATA_FAILED_TYPE		0x05
#define UPDATE_SUC_TYPE				0x06
#define ACK_NOTICE_UPDATE_NO_TYPE	0x07

#define AVG_SECTION_FW_SIZE		1024	//-- 1K per --//

#define FW_0_PATH	"./fws/fw_0/"

/****************************************************************
*				end update								        *
*****************************************************************/

/****************************************************************
*					communication protocol(slave)		        *
*****************************************************************/
#define REQ_MSG_FLAG0	'F'	//-- F --//
#define REQ_MSG_FLAG1	'D'	//-- D --//

#define ACK_MSG_FLAG0	'F'	//-- F --//
#define ACK_MSG_FLAG1	'E'	//-- E --//

#define ALERT_DATA_TYPE     		0
#define ACK_DATA_TYPE       		1
#define STATUS_DATA_TYPE    		2
#define CURVE_DATA_TYPE				3
#define REG_REQUEST_TYPE			6
#define RESTART_SLAVE_DATA_TYPE		10
#define CONF_CURVE_DATA_TYPE		12
#define CONF_TOBA_SIZE_DATA_TYPE	13
#define CONF_TIME_DATA_TYPE			14


#define REQ_MSG_SIZE	7
/****************************************************************
*				end communication protocol(slaver)		        *
*****************************************************************/

//---------------------------end---------------------------//


//---------------------Define new type for-----------------//
//---------------------------end---------------------------//


//-----------------Declaration variable for----------------//

//---------------------------end---------------------------//


//-------------------Declaration funciton for--------------//

extern int 				SendConfigration(int fd, unsigned char *pSlaverAddr, unsigned char type, unsigned char *pData, unsigned int len);
extern int 				SendCommunicationRequest(int fd, unsigned char *pSlaverAddr, unsigned char type);
extern int				SendFwToSlave(AisleInfo *pInfo);
extern int 				CheckCommuData(unsigned char *pData, unsigned char *pType);

//---------------------------end---------------------------//

#endif	//--_X_PROTOCOL_H_--//
