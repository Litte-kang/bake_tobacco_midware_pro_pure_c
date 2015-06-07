#ifndef	_MY_CLIENT_SOCKET_H_
#define _MY_CLIENT_SOCKET_H_

//----------------------Define macro for xxx-------------------//

#define HOSTENT			struct hostent
#define SOCKADDR_IN		struct sockaddr_in
#define IN_ADDR			struct in_addr
#define SOCKADDR		struct sockaddr	

#define CONNECTED_NO		0x00
#define CONNECTED_YES		0x01

#define INVALID_FD	-1

//---------------------------end-------------------------------//


//---------------------Define new type for xxx-----------------//

typedef struct _CNetParameter
{
	unsigned char m_IPAddr[16];
	unsigned short m_Port;
}CNetParameter;

//---------------------------end-------------------------------//


//-----------------Declaration variable for xxx----------------//


//---------------------------end-------------------------------//


//-------------------Declaration funciton for xxx--------------//

extern int 		ConnectServer(unsigned int times, CNetParameter param);
extern int 		RecDataFromServer(int fd, unsigned char *pBuff, unsigned int len, int timeout);
extern int 		SendDataToServer(int fd, unsigned char *pBuff, unsigned int len);
extern void 	LogoutClient(int fd, CNetParameter param);

//---------------------------end-------------------------------//

#endif //-- end of  _MY_CLIENT_SOCKET_H_ --//
