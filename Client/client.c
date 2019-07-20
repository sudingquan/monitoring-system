/*************************************************************************
	> File Name: client.c
	> Author: sudingquan
	> Mail: 1151015256@qq.com
	> Created Time: 六  7/20 14:31:28 2019
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#define CONF "client_conf"

int main() {
    char client_port[10];
    char master_port[10];
    char master[20];
    if (get_conf(CONF, "ClientPort", client_port) < 0) {
        printf("get ClientPort failed\n");
    }
    if (get_conf(CONF, "MasterPort", master_port) < 0) {
        printf("get Master failed\n");
    }
    if (get_conf(CONF, "Master", master) < 0) {
        printf("get Master failed\n");
    }
    int listen_socket = create_listen_socket(atoi(client_port));
    if (listen_socket < 0) {
        printf("create listen socket failed\n");
        exit(1);
    }

    return 0;
}
