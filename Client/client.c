/*************************************************************************
	> File Name: client.c
	> Author: sudingquan
	> Mail: 1151015256@qq.com
	> Created Time: 六  7/20 14:31:28 2019
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <signal.h>
#include "common.h"
#define CONF "client_conf"

char client_port[10];
char master_port[10];
char master[20];

void heartbeating() {
    printf("孙子进程接收到信号，开始心跳\n");
    int sleep_time = 1;
    for (int i = 1; ; i++) {
        printf("第 %d 次 : \n", i);
        int j = i;
        while (j--) {
            if (heartbeat(atoi(master_port), master) < 0) {
                fflush(stdout);
                //printf("\033[31m * \033[0m");
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

int main() {
    pid_t pid;
    struct epoll_event ev, events[1];
    int listen_socket, conn_sock, nfds, epollfd;
    struct sockaddr_in client;
    unsigned int addrlen = sizeof(client);

    if (get_conf(CONF, "client_port", client_port) < 0) {
        printf("get ClientPort failed\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "master_port", master_port) < 0) {
        printf("get Master failed\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "master", master) < 0) {
        printf("get Master failed\n");
        exit(EXIT_FAILURE);
    }
    listen_socket = create_listen_socket(atoi(client_port));
    if (listen_socket < 0) {
        printf("create listen socket failed\n");
        exit(EXIT_FAILURE);
    }

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        close(listen_socket);
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listen_socket;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_socket, &ev) == -1) {
        perror("epoll_ctl: listen_socket");
        close(listen_socket);
        close(epollfd);
        exit(EXIT_FAILURE);
    }
    pid = fork();
    if (pid == 0) {
        pid_t son;
        son = fork();
        if (son == 0) {
            printf("孙子进程等待信号开始心跳\n");
            signal(10, heartbeating);
            pause();
        } else {
            int n = 10, flag = 0;
            for (int i = 0; i < n; i++) {
                if (heartbeat(atoi(master_port), master) < 0) {
                    fflush(stdout);
                    //printf("\033[31m * \033[0m");
                    printf("❤️  ");
                    sleep(1);
                } else {
                    printf("心跳成功\n");
                    flag = 1;
                    break;
                }
            }
            printf("\n");
            if (flag == 0) {
                printf("子进程发送心跳信号，开启心跳进程\n");
                kill(son, 10);
            }
        }
    } else {
        while (1) {
           	nfds = epoll_wait(epollfd, events, 1, -1);
            if (nfds == -1) {
                perror("epoll_wait");
                close(listen_socket);
                close(epollfd);
                exit(EXIT_FAILURE);
            }
            if (events[0].data.fd == listen_socket) {
                conn_sock = accept(listen_socket, (struct sockaddr *) &client, &addrlen);
                if (conn_sock == -1) {
                    perror("accept");
                    close(conn_sock);
                    close(listen_socket);
                    close(epollfd);
                    exit(EXIT_FAILURE);
                }
                getpeername(conn_sock, (struct sockaddr *)&client, &addrlen);
                printf("connect to master %s : %d success\n", inet_ntoa((client.sin_addr)), ntohs(client.sin_port));

                close(conn_sock);
            }
        }
    }
    return 0;
}
