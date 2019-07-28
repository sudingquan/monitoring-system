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
#include "common.h"
#define CONF "client_conf"
#define MAX_BUFF 100

char heartbeat_client_port[10];
char ctl_client_port[10];
char master_port[10];
char master[20];

void handler(int sig) {	
	while (waitpid(-1, NULL, WNOHANG) > 0) {
		printf("成功处理一个子进程的退出\n");
	}
    return ;
}

void heartbeating() {
    printf("子进程接收到信号，开始心跳\n");
    int sleep_time = 1;
    for (int i = 1; ; i++) {
        printf("第 %d 次 : \n", i);
        int j = i;
        while (j--) {
            if (heartbeat(atoi(master_port), master) < 0) {
                fflush(stdout);
                printf("❤️  ");
                sleep(1);
            } else {
                printf("心跳成功\n");
                return ;
            }
        }
        printf("\n");
        sleep(sleep_time);
        sleep_time++;
    }
}

void *generate_log(void *a) {
    printf("generate log child pthread start !\n");
    FILE *pp=NULL;
    FILE *fp=NULL;
    char buff[128]={0};
    memset(buff,0,sizeof(buff));
    pp=popen("bash ../script/CpuLog.sh","r");
    fp=fopen("test.log","a+");
    fread(buff,1,127,pp);
    fwrite(buff,1,127,fp);
    pclose(pp);
    fclose(fp);
}

int main() {
    pid_t pid, son;
    struct epoll_event ev, events[10];
    int heartbeat_listen_socket, ctl_listen_socket, conn_sock, nfds, epollfd;
    struct sockaddr_in client;
    unsigned int addrlen = sizeof(client);
    char buff[MAX_BUFF] = {0};

    if (get_conf(CONF, "ctl_client_port", ctl_client_port) < 0) {
        printf("get ctl_client_port failed\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "heartbeat_client_port",heartbeat_client_port) < 0) {
        printf("get heartbeat_client_port failed\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "master_port", master_port) < 0) {
        printf("get master port failed\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "master", master) < 0) {
        printf("get Master failed\n");
        exit(EXIT_FAILURE);
    }

    heartbeat_listen_socket = create_listen_socket(atoi(heartbeat_client_port));
    if (heartbeat_listen_socket < 0) {
        printf("create heartbeat listen socket failed\n");
        exit(EXIT_FAILURE);
    }

    ctl_listen_socket = create_listen_socket(atoi(ctl_client_port));
    if (ctl_listen_socket < 0) {
        printf("create control listen socket failed\n");
        exit(EXIT_FAILURE);
    }

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        close(heartbeat_listen_socket);
        close(ctl_listen_socket);
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = heartbeat_listen_socket;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, heartbeat_listen_socket, &ev) == -1) {
        perror("epoll_ctl: heartbeat_listen_socket");
        close(heartbeat_listen_socket);
        close(ctl_listen_socket);
        close(epollfd);
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = ctl_listen_socket;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ctl_listen_socket, &ev) == -1) {
        perror("epoll_ctl: ctl_listen_socket");
        close(heartbeat_listen_socket);
        close(ctl_listen_socket);
        close(epollfd);
        exit(EXIT_FAILURE);
    }
    signal(SIGCHLD,  handler);    //处理子进程，防止僵尸进程的产生
    pid = fork();
    if (pid == 0) {
        son = fork();
        if (son != 0) {
            printf("子进程等待信号开始心跳\n");
            signal(10, heartbeating);
            while (1) {
                pause();
            }
        } else {
            printf("孙子进程开始自检\n");
            int n = 10, flag = 0;
            for (int i = 0; i < n; i++) {
                if (heartbeat(atoi(master_port), master) < 0) {
                    fflush(stdout);
                    //printf("\033[31m * \033[0m");
                    printf("❤️  ");
                    sleep(1);
                } else {
                    printf("\n心跳成功\n");
                    flag = 1;
                    break;
                }
            }
            printf("\n");
            if (flag == 0) {
                printf("孙子进程给父进程发送心跳信号，开启心跳进程\n");
                kill(getppid(), 10);
            }
        }
    } else {
        pthread_t log;

        //log process
        /*
        if (pthread_create(&log, NULL, generate_log, NULL) < 0) {
            perror("pthread_create");
            close(heartbeat_listen_socket);
            close(ctl_listen_socket);
            close(epollfd);
            exit(EXIT_FAILURE);
        }
        */

        while (1) {
            printf("epoll wait...\n");
           	nfds = epoll_wait(epollfd, events, 1, 30000);
            if (nfds == -1) {
                perror("epoll_wait");
                close(heartbeat_listen_socket);
                close(ctl_listen_socket);
                close(epollfd);
                exit(EXIT_FAILURE);
            } else if (nfds == 0) {
                 printf("\nmaster端30s无连接，发送心跳信号，开启心跳进程\n");
                 kill(pid, 10);
                 continue;
            }
            for (int n = 0; n < nfds; n++) {
                if (events[n].data.fd == heartbeat_listen_socket) {
                    conn_sock = accept(heartbeat_listen_socket, (struct sockaddr *) &client, &addrlen);
                    if (conn_sock == -1) {
                        perror("accept");
                        close(conn_sock);
                        close(heartbeat_listen_socket);
                        close(ctl_listen_socket);
                        close(epollfd);
                        exit(EXIT_FAILURE);
                    }
                    getpeername(conn_sock, (struct sockaddr *)&client, &addrlen);
                    printf("recv master %s : %d  ❤️   success\n", inet_ntoa((client.sin_addr)), ntohs(client.sin_port));
                    close(conn_sock);
                } else if (events[n].data.fd == ctl_listen_socket) {
                    conn_sock = accept(ctl_listen_socket, (struct sockaddr *) &client, &addrlen);
                    if (conn_sock == -1) {
                        perror("accept");
                        close(conn_sock);
                        close(heartbeat_listen_socket);
                        close(ctl_listen_socket);
                        close(epollfd);
                        exit(EXIT_FAILURE);
                    }
                    getpeername(conn_sock, (struct sockaddr *)&client, &addrlen);
                    printf("recv master %s : %d control success\n", inet_ntoa((client.sin_addr)), ntohs(client.sin_port));
                    int imode = 1;
                    ioctl(conn_sock, FIONBIO, &imode); //将新连接设置成非阻塞状态
                    ev.events = EPOLLIN; //将新连接加入epoll监听
                    ev.data.fd = conn_sock;
                    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
                        perror("epoll_ctl: conn_sock");
                        exit(EXIT_FAILURE);
                    }
                } else if(events[n].events & EPOLLIN) {
                    conn_sock = events[n].data.fd;
                    int ret = recv(conn_sock, buff, MAX_BUFF, 0);
                    getpeername(conn_sock, (struct sockaddr *)&client, &addrlen);
                    if (ret < 0) {
                        perror("recv");
                    } else if (ret = 0) {
                        printf("recv from <%s> \n\033[31mfailed\033[0m !\n", inet_ntoa(client.sin_addr), buff);
                    } else {
                        printf("recv from <%s> : %s \n\033[32msuccess\033[0m !\n", inet_ntoa(client.sin_addr), buff);
                    }
                    ev.events = EPOLLOUT;
                    ev.data.fd = conn_sock;
                    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, conn_sock, &ev) < 0) {
                        perror("epoll_ctl: conn_sock");
                        exit(EXIT_FAILURE);
                    }
                } else if (events[n].events & EPOLLOUT) {
                    conn_sock = events[n].data.fd;
                    int ret = send(conn_sock, buff, strlen(buff), 0);
                    getpeername(conn_sock, (struct sockaddr *)&client, &addrlen);
                    if (ret < 0) {
                        perror("send");
                    } else if (ret = 0) {
                        printf("send to <%s> : %s \n\033[31mfailed\033[0m !\n", inet_ntoa(client.sin_addr), buff);
                    } else {
                        printf("send to <%s> : %s \n\033[32msuccess\033[0m !\n", inet_ntoa(client.sin_addr), buff);
                    }
                    ev.events = EPOLLOUT;
                    ev.data.fd = conn_sock;
                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, conn_sock, &ev) < 0) {
                        perror("epoll_ctl: conn_sock");
                        exit(EXIT_FAILURE);
                    }
                    close(conn_sock);
                }
            }
        }
    }
    return 0;
}
