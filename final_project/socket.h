//
// Created by csimo on 6/7/2024.
//

#ifndef CODE_REPOSITORY_SOCKET_H
#define CODE_REPOSITORY_SOCKET_H
struct clientInfo {
    int socket;
    int locker_id;
    time_t block_time;
};
#endif //CODE_REPOSITORY_SOCKET_H
