//
// Created by csimo on 5/6/2024.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "socket_struct.h"

#define PORT 3000
#define MAX 1024

int main(){
    int client_sock;
    struct sockaddr_in server_addr;
    char buffer[MAX];
    char* first_message = "client connected";

    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == -1){
        printf("소켓 생성 실패! \n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = 8080;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(server_addr.sin_zero, 0, size_of(server_addr.sin_zero));

}