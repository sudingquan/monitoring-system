#master makefile
CC=gcc
OBJS=common.o master.o
LFLAGS=-lpthread
EXEC=master

all:${OBJS}
	${CC} ${OBJS} -o ${EXEC} ${LFLAGS}

common.o : common.c
master.o : master.c

.PHONY : clean
clean:
	-rm -f ${OBJS}
