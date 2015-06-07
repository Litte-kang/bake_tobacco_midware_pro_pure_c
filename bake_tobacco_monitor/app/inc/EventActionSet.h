#ifndef _EVENT_ACTION_SET_H_
#define _EVENT_ACTION_SET_H_


//----------------------Define macro for-------------------//

#define EVT_ACTION_NULL_FLAG		0x00
#define EVT_ACTION_SYNC_TIME_FLAG	0x01

//---------------------------end---------------------------//


//---------------------Define new type for-----------------//

//---------------------------end---------------------------//


//-----------------Declaration variable for----------------//

/*
Description			: event action flags.
Default value		: EVT_ACTION_NULL_FLAG.
The scope of value	: /.
First used			: /
*/
extern char g_EvtActionFlag;

//---------------------------end---------------------------//


//-------------------Declaration funciton for--------------//

extern void		SendDataReq(int arg);
extern void 	SendConfigData(int arg);
extern void 	SendTimeData(int arg);
extern void		SendFwUpdateNotice(int arg);

//---------------------------end---------------------------//

#endif	//--_EVENT_ACTION_SET_H_--//
