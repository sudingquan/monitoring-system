#master makefile
CC=gcc
OBJS=common.o client.o 
LFLAGS=-lpthread
EXEC=pihealth_client_sdq

all:${OBJS}
	${CC} ${OBJS} -o ${EXEC} ${LFLAGS}

common.o : common.c
client.o : client.c

.PHONY : clean
clean:
	-rm -f ${OBJS}

install: pihealth_client_sdq ../script/ pihealth_client_sdq.conf.sample
		@test -f /tmp/pihealth_client_sdq.conf || cp pihealth_client_sdq.conf.sample /tmp/pihealth_client_sdq.conf
		#@cp -f pihealth_client_sdq /usr/bin
		@cp -r ../script/ /tmp/sdq_script/
		@echo "pihealth_client_sdq installed"

