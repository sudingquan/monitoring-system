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
#include "common.h"
#define CONF "master_conf"

LinkList *link_client;

void *continue_heartbeat(void *a) {
    printf("child pthread start\n");
    while (1) {
        printf("LinkList(%d) : \n", link_client->length);
        for (ListNode *q = link_client->head.next, *p = &(link_client->head); q; q = q->next, p = p->next) {
            fflush(stdout);
            if (heartbeat(ntohs(q->data.sin_port), inet_ntoa(q->data.sin_addr)) != 0) {
                printf("%s : %d deleting...\n", inet_ntoa(q->data.sin_addr), ntohs(q->data.sin_port));
                p->next = q->next;
                clear_listnode(q);
                q = p->next;
                continue;
            }
            printf("%s : %d online !\n", inet_ntoa(q->data.sin_addr), ntohs(q->data.sin_port));
            sleep(1);
        }
        printf("NULL\n");
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
    if (get_conf(CONF, "master_port", master_port) < 0) {
        printf("get master_port failed\n");
    }
    if (get_conf(CONF, "client_port", client_port) < 0) {
        printf("get client_port failed\n");
    }
    if (get_conf(CONF, "from", from) < 0) {
        printf("get from failed\n");
    }
    if (get_conf(CONF, "to", to) < 0) {
        printf("get to failed\n");
    }
    int listen_socker = create_listen_socket(atoi(master_port));
    if (listen_socker < 0) {
        printf("create listen socket failed\n");
        exit(1);
    }
    printf("from %s to %s\n", from, to);
    int ip_num = htonl(inet_addr(to)) - htonl(inet_addr(from)) + 1;
    for (int i = 0; i < ip_num; i++) {
        struct sockaddr_in client;
        client.sin_family = AF_INET;
	    client.sin_port = htons(atoi(client_port));
	    client.sin_addr.s_addr = ntohl(htonl(inet_addr(from)) + i);
        if (insert(link_client, i, client) > 0) {
            printf("insert %s to LinkList success\n", inet_ntoa((client.sin_addr)));
        } else {
            printf("insert %s to LinkList failed\n", inet_ntoa((client.sin_addr)));
            exit(1);
        }
    }
    if (pthread_create(&connect_client, NULL , continue_heartbeat, NULL)) {
        perror("pthread_create");
        exit(1);
    }
    while (1) {

    }
    return 0;
}
