/*************************************************************************
	> File Name: master.c
	> Author: sudingquan
	> Mail: 1151015256@qq.com
	> Created Time: 六  7/20 14:45:06 2019
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include "common.h"
#define CONF "/tmp/master.conf"
#define MAX_SIZE 1024

char master_port[10];
char client_heartbeat_port[10];
char client_ctl_port[10];
char from[20];
char to[20];
char ins[5];
char datadir[100];
char PiHealthLog[100];

LinkList **link_client;

typedef struct Log {
    char data[MAX_SIZE + 5];
    int flag;
} Log;

void *continue_heartbeat(void *a) {
    //printf("heartbeat child pthread start\n");
    my_log(PiHealthLog, "[Note] [continue_heartbeat] : heartbeat child pthread start\n");
    while (1) {
        for (int i = 0; i < atoi(ins); i++) {
            //printf("LinkList <%d> online client number : %d\n", i, link_client[i]->length);
            //my_log(PiHealthLog, "[Note] [continue_heartbeat] : LinkList <%d> online client number : %d\n", i, link_client[i]->length);
            for (ListNode *q = link_client[i]->head.next, *p = &(link_client[i]->head); q;) {
                //fflush(stdout);
                if (heartbeat(ntohs(q->data.sin_port), inet_ntoa(q->data.sin_addr)) == 0) {
                    //printf("%s : %d \033[32monline\033[0m !\n", inet_ntoa(q->data.sin_addr), ntohs(q->data.sin_port));
                    p = p->next;
                    q = q->next;
                } else {
                    //printf("%s : %d \033[31mdeleting\033[0m...\n", inet_ntoa(q->data.sin_addr), ntohs(q->data.sin_port));
                    my_log(PiHealthLog, "[Note] [continue_heartbeat] : %s:%d offline\n", inet_ntoa(q->data.sin_addr), ntohs(q->data.sin_port));
                    p->next = q->next;
                    clear_listnode(q);
                    q = p->next;
                    link_client[i]->length--;
                }
            }
            //printf("End of traversal : LinkList <%d> !\n", i);
            sleep(1);
        }
        //printf("End of all traversal !\n");
        sleep(3);
    }
}

void *do_event(void *i) {
    int id = *(int *)i;
    //printf("send and recv data pthread %d start !\n", id);
    my_log(PiHealthLog, "[Note] [do_event] : send and recv pthread %d start !\n", id);
    while (1) {
        if (link_client[id]->length == 0) {
            //printf("link_client <%d> is empty !\n", id);
            //printf("sleep 5s\n");
            sleep(5);
            continue;
        }
        int events_num = link_client[id]->length;
        struct epoll_event ev, events[events_num];
        int conn_sock, nfds, epollfd;
        struct sockaddr_in client_addr;
        unsigned int addrlen = sizeof(client_addr);
        ListNode *p;

        epollfd = epoll_create1(0);
        if (epollfd == -1) {
            //perror("epoll_create1");
            my_log(PiHealthLog, "[Error] [do_event : epoll_create1] : %s\n", strerror(errno));
            continue;
        }

        for (p = link_client[id]->head.next; p; p = p->next) {
            int sockfd;
            client_addr = p->data;
            client_addr.sin_port = htons(atoi(client_ctl_port));
	        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		        //perror("socket() error");
                my_log(PiHealthLog, "[Error] [do_event : socket] : %s\n", strerror(errno));
		        continue;
	        }

            //unsigned int imode = 1;
            //ioctl(sockfd, FIONBIO, &imode);
            if (connect(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
                //perror("connect");
                my_log(PiHealthLog, "[Error] [do_event : connect] : %s\n", strerror(errno));
                continue;
            }

            ev.events = EPOLLIN;
            ev.data.fd = sockfd;
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
                //perror("epoll_ctl: sockfd");
                my_log(PiHealthLog, "[Error] [do_event : epoll_ctl] : %s\n", strerror(errno));
                continue;
            }
        }
        while (1) {
            nfds = epoll_wait(epollfd, events, events_num, 300);
            if (nfds == -1) {
                //perror("epoll_wait");
                my_log(PiHealthLog, "[Error] [do_event : epoll_wait] : %s\n", strerror(errno));
                continue;
            } else if (nfds == 0) {
                //printf("epoll wait timeout\n");
                break;
            }
            for (int n = 0; n < nfds; n++) {
                if (events[n].events & EPOLLIN) {
                    int client = events[n].data.fd;
                    int log_size = sizeof(Log);
                    Log *log = (Log *)malloc(sizeof(Log));
                    getpeername(client, (struct sockaddr *)&client_addr, &addrlen);
                    char dir_path[100];
                    sprintf(dir_path, "%s/%s", datadir, inet_ntoa(client_addr.sin_addr));
                    mkdir(dir_path, 0755);
                    char log_filename[6][20] = {"Cpu.log", "Mem.log", "Disk.log", "Process.log", "User.log", "Sys.log"};
                    while (1) {
                        //printf("\033[32mrecv file ...\033[0m\n");
                        //my_log(PiHealthLog, "[Note] [do_event] : recv file form %s\n", inet_ntoa(client_addr.sin_addr));
                        memset(log->data, 0, sizeof(log->data));
                        int j = recv(client, (char *)log, log_size, 0);
                        int end = 0;
                        if (j == 0) {
                            break;
                        }
                        while (j != log_size) {
                            //printf("\033[31m发生粘包\033[0m");
                            int left = log_size - j;
                            int i = recv(client, ((char *)log) + j, left, 0);
                            if (i == 0) {
                                end = 1;
                                break;
                            }
                            j += i;
                        }
                        if (end == 1) {
                            break;
                        }
                        //printf("%s\n", log->data);
                        //printf("\033[32mrecv complete\033[0m\n");
                        //my_log(PiHealthLog, "[Note] [do_event] : recv file form %s complete\n", inet_ntoa(client_addr.sin_addr));
                        char path[100] = {0};
                        FILE *fp = NULL;
                        //printf("log filename is %s\n", log_filename[log->flag]);
                        sprintf(path, "%s/%s/%s", datadir, inet_ntoa(client_addr.sin_addr), log_filename[log->flag]);
                        //printf("log path is %s\n", path);
                        fp = fopen(path, "a");
                        if (fp == NULL) {
                            perror("fopen:fp");
                            break;
                        }
                        //printf("\033[32mwriting ...\033[0m\n");
                        //my_log(PiHealthLog, "[Note] [do_event] : write data form %s to %s\n", inet_ntoa(client_addr.sin_addr), path);
                        fwrite(log->data, 1, strlen(log->data), fp);
                        //printf("\033[32mwriting complete\033[0m\n");
                        fclose(fp);
                    }
                    //printf("recv complete, close connect\n");
                    //my_log(PiHealthLog, "[Note] [do_event] : close connect %s\n", inet_ntoa(client_addr.sin_addr));
                    close(client);
                }
            }
        }
        close(epollfd);
        sleep(5);
    }
}

int main() {
    if (get_conf(CONF, "PiHealthLog", PiHealthLog) < 0) {
        printf("get [PiHealthLog] failed\n");
        exit(EXIT_FAILURE);
    }
    init_daemon();
    pthread_t connect_client[atoi(ins)], heartbeat;
    struct epoll_event ev, events[1];
    int listen_socket, conn_sock, nfds, epollfd;
    struct sockaddr_in client;
    unsigned int addrlen = sizeof(client);
    if (get_conf(CONF, "master_port", master_port) < 0) {
        //printf("get master_port failed\n");
        my_log(PiHealthLog, "[Error] [getconf] : get [master_port] failed\n");
        my_log(PiHealthLog, "exit\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "client_heartbeat_port", client_heartbeat_port) < 0) {
        //printf("get client heartbeat port failed\n");
        my_log(PiHealthLog, "[Error] [getconf] : get [client_heartbeat_port] failed\n");
        my_log(PiHealthLog, "exit\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "client_ctl_port", client_ctl_port) < 0) {
        //printf("get client ctl port failed\n");
        my_log(PiHealthLog, "[Error] [getconf] : get [client_ctl_port] failed\n");
        my_log(PiHealthLog, "exit\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "from", from) < 0) {
        //printf("get from failed\n");
        my_log(PiHealthLog, "[Error] [getconf] : get [from] failed\n");
        my_log(PiHealthLog, "exit\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "to", to) < 0) {
        //printf("get to failed\n");
        my_log(PiHealthLog, "[Error] [getconf] : get [to] failed\n");
        my_log(PiHealthLog, "exit\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "INS", ins) < 0) {
        //printf("get INS failed\n");
        my_log(PiHealthLog, "[Error] [getconf] : get [INS] failed\n");
        my_log(PiHealthLog, "exit\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "datadir", datadir) < 0) {
        //printf("get datadir failed\n");
        my_log(PiHealthLog, "[Error] [getconf] : get client ctl port failed\n");
        my_log(PiHealthLog, "exit\n");
        exit(EXIT_FAILURE);
    }
    mkdir(datadir, 0755);

    listen_socket = create_listen_socket(atoi(master_port));
    if (listen_socket < 0) {
        //printf("create listen socket failed\n");
        my_log(PiHealthLog, "[Error] [create_listen_socket] : %s\n", strerror(errno));
        my_log(PiHealthLog, "exit\n");
        exit(EXIT_FAILURE);
    }

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        my_log(PiHealthLog, "[Error] [epoll_create1] : %s\n", strerror(errno));
        my_log(PiHealthLog, "exit\n");
        close(listen_socket);
        exit(EXIT_FAILURE);
    }

    link_client = init_linklist(atoi(ins));
    if (link_client == NULL) {
        my_log(PiHealthLog, "[Error] [init_linklist] : %s\n", strerror(errno));
        my_log(PiHealthLog, "exit\n");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listen_socket;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_socket, &ev) == -1) {
        my_log(PiHealthLog, "[Error] [epoll_ctl : listen_socket] : %s\n", strerror(errno));
        my_log(PiHealthLog, "exit\n");
        close(listen_socket);
        close(epollfd);
        exit(EXIT_FAILURE);
    }

    //printf("from %s to %s\n", from, to);
    int ip_num = htonl(inet_addr(to)) - htonl(inet_addr(from)) + 1;
    for (int i = 0; i < ip_num; i++) {
        if ((htonl(inet_addr(from)) + i) % 256 == 0 || (htonl(inet_addr(from)) + i) % 256 == 255) {
            continue;
        }
        client.sin_family = AF_INET;
	    client.sin_port = htons(atoi(client_heartbeat_port));
	    client.sin_addr.s_addr = ntohl(htonl(inet_addr(from)) + i);
        int min = min_length(link_client, atoi(ins));
        if (insert(link_client[min], link_client[min]->length, client) > 0) {
            //printf("insert %s to LinkList <%d> success\n", inet_ntoa((client.sin_addr)), min);
        } else {
            //printf("insert %s to LinkList <%d> failed\n", inet_ntoa((client.sin_addr)), min);
            my_log(PiHealthLog, "[Error] [insert] : insert %s to LinkList <%d> failed\n", inet_ntoa((client.sin_addr)), min);
            my_log(PiHealthLog, "exit\n");
            close(listen_socket);
            close(epollfd);
            exit(EXIT_FAILURE);
        }
    }
    //heartbeat
    if (pthread_create(&heartbeat, NULL, continue_heartbeat, NULL) < 0) {
        //perror("pthread_create");
        my_log(PiHealthLog, "[Error] [pthread_create] : %s\n", strerror(errno));
        my_log(PiHealthLog, "exit\n");
        close(listen_socket);
        close(epollfd);
        exit(EXIT_FAILURE);
    }

    //data processing
    for (int i = 0; i < atoi(ins); i++) {
        if (pthread_create(&connect_client[i], NULL, do_event, (void *)&(link_client[i]->id)) < 0) {
            //perror("pthread_create");
            my_log(PiHealthLog, "[Error] [pthread_create] : %s\n", strerror(errno));
            my_log(PiHealthLog, "exit\n");
            close(listen_socket);
            close(epollfd);
            exit(EXIT_FAILURE);
        }
    }

    while (1) {
        //printf("epoll wait...\n");
        nfds = epoll_wait(epollfd, events, 1, -1);
        if (nfds == -1) {
            //perror("epoll_wait");
            my_log(PiHealthLog, "[Error] [epoll_wait] : %s\n", strerror(errno));
            my_log(PiHealthLog, "exit\n");
            close(listen_socket);
            close(epollfd);
            exit(EXIT_FAILURE);
        }
        if (events[0].data.fd == listen_socket) {
            conn_sock = accept(listen_socket, (struct sockaddr *) &client, &addrlen);
            if (conn_sock == -1) {
                //perror("accept");
                my_log(PiHealthLog, "[Error] [accept] : %s\n", strerror(errno));
                my_log(PiHealthLog, "exit\n");
                close(conn_sock);
                close(listen_socket);
                close(epollfd);
                exit(EXIT_FAILURE);
            }
            getpeername(conn_sock, (struct sockaddr *)&client, &addrlen);
	        client.sin_port = htons(atoi(client_heartbeat_port));
            int min = min_length(link_client, atoi(ins));
            int in;
            if (already_in_linklist(link_client, atoi(ins), client, &in) == 0) {
                //printf("insert %s to LinkList <%d> failed : Already in LinkList <%d>\n", inet_ntoa((client.sin_addr)), min, in);
                //my_log(PiHealthLog, "[Note] [insert] : insert %s to LinkList <%d> failed : Already in LinkList <%d>\n", inet_ntoa((client.sin_addr)), min, in);
                close(conn_sock);
                continue;
            }
            if (insert(link_client[min], link_client[min]->length, client) > 0) {
                //printf("insert %s to LinkList <%d> success\n", inet_ntoa((client.sin_addr)), min);
                my_log(PiHealthLog, "[Note] [insert] : %s online\n", inet_ntoa(client.sin_addr));

            } else {
                //printf("insert %s to LinkList <%d> failed\n", inet_ntoa((client.sin_addr)), min);
                my_log(PiHealthLog, "[Error] [insert] : insert %s to LinkList <%d> failed\n", inet_ntoa((client.sin_addr)), min);
                my_log(PiHealthLog, "exit\n");
                close(conn_sock);
                close(listen_socket);
                close(epollfd);
                exit(EXIT_FAILURE);
            }
            close(conn_sock);
        }
    }
    return 0;
}
