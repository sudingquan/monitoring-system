/*************************************************************************
	> File Name: client.c
	> Author: sudingquan
	> Mail: 1151015256@qq.com
	> Created Time: 六  7/20 14:31:28 2019
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include "common.h"
#define CONF "/tmp/pihealth_client_sdq.conf"
#define LOG_BUFF 1024
#define MAX_SIZE 1024

char heartbeat_client_port[10];
char ctl_client_port[10];
char master_port[10];
char master[20];
char dyaver[10] = {0};
char datadir[100];
char scriptdir[100];
char PiHealthLog[100];

struct sm_msg{
    int flag;
    pthread_mutex_t sm_mutex;
    pthread_cond_t sm_ready;
};

typedef struct Log {
    char data[MAX_SIZE + 5];
    int flag;
} Log;

void handler(int sig) {	
	while (waitpid(-1, NULL, WNOHANG) > 0) {
		//printf("成功处理一个子进程的退出\n");
	}
    return ;
}

void heartbeating() {
    //printf("子进程接收到信号，开始心跳\n");
    my_log(PiHealthLog, "[Note] [heartbeating] : heartbeat start\n");
    int sleep_time = 1;
    for (int i = 1; ; i++) {
        //printf("第 %d 次 : \n", i);
        int j = i;
        while (j--) {
            if (heartbeat(atoi(master_port), master) < 0) {
                fflush(stdout);
                //printf("❤️  ");
                sleep(1);
            } else {
                //printf("心跳成功\n");
                my_log(PiHealthLog, "[Note] [heartbeating] : heartbeat success\n");
                return ;
            }
        }
        //printf("\n");
        sleep(sleep_time);
        sleep_time++;
    }
}

void *listen_heartbeat(void *a) {
    //printf("listen heartbeat pthread start\n");
    my_log(PiHealthLog, "[Note] [listen_heartbeat] : listen heartbeat pthread start\n");
    struct epoll_event ev, events[10];
    int heartbeat_listen_socket, conn_sock, nfds, epollfd;
    struct sockaddr_in client;
    unsigned int addrlen = sizeof(client);

    heartbeat_listen_socket = create_listen_socket(atoi(heartbeat_client_port));
    if (heartbeat_listen_socket < 0) {
        //printf("create heartbeat listen socket failed\n");
        my_log(PiHealthLog, "[Error] [create_listen_socket] : %s\n", strerror(errno));
        my_log(PiHealthLog, "exit\n");

        exit(EXIT_FAILURE);
    }

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        //perror("epoll_create1");
        my_log(PiHealthLog, "[Error] [epoll_create1] : %s\n", strerror(errno));
        my_log(PiHealthLog, "exit\n");
        close(heartbeat_listen_socket);
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = heartbeat_listen_socket;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, heartbeat_listen_socket, &ev) == -1) {
        //perror("epoll_ctl: heartbeat_listen_socket");
        my_log(PiHealthLog, "[Error] [epoll_ctl] : %s\n", strerror(errno));
        my_log(PiHealthLog, "exit\n");
        close(heartbeat_listen_socket);
        close(epollfd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        //printf("wait for heartbeat...\n");
       	nfds = epoll_wait(epollfd, events, 1, -1);
        if (nfds == -1) {
            //perror("epoll_wait");
            my_log(PiHealthLog, "[Error] [epoll_wait] : %s\n", strerror(errno));
            my_log(PiHealthLog, "exit\n");
            close(heartbeat_listen_socket);
            close(epollfd);
            exit(EXIT_FAILURE);
        } 
        for (int n = 0; n < nfds; n++) {
            if (events[n].data.fd == heartbeat_listen_socket) {
                conn_sock = accept(heartbeat_listen_socket, (struct sockaddr *) &client, &addrlen);
                if (conn_sock == -1) {
                    //perror("accept");
                    my_log(PiHealthLog, "[Error] [accept] : %s\n", strerror(errno));
                    my_log(PiHealthLog, "exit\n");
                    close(conn_sock);
                    close(heartbeat_listen_socket);
                    close(epollfd);
                    exit(EXIT_FAILURE);
                }
                getpeername(conn_sock, (struct sockaddr *)&client, &addrlen);
                //printf("recv master %s : %d  ❤️   success\n", inet_ntoa((client.sin_addr)), ntohs(client.sin_port));
                close(conn_sock);
            }
        }
    }
}

void *generate_log(void *a) {
    //printf("generate log child pthread start !\n");
    my_log(PiHealthLog, "[Note] [generate_log] : generate log child pthread start\n");
    while (1) {
        char cpu_buff[5][LOG_BUFF];
        char mem_buff[5][LOG_BUFF];
        char disk_buff[5][LOG_BUFF];
        char pro_buff[5][LOG_BUFF];
        char user_buff[5][LOG_BUFF];
        char sys_buff[5][LOG_BUFF];
        for (int i = 0; i < 5; i++) {
            FILE *cpu_pp = NULL;
            FILE *mem_pp = NULL;
            FILE *disk_pp = NULL;
            FILE *pro_pp = NULL;
            FILE *user_pp = NULL;
            FILE *sys_pp = NULL;
            memset(cpu_buff[i], 0, sizeof(cpu_buff[i]));
            memset(mem_buff[i], 0, sizeof(mem_buff[i]));
            memset(disk_buff[i], 0, sizeof(disk_buff[i]));
            memset(pro_buff[i], 0, sizeof(pro_buff[i]));
            memset(user_buff[i], 0 ,sizeof(user_buff[i]));
            memset(sys_buff[i], 0, sizeof(sys_buff[i]));

            char cpulog[100] = {0};
            char memlog[100] = {0};
            char disklog[100] = {0};
            char detectlog[100] = {0};
            char userlog[100] = {0};
            char syslog[100] = {0};
            sprintf(cpulog, "bash %s/CpuLog.sh", scriptdir);
            sprintf(memlog, "bash %s/MemLog.sh", scriptdir);
            sprintf(disklog, "bash %s/Disk.sh", scriptdir);
            sprintf(detectlog, "bash %s/Detect.sh", scriptdir);
            sprintf(userlog, "bash %s/Users.sh", scriptdir);
            sprintf(syslog, "bash %s/SysInfo.sh", scriptdir);

            cpu_pp = popen(cpulog,"r");
            char mem_cmd[100];
            sprintf(mem_cmd, "%s %s", memlog, dyaver);
            mem_pp = popen(mem_cmd,"r");
            disk_pp = popen(disklog,"r");
            pro_pp = popen(detectlog,"r");
            user_pp = popen(userlog,"r");
            sys_pp = popen(syslog,"r");
            if (cpu_pp == NULL) {
                //perror("popen");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (mem_pp == NULL) {
                //perror("popen");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (disk_pp == NULL) {
                //perror("popen");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (pro_pp == NULL) {
                //perror("popen");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (user_pp == NULL) {
                //perror("popen");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (sys_pp == NULL) {
                //perror("popen");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }

            if (fread(cpu_buff[i], 1, LOG_BUFF, cpu_pp) < 0) {
                //perror("fread:cpu_buff");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }

            if (fgets(mem_buff[i], LOG_BUFF, mem_pp) < 0) {
                //perror("fgets:mem_buff");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (fgets(dyaver, LOG_BUFF, mem_pp) < 0) {
                //perror("fgets:dyaver");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }

            if (fread(disk_buff[i], 1, LOG_BUFF, disk_pp) < 0) {
                //perror("fread:disk_buff");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (fread(pro_buff[i], 1, LOG_BUFF, pro_pp) < 0) {
                //perror("fread:pro_buff");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (fread(user_buff[i], 1, LOG_BUFF, user_pp) < 0) {
                //perror("fread:user_buff");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (fread(sys_buff[i], 1, LOG_BUFF, sys_pp) < 0) {
                //perror("fread:sys_buff");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            pclose(cpu_pp);
            pclose(mem_pp);
            pclose(disk_pp);
            pclose(pro_pp);
            pclose(user_pp);
            pclose(sys_pp);
            sleep(5);
        }
        for (int i = 0; i < 5; i++) {
            FILE *cpu_fp=NULL;
            FILE *mem_fp=NULL;
            FILE *disk_fp=NULL;
            FILE *pro_fp=NULL;
            FILE *user_fp=NULL;
            FILE *sys_fp=NULL;
            char cpudata[100] = {0};
            sprintf(cpudata, "%s/Cpu.log", datadir);
            cpu_fp = fopen(cpudata, "a");
            if (cpu_fp == NULL) {
                //perror("fopen:cpu_fp");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            char memdata[100] = {0};
            sprintf(memdata, "%s/Mem.log", datadir);
            mem_fp = fopen(memdata, "a");
            if (mem_fp == NULL) {
                //perror("fopen:mem_fp");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            char diskdata[100] = {0};
            sprintf(diskdata, "%s/Disk.log", datadir);
            disk_fp = fopen(diskdata, "a");
            if (disk_fp == NULL) {
                //perror("fopen:disk_fp");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            char prodata[100] = {0};
            sprintf(prodata, "%s/Process.log", datadir);
            pro_fp = fopen(prodata, "a");
            if (pro_fp == NULL) {
                //perror("fopen:pro_fp");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            char userdata[100] = {0};
            sprintf(userdata, "%s/User.log", datadir);
            user_fp = fopen(userdata, "a");
            if (user_fp == NULL) {
                //perror("fopen:user_fp");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            char sysdata[100] = {0};
            sprintf(sysdata, "%s/Sys.log", datadir);
            sys_fp = fopen(sysdata, "a");
            if (sys_fp == NULL) {
                //perror("fopen:sys_fp");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }

            if (flock(fileno(cpu_fp), LOCK_EX) < 0) {
                //perror("flock:cpu_fp");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (flock(fileno(mem_fp), LOCK_EX) < 0) {
                //perror("flock:mem_fp");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (flock(fileno(disk_fp), LOCK_EX) < 0) {
                //perror("flock:disk_fp");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (flock(fileno(pro_fp), LOCK_EX) < 0) {
                //perror("flock:pro_fp");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (flock(fileno(user_fp), LOCK_EX) < 0) {
                //perror("flock:user_fp");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (flock(fileno(sys_fp), LOCK_EX) < 0) {
                //perror("flock:sys_fp");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }

            if (fwrite(cpu_buff[i],1, strlen(cpu_buff[i]), cpu_fp) < 0) {
                //perror("fwrite:cpu_buff");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (fwrite(mem_buff[i],1, strlen(mem_buff[i]), mem_fp) < 0) {
                //perror("fwrite:mem_buff");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (fwrite(disk_buff[i],1, strlen(disk_buff[i]), disk_fp) < 0) {
                //perror("fwrite:disk_buff");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (fwrite(pro_buff[i],1, strlen(pro_buff[i]), pro_fp) < 0) {
                //perror("fwrite:pro_buff");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (fwrite(user_buff[i],1, strlen(user_buff[i]), user_fp) < 0) {
                //perror("fwrite:user_buff");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            if (fwrite(sys_buff[i],1, strlen(sys_buff[i]), sys_fp) < 0) {
                //perror("fwrite:sys_buff");
                my_log(PiHealthLog, "[Error] [generate_log] : %s\n", strerror(errno));
                continue;
            }
            fclose(cpu_fp);
            fclose(mem_fp);
            fclose(disk_fp);
            fclose(pro_fp);
            fclose(user_fp);
            fclose(sys_fp);
        }
        //printf("完成系统检测，睡眠5s\n");
        my_log(PiHealthLog, "[Note] [generate_log] : complete system detection\n");
        sleep(5);
    }
}

int main() {
    if (get_conf(CONF, "PiHealthLog", PiHealthLog) < 0) {
        printf("get [PiHealthLog] failed\n");
        exit(EXIT_FAILURE);
    }
    init_daemon();
    pid_t pid, son;
    struct epoll_event ev, events[10];
    int ctl_listen_socket, conn_sock, nfds, epollfd;
    struct sockaddr_in client;
    unsigned int addrlen = sizeof(client);

    int shmid;
    void *share_memory = NULL;
    pthread_mutexattr_t m_attr;
    pthread_condattr_t c_attr;

    pthread_mutexattr_init(&m_attr);
    pthread_condattr_init(&c_attr);

    pthread_mutexattr_setpshared(&m_attr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&c_attr, PTHREAD_PROCESS_SHARED);

    if ((shmid = shmget(IPC_PRIVATE, sizeof(struct sm_msg), 0666 | IPC_CREAT)) == -1) {
        //perror("shmget");
        my_log(PiHealthLog, "[Error] [main : shmget] : %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((share_memory = shmat(shmid, 0, 0)) == NULL) {
        //perror("shmat");
        my_log(PiHealthLog, "[Error] [mian : shmat] : %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct sm_msg *msg = (struct sm_msg *)share_memory;

    pthread_mutex_init(&(msg->sm_mutex), &m_attr);
    pthread_cond_init(&msg->sm_ready, &c_attr);

    if (get_conf(CONF, "ctl_client_port", ctl_client_port) < 0) {
        //printf("get ctl_client_port failed\n");
        my_log(PiHealthLog, "[Error] [get_conf] : get [ctl_client_port] failed\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "heartbeat_client_port",heartbeat_client_port) < 0) {
        //printf("get heartbeat_client_port failed\n");
        my_log(PiHealthLog, "[Error] [get_conf] : get [heartbeat_client_port] failed\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "master_port", master_port) < 0) {
        //printf("get master port failed\n");
        my_log(PiHealthLog, "[Error] [get_conf] : get [master_port] failed\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "master", master) < 0) {
        //printf("get Master failed\n");
        my_log(PiHealthLog, "[Error] [get_conf] : get [master] failed\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "datadir", datadir) < 0) {
        //printf("get datadir failed\n");
        my_log(PiHealthLog, "[Error] [get_conf] : get [datadir] failed\n");
        exit(EXIT_FAILURE);
    }
    mkdir(datadir, 0755);
    if (get_conf(CONF, "scriptdir", scriptdir) < 0) {
        //printf("get scriptdir failed\n");
        my_log(PiHealthLog, "[Error] [get_conf] : get [scriptdir] failed\n");
        exit(EXIT_FAILURE);
    }

    ctl_listen_socket = create_listen_socket(atoi(ctl_client_port));
    if (ctl_listen_socket < 0) {
        //printf("create control listen socket failed\n");
        my_log(PiHealthLog, "[Error] [create_listen_socket] : %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        //perror("epoll_create1");
        my_log(PiHealthLog, "[Error] [epoll_create1] : %s\n", strerror(errno));
        close(ctl_listen_socket);
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = ctl_listen_socket;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ctl_listen_socket, &ev) == -1) {
        //perror("epoll_ctl: ctl_listen_socket");
        my_log(PiHealthLog, "[Error] [epoll_ctl] : %s\n", strerror(errno));
        close(ctl_listen_socket);
        close(epollfd);
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == 0) {
        son = fork();
        signal(SIGCHLD, handler);    //处理子进程，防止僵尸进程的产生
        if (son != 0) {
            while (1) {
                //printf("子进程等待信号开始心跳\n");
                pthread_mutex_lock(&msg->sm_mutex);
                pthread_cond_wait(&msg->sm_ready, &msg->sm_mutex);
                pthread_mutex_unlock(&msg->sm_mutex);
                heartbeating();
            }
        } else {
            //printf("孙子进程开始自检\n");
            int n = 10, flag = 0;
            for (int i = 0; i < n; i++) {
                if (heartbeat(atoi(master_port), master) < 0) {
                    fflush(stdout);
                    //printf("\033[31m * \033[0m");
                    //printf("❤️  ");
                    sleep(1);
                } else {
                    //printf("\n心跳成功\n");
                    flag = 1;
                    break;
                }
            }
            //printf("\n");
            if (flag == 0) {
                //printf("孙子进程给父进程发送心跳信号，开启心跳进程\n");
                pthread_mutex_lock(&msg->sm_mutex);
                pthread_cond_signal(&msg->sm_ready);
                pthread_mutex_unlock(&msg->sm_mutex);
            }
        }
    } else {
        pthread_t log;

        //log process
        if (pthread_create(&log, NULL, generate_log, NULL) < 0) {
            //perror("pthread_create");
            my_log(PiHealthLog, "[Error] [pthread_create] : %s\n", strerror(errno));
            close(ctl_listen_socket);
            close(epollfd);
            exit(EXIT_FAILURE);
        }
        pthread_t listen_heart;
        if (pthread_create(&listen_heart, NULL, listen_heartbeat, NULL) < 0) {
            //perror("pthread_create");
            my_log(PiHealthLog, "[Error] [pthread_create] : %s\n", strerror(errno));
            close(ctl_listen_socket);
            close(epollfd);
            exit(EXIT_FAILURE);
        }

        while (1) {
            //printf("epoll wait...\n");
           	nfds = epoll_wait(epollfd, events, 1, 30000);
            if (nfds == -1) {
                //perror("epoll_wait");
                my_log(PiHealthLog, "[Error] [epoll_wait] : %s\n", strerror(errno));
                close(ctl_listen_socket);
                close(epollfd);
                exit(EXIT_FAILURE);
            } else if (nfds == 0) {
                //printf("\nmaster端30s无连接，发送心跳信号，开启心跳进程\n");
                pthread_mutex_lock(&msg->sm_mutex);
                pthread_cond_signal(&msg->sm_ready);
                pthread_mutex_unlock(&msg->sm_mutex);
                continue;
            }
            for (int n = 0; n < nfds; n++) {
                if (events[n].data.fd == ctl_listen_socket) {
                    conn_sock = accept(ctl_listen_socket, (struct sockaddr *) &client, &addrlen);
                    if (conn_sock == -1) {
                        //perror("accept");
                        my_log(PiHealthLog, "[Error] [accept] : %s\n", strerror(errno));
                        close(conn_sock);
                        close(ctl_listen_socket);
                        close(epollfd);
                        exit(EXIT_FAILURE);
                    }
                    getpeername(conn_sock, (struct sockaddr *)&client, &addrlen);
                    //printf("recv master %s : %d control success\n", inet_ntoa((client.sin_addr)), ntohs(client.sin_port));
                    int imode = 1;
                    //ioctl(conn_sock, FIONBIO, &imode); //将新连接设置成非阻塞状态
                    ev.events = EPOLLOUT; //将新连接加入epoll监听
                    ev.data.fd = conn_sock;
                    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
                        //perror("epoll_ctl: conn_sock");
                        my_log(PiHealthLog, "[Error] [epoll_ctl] : %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                } else if (events[n].events & EPOLLOUT) {
                    conn_sock = events[n].data.fd;
                    char log_filename[6][100] = {"Cpu.log", "Mem.log", "Disk.log", "Process.log", "User.log", "Sys.log"};
                    getpeername(conn_sock, (struct sockaddr *)&client, &addrlen);
                    for (int i = 0; i < 6; i++) {
                        Log *log = (Log *)malloc(sizeof(Log));
                        log->flag = i;
                        FILE *fp = NULL;
                        char logdir[100] = {0};
                        sprintf(logdir, "%s/%s", datadir, log_filename[i]);
                        fp = fopen(logdir, "a+");
                        if (fp == NULL) {
                            //printf("open %s error\n", logdir);
                            my_log(PiHealthLog, "[Error] [fopen] : %s\n", strerror(errno));
                            break;
                        }
                        if (flock(fileno(fp), LOCK_EX) < 0) {
                            //perror("flock");
                            my_log(PiHealthLog, "[Error] [flock] : %s\n", strerror(errno));
                            fclose(fp);
                            break;
                        }
                        //printf("flag = %d\n", log->flag);
                        char ch = fgetc(fp);
                        if (ch == EOF) {
                            //printf("file empty !\n");
                            fclose(fp);
                            continue;
                        }
                        fseek(fp, 0, SEEK_SET);
                        //printf("flag = %d\n", log->flag);
                        //printf("\033[32msend file ...\033[0m\n");
                        while (!feof(fp)) {
                            memset(log->data, 0, sizeof(log->data));
                            int j = fread(log->data, 1, MAX_SIZE, fp);
                            int ret = send(conn_sock, log, sizeof(Log), 0);
                            if (ret < 0) {
                                //printf("\033[31msend %s fail\033[0m\n", log_filename[i]);
                                my_log(PiHealthLog, "[Error] [send] : %s\n", strerror(errno));
                                break;
                            }
                        }
                        //printf("\033[32msend complete\033[0m\n");
                        //my_log(PiHealthLog, "[Note] [send] : send complete\n");
                        //printf("start clear log\n");
                        FILE *fp_clear = NULL;
                        fp_clear = fopen(logdir, "w");
                        if (fp_clear == NULL) {
                            //printf("open file error\n");
                            my_log(PiHealthLog, "[Error] [fopen] : %s\n", strerror(errno));
                            fclose(fp);
                            break;
                        }
                        fclose(fp);
                        fclose(fp_clear);
                        //printf("\033[32mclear success\033[0m\n");
                    }
                    close(conn_sock);
                }
            }
        }
    }
    return 0;
}
