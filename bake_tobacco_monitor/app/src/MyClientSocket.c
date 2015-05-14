#include "MyClientSocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>

//----------------------Define macro for xxx-------------------//

//---------------------------end-------------------------------//


//--------------------Define variable for xxx------------------//

/*
Description			: 
Default value		:
The scope of value	:
First used			: 
*/


//---------------------------end-------------------------------//


//------------Declaration function for xxx--------------//

static void CatchSig(int SigNum);

//---------------------------end-----------------------//

/***********************************************************************
**Function Name	: CacthSig
**Description	: this is callback,when a specified signal come, call it.
**Parameters	: SigNum - signal type.
**Return		: none.
***********************************************************************/
static void CatchSig(int SigNum)
{
	switch (SigNum)
	{
		case SIGPIPE:
			printf("catch SIGPIPE!\n");
			break;
		default:
			break;
	}
}

/***********************************************************************
**Function Name	: ConnectServer
**Description	: connect server.
**Parameters	: times - in.
				: param - in.
**Return		: -1 - failed, socket - ok.
***********************************************************************/
int ConnectServer(unsigned int times, CNetParameter param)
{	
	SOCKADDR_IN serv_addr = {0};
	int tmp = 0;
	int socket_fd = -1;
	struct sigaction action;
	struct sigaction sa;

	if (0 >= times)
	{		
		printf("%s:param error\n", __FUNCTION__);
		return -1;
	}
		
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (-1 == socket_fd)
	{
		printf("%s:create socket failed!\n",__FUNCTION__);
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(param.m_Port);
	serv_addr.sin_addr.s_addr = inet_addr(param.m_IPAddr);

	bzero(&(serv_addr.sin_zero), 8);
	
	do
	{
		tmp = connect(socket_fd, (SOCKADDR*)&serv_addr, sizeof(SOCKADDR));
		
		if (0 == tmp)
		{			
			printf("connect %s:%d sucessful!\n", param.m_IPAddr, param.m_Port);
			
			sa.sa_handler = SIG_IGN;
			action.sa_handler = CatchSig;

			sigemptyset(&action.sa_mask);

			action.sa_flags = 0;
			sigaction(SIGPIPE, &sa, 0);
			sigaction(SIGPIPE, &action, 0);

			return socket_fd;
		}
		
		sleep(1);
	}while(--times);
	
	printf("connect %s:%d failed!\n", param.m_IPAddr, param.m_Port);
	close(socket_fd);
	
	return -1;
}

/***********************************************************************
**Function Name	: RecDataFromServer
**Description	: recieve data from server.
**Parameters	: fd - in.
				: pBuff - store data recieved.
				: len - expectations.
				: timeout - in(s).
**Return		: none.
***********************************************************************/
int RecDataFromServer(int fd, unsigned char *pBuff, unsigned int len, int timeout)
{
	int rec_len = 0;
	int max_fd = 0;
	fd_set inset;
	struct timeval tv;

	if (NULL == pBuff)
	{
		printf("%s:memory error!\n",__FUNCTION__);
		return -1;
	}

	if (0 > fd)
	{
		printf("%s:socket fd error!\n",__FUNCTION__);
		return -3;
	}
	
	FD_ZERO(&inset);
	FD_SET(fd, &inset);	
	max_fd = fd + 1;
	
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	
	//--- wait data from server ---//
	select(max_fd,  &inset, NULL, NULL, (timeout ? &tv : NULL));	

	if (!FD_ISSET(fd, &inset))
	{
		printf("%s:socket error!\n",__FUNCTION__);
		return -4;
	}

	FD_CLR(fd, &inset);	
	
	rec_len = recv(fd, pBuff, len, 0);	
	if (-1 == rec_len)
	{
		printf("%s:recieve data from server failed!\n",__FUNCTION__);
		return -2;
	}
	else if (0 == rec_len)
	{
		printf("%s:connection break!\n",__FUNCTION__);
		return -5;
	}	

	return 0;
	
}

/***********************************************************************
**Function Name	: SendDataToServer
**Description	: send data to server.
**Parameters	: fd.
				: pBuff - store data recieved.
				: len - expectations.
**Return		: none.
***********************************************************************/
int SendDataToServer(int fd, unsigned char *pBuff, unsigned int len)
{
	int send_len = 0;	

	if (NULL == pBuff)
	{
		printf("%s:memory error!\n",__FUNCTION__);
		return -1;
	}

	if (0 <= fd)
	{
		send_len = send(fd, pBuff, len, 0);
		
		if (-1 == send_len)
		{
			printf("%s:send data to server failed!\n",__FUNCTION__);
			return -2;
		}		
	}
	else
	{
		printf("%s:socket fd error!\n",__FUNCTION__);
		return -3;
	}

	return 0;
}

/***********************************************************************
**Function Name	: LogoutClient
**Description	: logout client free source.
**Parameters	: fd.
**Return		: none.
***********************************************************************/
void LogoutClient(int fd)
{
	if (0 <= fd)
	{
		close(fd);
		printf("logout client(%d) sucessful!\n",fd);		
	}
}









