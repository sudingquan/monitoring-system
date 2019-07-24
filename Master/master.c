/*************************************************************************
	> File Name: master.c
	> Author: sudingquan
	> Mail: 1151015256@qq.com
	> Created Time: 六  7/20 14:45:06 2019
 ************************************************************************/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "common.h"
#define CONF "master_conf"

LinkList **link_client;

char master_port[10];
char client_port[10];
char from[20];
char to[20];
char ins[5];

void *continue_heartbeat(void *a) {
    printf("heartbeat child pthread start\n");
    while (1) {
        for (int i = 0; i < atoi(ins); i++) {
            printf("LinkList <%d> online client number : %d\n", i, link_client[i]->length);
            for (ListNode *q = link_client[i]->head.next, *p = &(link_client[i]->head); q;) {
                fflush(stdout);
                if (heartbeat(ntohs(q->data.sin_port), inet_ntoa(q->data.sin_addr)) == 0) {
                    printf("%s : %d \033[32monline\033[0m !\n", inet_ntoa(q->data.sin_addr), ntohs(q->data.sin_port));
                    p = p->next;
                    q = q->next;
                } else {
                    printf("%s : %d \033[31mdeleting\033[0m...\n", inet_ntoa(q->data.sin_addr), ntohs(q->data.sin_port));
                    p->next = q->next;
                    clear_listnode(q);
                    q = p->next;
                    link_client[i]->length--;
                }
            }
            printf("End of traversal : LinkList <%d> !\n", i);
            sleep(1);
        }
        printf("End of all traversal !\n");
        sleep(3);
    }
}

void *do_events(void *i) {
    int id = *(int *)i;
    while (1) {
        struct epoll_event ev, events[link_client[id]->length];
        int conn_sock, nfds, epollfd;
    }
}

int main() {
    pthread_t connect_client[atoi(ins)], heartbeat;
    struct epoll_event ev, events[1];
    int listen_socket, conn_sock, nfds, epollfd;
    struct sockaddr_in client;
    unsigned int addrlen = sizeof(client);
    if (get_conf(CONF, "master_port", master_port) < 0) {
        printf("get master_port failed\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "client_port", client_port) < 0) {
        printf("get client_port failed\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "from", from) < 0) {
        printf("get from failed\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "to", to) < 0) {
        printf("get to failed\n");
        exit(EXIT_FAILURE);
    }

    listen_socket = create_listen_socket(atoi(master_port));
    if (listen_socket < 0) {
        printf("create listen socket failed\n");
        exit(EXIT_FAILURE);
    }
    if (get_conf(CONF, "INS", ins) < 0) {
        printf("get master_port failed\n");
        exit(EXIT_FAILURE);
    }

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        close(listen_socket);
        exit(EXIT_FAILURE);
    }

    link_client = init_linklist(atoi(ins));
    if (link_client == NULL) {
        exit(1);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listen_socket;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_socket, &ev) == -1) {
        perror("epoll_ctl: listen_socket");
        close(listen_socket);
        close(epollfd);
        exit(EXIT_FAILURE);
    }

    printf("from %s to %s\n", from, to);
    int ip_num = htonl(inet_addr(to)) - htonl(inet_addr(from)) + 1;
    for (int i = 0; i < ip_num; i++) {
        if ((htonl(inet_addr(from)) + i) % 256 == 0 || (htonl(inet_addr(from)) + i) % 256 == 255) {
            continue;
        }
        client.sin_family = AF_INET;
	    client.sin_port = htons(atoi(client_port));
	    client.sin_addr.s_addr = ntohl(htonl(inet_addr(from)) + i);
        int min = min_length(link_client, atoi(ins));
        if (insert(link_client[min], link_client[min]->length, client) > 0) {
            printf("insert %s to LinkList <%d> success\n", inet_ntoa((client.sin_addr)), min);
        } else {
            printf("insert %s to LinkList <%d> failed\n", inet_ntoa((client.sin_addr)), min);
            close(listen_socket);
            close(epollfd);
            exit(EXIT_FAILURE);
        }
    }
    //heartbeat
    if (pthread_create(&heartbeat, NULL, continue_heartbeat, NULL)) {
        perror("pthread_create");
        close(listen_socket);
        close(epollfd);
        exit(EXIT_FAILURE);
    }

    //data processing
    for (int i = 0; i < atoi(ins); i++) {
        if (pthread_create(&connect_client[i], NULL, do_events, (void *)&(link_client[i]->id))) {
            perror("pthread_create");
            close(listen_socket);
            close(epollfd);
            exit(EXIT_FAILURE);
        }
    }

    while (1) {
        printf("epoll wait...\n");
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
	        client.sin_port = htons(atoi(client_port));
            int min = min_length(link_client, atoi(ins));
            int in;
            if (already_in_linklist(link_client, atoi(ins), client, &in) == 0) {
                printf("insert %s to LinkList <%d> failed : Already in LinkList <%d>\n", inet_ntoa((client.sin_addr)), min, in);
                close(conn_sock);
                continue;
            }
            if (insert(link_client[min], link_client[min]->length, client) > 0) {
                printf("insert %s to LinkList <%d> success\n", inet_ntoa((client.sin_addr)), min);
            } else {
                printf("insert %s to LinkList <%d> failed\n", inet_ntoa((client.sin_addr)), min);
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
