#master makefile
CC=gcc
OBJS=common.o client.o 
LFLAGS=-lpthread
EXEC=client

all:${OBJS}
	${CC} ${OBJS} -o ${EXEC} ${LFLAGS}

common.o : common.c
client.o : client.c

.PHONY : clean
clean:
	-rm -f ${OBJS}
