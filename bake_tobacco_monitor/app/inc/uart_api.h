/* uart_api.h */
#ifndef		UART_API_H
#define		UART_API_H

#define		GNR_COM			0
#define		USB_COM			1
#define 	COM_TYPE		GNR_COM

#define		HOST_COM_PORT0		1
#define     HOST_COM_PORT_SIZE	1

#define		TARGET_COM_PORT0		2
//#define 	TARGET_COM_PORT1		3
#define		TARGET_COM_PORT_SIZE	1

#if (COM_TYPE == GNR_COM)
#define 	MAX_COM_NUM		4
#define		USER_COM_SIZE	TARGET_COM_PORT_SIZE
#else
#define     MAX_COM_NUM     3
#define		USER_COM_SIZE	HOST_COM_PORT_SIZE
#endif

#define 	BUFFER_SIZE		1024*2
#define		TIME_DELAY		180
#define		SEL_FILE_NUM		2
#define		RECV_FILE_NAME		"recv.dat"

extern int g_UartFDS[USER_COM_SIZE];

extern int 	open_port(int com_port);
extern int 	set_com_config(int fd,int baud_rate, int data_bits, char parity, int stop_bits);

#endif /* UART_API_H */
