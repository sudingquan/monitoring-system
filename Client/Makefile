#master makefile
CC=gcc
OBJS=common.o client.o 
LFLAGS=-lpthread
EXEC=client

all:${OBJS}
	${CC} ${OBJS} -o ${EXEC} ${LFLAGS}

common.o:
client.o:

clean:
	rm -rf ${OBJS}
