/*************************************************************************
	> File Name: common.c
	> Author: sudingquan
	> Mail: 1151015256@qq.com
	> Created Time: 六  7/20 14:33:10 2019
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "common.h"

ListNode *init_listnode(struct sockaddr_in val) {
    ListNode *p = (ListNode *)malloc(sizeof(ListNode));
    p->data = val;
    p->next = NULL;
    return p;
}

LinkList *init_linklist() {
    LinkList *l = (LinkList *)malloc(sizeof(LinkList));
    l->head.next = NULL;
    l->length = 0;
    return l;
}

void clear_listnode(ListNode *node) {
    if (node == NULL) return ;
    free(node);
    return ;
}

void clear_linklist(LinkList *l) {
    if (l == NULL) return ;
    ListNode *p = l->head.next, *q;
    while (p) {
        q = p->next;
        clear_listnode(p);
        p = q;
    }
    free(l);
    return ;
}

int insert(LinkList *l, int ind, struct sockaddr_in val) {
    if (l == NULL) return 0;
    if (ind < 0 || ind > l->length) return 0;
    ListNode *p = &(l->head), *node = init_listnode(val);
    while (ind--) {
        p = p->next;
    }
    node->next = p->next;
    p->next = node;
    l->length += 1;
    return 1;
}

int erase(LinkList *l, int ind) {
    if (l == NULL) return 0;
    if (ind < 0 || ind >= l->length) return 0;
    ListNode *p = &(l->head), *q;
    while (ind--) {
        p = p->next;
    }
    q = p->next->next;
    clear_listnode(p->next);
    p->next = q;
    l->length -= 1;
    return 1;
}

int socket_connect(int port, char *host) {
	int sockfd;
	struct sockaddr_in dest_addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket() error");
		return -1;
	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	dest_addr.sin_addr.s_addr = inet_addr(host);

	//printf("Connetion TO %s:%d\n",host,port);
	//printf(stdout);
	if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
		//perror("connect() error");
		//printf("connect() error : %s!\n", stderror(errno));
		return -1;
	}
	return sockfd;

}

int create_listen_socket(int port) {
    int listen_socket;
    if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() error");
        return -1;
    }
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listen_socket, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind");
        return -1;
    }
    if (listen(listen_socket, 5) < 0) {
        perror("listen");
    }
    return listen_socket;
}

int wait_client(int listen_socket) {
    struct sockaddr_in client_addr;
    printf("wait for client...\n");
    unsigned int addrlen = sizeof(client_addr);
    int client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &addrlen);
    if (client_socket < 0) {
        perror("accept");
        return -1;
    }
    printf("connection to the client %s successful\n", inet_ntoa(client_addr.sin_addr));
    return client_socket;
}

int heartbeat(int port, char *host) {
	int sockfd;
    int retval;
    struct timeval timeout;
	struct sockaddr_in dest_addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket() error");
		exit(1);
	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	dest_addr.sin_addr.s_addr = inet_addr(host);

	//printf("Connetion TO %s:%d\n",host,port);
	//fflush(stdout);
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sockfd, &wfds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 300000;
    unsigned long imode = 1;

    int error = -1;
    int len = sizeof(error);
    int ret = -1;

    ioctl(sockfd, FIONBIO, &imode);
    int n = connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (n < 0 && errno == EINPROGRESS) {
        //printf("in if....\n");
        retval = select(sockfd + 1, NULL, &wfds, NULL, &timeout);
        if (retval > 0) {
            //printf("in if if....\n");
            if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len) < 0) {
                perror("getsockopt");
                ret = -1;
            }
            if (error == 0) {
                ret = 0;
            } else {
                ret = -1;
            }
        }
    }
    close(sockfd);
    return ret;
}

int get_conf(char *file, char *key, char *val) {
    FILE *fp = NULL;
    fp = fopen(file, "r");
    if (fp == NULL) {
        return -1;
    }
    char str[100];
    while (!feof(fp)) {
        char *p = str;
        fgets(str, 1024, fp);
        p = strstr(str, key);
        if(p == NULL) {
            continue;
        }
        if (str[strlen(key)] != '=') {
            printf("no =\n");
            fp = NULL;
            fclose(fp);
            return -1;
        }
        p = strstr(str, "=");
        p += 1;
        strcpy(val, p);
        val[strlen(val) - 1] = 0;
        break;
    }
    fp = NULL;
    fclose(fp);
    return 0;
}
