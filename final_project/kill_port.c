//
// Created by csimo on 6/7/2024.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "locker.h"

void kill_processes(int port){
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "lsof -t -i:%d", port);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("Failed to run command\n");
        exit(1);
    }

    char line[128];
    while (fgets(line, sizeof(line), fp)!= NULL){
        int pid = atoi(line);
        if (pid > 0){
            snprintf(command, BUFFER_SIZE, "kill -9 %d", pid);
            system(command);
            printf("kill %d port process %d\n", pid, port);
        }
    }

    pclose(fp);
}

int main(){
    kill_processes(PORT);
    return 0;
}