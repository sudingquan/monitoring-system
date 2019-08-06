/*************************************************************************
	> File Name: common.h
	> Author: sudingquan
	> Mail: 1151015256@qq.com
	> Created Time: 六  7/20 14:37:52 2019
 ************************************************************************/

#ifndef _COMMON_H
#define _COMMON_H
// LinkList
typedef struct ListNode {
    struct sockaddr_in data;
    struct ListNode *next;
} ListNode;

typedef struct LinkList {
    ListNode head;
    int length;
    int id;
} LinkList;

ListNode *init_listnode(struct sockaddr_in val);

LinkList **init_linklist(int ins);

int min_length(LinkList **l, int ins);

void clear_listnode(ListNode *node);

void clear_linklist(LinkList *l);

int insert(LinkList *l, int ind, struct sockaddr_in val);

int already_in_linklist(LinkList **l, int ins, struct sockaddr_in client, int *in);

int erase(LinkList *l, int ind);
// LinkList end

// socket
int socket_connect(int port, char *host);

void set_reuseaddr(int sockfd, int optval);

int create_listen_socket(int port);

int wait_client(int listen_socket);

int heartbeat(int port, char *host);
// socket end

// conf 
int get_conf(char *file, char *key, char *val);
// conf end

// log
int my_log(char *filename, const char *format, ...) {
// log end

#endif
