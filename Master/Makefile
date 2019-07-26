#master makefile
CC=gcc
OBJS=common.o master.o
LFLAGS=-lpthread
EXEC=master

all:${OBJS}
	${CC} ${OBJS} -o ${EXEC} ${LFLAGS}

common.o:
master.o:

clean:
	rm -rf ${OBJS}
