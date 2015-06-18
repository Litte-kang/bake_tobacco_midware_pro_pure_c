SRC = ../app/src/
INC = ../app/inc/
INCS = -I../app/inc
OBJS = main.o uart_api.o AisleManage.o MyPublicFunction.o MyClientSocket.o RemoteCmds.o AsyncEvents.o EventActionSet.o xProtocol.o mongoose.o HttpServer.o

CC = mipsel-openwrt-linux-gcc

APP_NAME = Bake_Tobacco_Monitor

${APP_NAME}:${OBJS}
	${CC} -o ../out/${APP_NAME} $^ -lpthread -ldl -ljson -lsqlite3 -std=c99 -L/usr/local/mips/json-C-0.9/lib -L/usr/local/mips/sqlite-3.7.14/lib

%.o:${SRC}%.c
	${CC} -c ${INCS} $< -o $@ -I/usr/local/mips/json-C-0.9/include/json -I/usr/local/mips/sqlite-3.7.14/include

.PHONY : clean
clean :
	-rm -f ${OBJS} ../out/${APP_NAME}
