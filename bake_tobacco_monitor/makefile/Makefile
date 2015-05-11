SRC = ../app/src/
INC = ../app/inc/
INCS = -I../app/inc
OBJS = 	main.o \
		uart_api.o \
		AisleManage.o \
		MyPublicFunction.o \
		MyClientSocket.o \
		RemoteCmds.o \
		AsyncEvents.o \
		EventActionSet.o \
		xProtocol.o

HOST_CC = gcc
ARM_CC = arm-linux-gcc
MIPS_CC = mipsel-openwrt-linux-gcc
CC = ${MIPS_CC}

APP_NAME = Bake_Tobacco_Monitor

${APP_NAME}:${OBJS}
	${CC} -o ../out/${APP_NAME} $^ -lpthread -ldl -ljson -std=c99 -L/usr/local/mips/json-C-0.9/lib
		
%.o:${SRC}%.c
	${CC} -c ${INCS} $< -o $@ -I/usr/local/mips/json-C-0.9/include/json

.PHONY : clean
clean :
	-rm -f ${OBJS} ../out/${APP_NAME}
