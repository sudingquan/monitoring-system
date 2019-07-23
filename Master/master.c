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

LinkList *link_client;

void *continue_heartbeat(void *a) {
    printf("child pthread start\n");
    while (1) {
        printf("online client number : %d\n", link_client->length);
        for (ListNode *q = link_client->head.next, *p = &(link_client->head); q;) {
            fflush(stdout);
            //printf("p -> %s : %d \n", inet_ntoa(p->data.sin_addr), ntohs(p->data.sin_port));
            //printf("q -> %s : %d \n", inet_ntoa(q->data.sin_addr), ntohs(q->data.sin_port));
            if (heartbeat(ntohs(q->data.sin_port), inet_ntoa(q->data.sin_addr)) == 0) {
                printf("%s : %d online !\n", inet_ntoa(q->data.sin_addr), ntohs(q->data.sin_port));
                p = p->next;
                q = q->next;
            } else {
                printf("%s : %d deleting...\n", inet_ntoa(q->data.sin_addr), ntohs(q->data.sin_port));
                p->next = q->next;
                clear_listnode(q);
                q = p->next;
                link_client->length--;
            }
            sleep(1);
        }
        printf("End of traversal\n");
        sleep(5);
    }
}

int main() {
    pthread_t connect_client;
    link_client = init_linklist();
    if (link_client == NULL) {
        exit(1);
    }
    char master_port[10];
    char client_port[10];
    char from[20];
    char to[20];
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

    printf("from %s to %s\n", from, to);
    int ip_num = htonl(inet_addr(to)) - htonl(inet_addr(from)) + 1;
    for (int i = 0; i < ip_num; i++) {
        client.sin_family = AF_INET;
	    client.sin_port = htons(atoi(client_port));
	    client.sin_addr.s_addr = ntohl(htonl(inet_addr(from)) + i);
        if (insert(link_client, i, client) > 0) {
            printf("insert %s to LinkList success\n", inet_ntoa((client.sin_addr)));
        } else {
            printf("insert %s to LinkList failed\n", inet_ntoa((client.sin_addr)));
            close(listen_socket);
            close(epollfd);
            exit(EXIT_FAILURE);
        }
    }
    if (pthread_create(&connect_client, NULL , continue_heartbeat, NULL)) {
        perror("pthread_create");
        close(listen_socket);
        close(epollfd);
        exit(EXIT_FAILURE);
    }
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
	        client.sin_port = htons(atoi(client_port));
            if (insert(link_client, link_client->length, client) > 0) {
                printf("insert %s to LinkList success\n", inet_ntoa((client.sin_addr)));
            } else {
                printf("insert %s to LinkList failed\n", inet_ntoa((client.sin_addr)));
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
