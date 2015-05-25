#ifndef _ASYNC_EVENTS_H_
#define _ASYNC_EVENTS_H_

#include "RemoteCmds.h"

//----------------------Define macro for-------------------//

#define MAX_ASYNC_EVT_SUM	5

//--- Priority ---//
#define LEVEL_0		0	//--- high ---//
#define LEVEL_1		1 	//--- general ---//

//--- event flag ---//
#define NULL_ASYNC_EVT_FLAG			0x00
#define PRO_ASYNC_EVT_FLAG			0x01
#define FORCE_END_ASYNC_EVT_FLAG	0x02

//---------------------------end---------------------------//


//---------------------Define new type for-----------------//

typedef void (*EvtAction)(int); 

typedef struct _EventParams
{
	int				m_Aisle;
	int 			m_Type;
	union
	{
		RemoteCmdInfo 	m_RemoteCmd;	//-- config slave --//
		int				m_Id;			//-- get slave data --//
	}m_Body;
}EventParams;

typedef struct _AsyncEvent
{
	EventParams		m_Params;	
	EvtAction		m_Action;
	char			m_Priority;
}AsyncEvent;

typedef struct _AsyncEventQueue
{
	AsyncEvent 		m_AsyncEvts[MAX_ASYNC_EVT_SUM];
	unsigned char	m_ReadPos;
	unsigned char	m_WritePos;
	unsigned char 	m_CurAsyncEvtsSum;
}AsyncEventQueue;

//---------------------------end---------------------------//


//-----------------Declaration variable for----------------//

/*
Description			: force current event.
Default value		: 0.
The scope of value	: /.
First used			: /
*/
extern char g_AsyncEvtFlag;

//---------------------------end---------------------------//


//-------------------Declaration funciton for--------------//

extern int 		AsyncEventsInit(void);
extern int 		AddAsyncEvent(AsyncEvent evt);

//---------------------------end---------------------------//

#endif	//--_ASYNC_EVENTS_H_--//
