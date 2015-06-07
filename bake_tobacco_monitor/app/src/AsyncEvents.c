#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "AsyncEvents.h"
#include "MyPublicFunction.h"

//----------------------Define macro for-------------------//
	
#define READ_ASYNC_CMD_THREAD_START		0xff

//---------------------------end---------------------------//


//--------------------Define variable for------------------//

/*
Description			: general priority async event queue.
Default value		: /.
The scope of value	: /
First used			: AsyncEventsInit
*/
static AsyncEventQueue g_AsyncEvtQueLevel01;

/*
Description			: high priority async event queue.
Default value		: /.
The scope of value	: /
First used			: AsyncEventsInit
*/
static AsyncEventQueue g_AsyncEvtQueLevel00;

/*
Description			: async event queue.
Default value		: IDLE_ASYNC_EVT.
The scope of value	: /
First used			: AsyncEventsInit
*/
static AsyncEvent g_CurAsyncEvent;

/*
Description			: event flags.
Default value		: READ_ASYNC_CMD_THREAD_START.
The scope of value	: /.
First used			: /
*/
char g_AsyncEvtFlag = READ_ASYNC_CMD_THREAD_START;

//---------------------------end--------------------------//


//------------Declaration static function for--------------//

static void* 	ReadAsyncEvtsThrd(void *pArg);
static void		MyDelay_ms(unsigned int xms);

//---------------------------end---------------------------//


/***********************************************************************
**Function Name	: AsyncEventsInit
**Description	: initialize async event queue,create a thread to read a event.
**Parameters	: none.
**Return		: 0 - initialize ok, other value - failed.
***********************************************************************/
int AsyncEventsInit()
{
	int res = 0;
	pthread_t thread;
	pthread_attr_t attr;
	void *thrd_ret = NULL;
	
	g_CurAsyncEvent.m_Action = NULL;
	g_CurAsyncEvent.m_Priority = LEVEL_0;
	
	//--- hight priority queue ---//
	g_AsyncEvtQueLevel00.m_ReadPos = 0;
	g_AsyncEvtQueLevel00.m_WritePos = 0;
	g_AsyncEvtQueLevel00.m_CurAsyncEvtsSum = 0;
	
	//--- general priority queue ---//
	g_AsyncEvtQueLevel01.m_ReadPos = 0;
	g_AsyncEvtQueLevel01.m_WritePos = 0;
	g_AsyncEvtQueLevel01.m_CurAsyncEvtsSum = 0;
	
	res = pthread_attr_init(&attr);
	if (0 != res)
	{
		l_debug(ERR_LOG, "%s:create thread attribute failed!\n",__FUNCTION__);
		return -2;
	}

	res = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	if (0 != res)
	{
		l_debug(ERR_LOG, "%s:bind attribute failed!\n", __FUNCTION__);
		return -2;
	}
	
	res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (0 != res)
	{
		l_debug(ERR_LOG, "%s:setting attribute failed!\n",__FUNCTION__);
		return -3;
	}

	res = pthread_create(&thread, &attr, ReadAsyncEvtsThrd, (void*)0);
	if (0 != res)
	{
		l_debug(ERR_LOG, "%s:create connect \"ReadAsyncCmdsThrd\" failed!\n",__FUNCTION__);
		return -4;
	}
	
	pthread_attr_destroy(&attr);	
	
	while (READ_ASYNC_CMD_THREAD_START == g_AsyncEvtFlag)
	{
		MyDelay_ms(2);
	}
	
	return 0;
}

/***********************************************************************
**Function Name	: MyDelay_ms
**Description	: delay ? ms,but it is not exact.
**Parameters	: xms - in.
**Return		: none.
***********************************************************************/
static void MyDelay_ms(unsigned int xms)
{
	struct timeval delay;
	
	delay.tv_sec = 0;
	delay.tv_usec = xms * 1000;
	
	select(0, NULL, NULL, NULL, &delay);
}

/***********************************************************************
**Function Name	: ReadAsyncEvtsThrd
**Description	: read async event from async event queue.
**Parameters	: pArg - in.
**Return		: none.
***********************************************************************/
static void* ReadAsyncEvtsThrd(void *pArg)
{
	g_AsyncEvtFlag = NULL_ASYNC_EVT_FLAG;
	
	l_debug(NULL, "%s\n", __FUNCTION__);
	
	while (1)
	{
		if (0 < g_AsyncEvtQueLevel00.m_CurAsyncEvtsSum || 0 < g_AsyncEvtQueLevel01.m_CurAsyncEvtsSum)
		{		
			g_AsyncEvtFlag |= PRO_ASYNC_EVT_FLAG;
				
			g_CurAsyncEvent = (0 < g_AsyncEvtQueLevel00.m_CurAsyncEvtsSum \
							? g_AsyncEvtQueLevel00.m_AsyncEvts[g_AsyncEvtQueLevel00.m_ReadPos] \
							: g_AsyncEvtQueLevel01.m_AsyncEvts[g_AsyncEvtQueLevel01.m_ReadPos]);
			
			switch (g_CurAsyncEvent.m_Priority)
			{
				case LEVEL_0:
					g_AsyncEvtQueLevel00.m_ReadPos++;
					g_AsyncEvtQueLevel00.m_CurAsyncEvtsSum--;
					
					if (MAX_ASYNC_EVT_SUM <= g_AsyncEvtQueLevel00.m_ReadPos)
					{
						g_AsyncEvtQueLevel00.m_ReadPos = 0;
					}
					break;
				case LEVEL_1:
					g_AsyncEvtQueLevel01.m_ReadPos++;
					g_AsyncEvtQueLevel01.m_CurAsyncEvtsSum--;
					
					if (MAX_ASYNC_EVT_SUM <= g_AsyncEvtQueLevel01.m_ReadPos)
					{
						g_AsyncEvtQueLevel01.m_ReadPos = 0;
					}					
					break;
				default:
					break;
			}
									
			g_CurAsyncEvent.m_Action((int)&g_CurAsyncEvent.m_Params);
			
			g_AsyncEvtFlag ^= ((FORCE_END_ASYNC_EVT_FLAG & g_AsyncEvtFlag) ? FORCE_END_ASYNC_EVT_FLAG : PRO_ASYNC_EVT_FLAG);
			
			g_CurAsyncEvent.m_Action = NULL;
			g_CurAsyncEvent.m_Priority = LEVEL_0;
	
		}

		MyDelay_ms(50);		
	}
		
	pthread_exit(NULL);
}

/***********************************************************************
**Function Name	: AddAsyncEvent
**Description	: add a async event.
**Parameters	: evt - in.
**Return		: 0 - add ok, -1 - failed.
***********************************************************************/
int AddAsyncEvent(AsyncEvent evt)
{	
	switch (evt.m_Priority)
	{
		case LEVEL_0:
			{
				if (MAX_ASYNC_EVT_SUM > g_AsyncEvtQueLevel00.m_CurAsyncEvtsSum)
				{
					g_AsyncEvtQueLevel00.m_AsyncEvts[g_AsyncEvtQueLevel00.m_WritePos] = evt;
		
					g_AsyncEvtQueLevel00.m_WritePos++;
					g_AsyncEvtQueLevel00.m_CurAsyncEvtsSum++;
					
					if ((PRO_ASYNC_EVT_FLAG & g_AsyncEvtFlag) && LEVEL_1 == g_CurAsyncEvent.m_Priority) //-- we just force end level 1 --//
					{
						g_AsyncEvtFlag |= FORCE_END_ASYNC_EVT_FLAG;
					}
			
					if (MAX_ASYNC_EVT_SUM <= g_AsyncEvtQueLevel00.m_WritePos)
					{
						g_AsyncEvtQueLevel00.m_WritePos = 0;
					}
		
					return 0;
				}				
			}
			break;
		case LEVEL_1:
			{
				if (MAX_ASYNC_EVT_SUM > g_AsyncEvtQueLevel01.m_CurAsyncEvtsSum)
				{
					g_AsyncEvtQueLevel01.m_AsyncEvts[g_AsyncEvtQueLevel01.m_WritePos] = evt;
		
					g_AsyncEvtQueLevel01.m_WritePos++;
					g_AsyncEvtQueLevel01.m_CurAsyncEvtsSum++;
			
					if (MAX_ASYNC_EVT_SUM <= g_AsyncEvtQueLevel01.m_WritePos)
					{
						g_AsyncEvtQueLevel01.m_WritePos = 0;
					}
		
					return 0;
				}				
			}
			break;
		default:
			break;
	}
	
	l_debug(ERR_LOG, "%s: LEVEL_%d async cmd queue fulled!\n", __FUNCTION__,evt.m_Priority);
	
	return -1;
}












