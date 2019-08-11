#master makefile
CC=gcc
OBJS=common.o master.o
LFLAGS=-lpthread
EXEC=pihealth_master_sdq

all:${OBJS}
	${CC} ${OBJS} -o ${EXEC} ${LFLAGS}

common.o : common.c
master.o : master.c

.PHONY : clean
clean:
	-rm -f ${OBJS}

install: pihealth_master_sdq pihealth_master_sdq.conf.sample ./service/pihealth/pihealth_master_sdq.start ./service/pihealth/pihealth_master_sdq.stop ./service/pihealth_master_sdq.service
	@test -f /etc/pihealth_master_sdq.conf.sample || cp pihealth_master_sdq.conf.sample /etc/pihealth_master_sdq.conf
	@cp -f pihealth_master_sdq /usr/bin
	@cp -f ./service/pihealth_master_sdq.service /lib/systemd/system
	@cp -rf ./service/pihealth/ /usr/bin/
	@echo "pihealth_master_sdq installed"
