#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include "locker.h"
#include "socket.h"

int MAX_CLIENTS;
struct Locker *lockers;
int server_socket;
struct clientInfo *clients;
int client_count = 0;
int *failed_attempts;


void saveDB(int locker_id);
void saveLogger(const char* message);

void add_client(int socket, int locker_id){
    struct clientInfo *newClients = (struct clientInfo *)malloc(sizeof(struct clientInfo) * (client_count + 1));
    if (newClients == NULL){
        perror("memory allocate error");
        saveLogger("memory allocate error");
        return;
    }
    if (clients != NULL){
        memcpy(newClients, clients, sizeof(struct clientInfo) * client_count);
        free(clients);
    }
    clients = newClients;
    clients[client_count].socket = socket;
    clients[client_count].locker_id = locker_id;
    clients[client_count].block_time = 0;
    client_count++;

    char logMessage[BUFFER_SIZE];
    sprintf(logMessage, "current client_count : %d", client_count);
    saveLogger("new client socker add\n");
    saveLogger(logMessage);
}

void remove_client(int socket){
    for (int i = 0; i < client_count; i++){
        if (clients[i].socket == socket){
            int locker_id = clients[i].locker_id;
            lockers[locker_id].draft = 0;
            saveDB(locker_id);

            for (int j = 0; j < client_count - 1; j++){
                clients[j] = clients[j + 1];
            }
            client_count--;
            struct clientInfo *newClients = (struct clientInfo *)malloc(sizeof(struct clientInfo) * (client_count + 1));
            if (newClients == NULL){
                perror("memory allocate error");
                saveLogger("memory allocate error");
                return;
            }
            if (client_count > 0){
                memcpy(newClients, clients, sizeof(struct clientInfo)* client_count);
            }
            free(clients);
            clients = newClients;
            break;
        }
    }
}


void handle_exit(int sig) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket == -1 && lockers[clients[i].locker_id].draft == 1) {
            lockers[clients[i].locker_id].draft = 0;
            saveDB(clients[i].locker_id);
        }
    }
    close(server_socket);
    unlink(SERVER_LOCK_FILE);
    printf("server shutdown..\n");
    exit(0);
}

void signal_handler(){
    struct sigaction signal;
    signal.sa_handler = handle_exit;
    sigemptyset(&signal.sa_mask);
    signal.sa_flags = 0;
    if (sigaction(SIGINT, &signal, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}


void saveDB(int locker_id) {
    FILE *db = fopen(DATABASE, "rb+");
    if (db == NULL) {
        db = fopen(DATABASE, "wb");
        if (db == NULL) {
            perror("cannot access to db file");
            printf("new db file created\n");
            return;
        }
    }
    fseek(db, locker_id * sizeof(struct Locker), SEEK_SET);
    fwrite(&lockers[locker_id], sizeof(struct Locker), 1, db);
    fclose(db);
}

void initialize_lockers() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        lockers[i].locker_id = i;
        lockers[i].in_use = 0;
        lockers[i].draft = 0;
        strcpy(lockers[i].password, "");
        strcpy(lockers[i].content, "");
        lockers[i].time = 0;
        lockers[i].duration = 0;
    }
    FILE *db = fopen(DATABASE, "wb");
    if (db == NULL) {
        perror("cannot create db file");
        printf("new db file created\n");
        return;
    }
    fwrite(lockers, sizeof(struct Locker), MAX_CLIENTS, db);
    fclose(db);
}


void loadDB() {
    FILE *db = fopen(DATABASE, "rb");
    if (db == NULL) {
        perror("cannot access to db file");
        printf("new db file created\n");
        initialize_lockers();
        return;
    }
    fread(lockers, sizeof(struct Locker), MAX_CLIENTS, db);
    fclose(db);
}

void loadDBbyId(int locker_id) {
    FILE *db = fopen(DATABASE, "rb");
    if (db == NULL) {
        perror("cannot access to db file");
        return;
    }
    fseek(db, locker_id * sizeof(struct Locker), SEEK_SET);
    fread(&lockers[locker_id], sizeof(struct Locker), 1, db);
    fclose(db);
}


void updateLocker(int locker_id) {
    loadDB();
    saveDB(locker_id);
}


int checkPassword(int num, const char *password) {
    loadDBbyId(num);
    if (strcmp(password, lockers[num].password) == 0) {
        return 1;
    } else {
        return 0;
    }
}


void saveLogger(const char *message) {
    FILE *Logger = fopen(LoggerFile, "a");
    if (Logger == NULL) {
        perror("Logger file not exist");
        return;
    } else {
        time_t curtime = time(NULL);
        struct tm tm = *localtime(&curtime);
        fprintf(Logger, "%ld %s\n", curtime, message);
        fclose(Logger);
    }
}

// 0 : reg_time, 1 : end_time, 2: reg + end time
void calculate_remaining_time(struct Locker *locker, char *buffer, int type) {
    time_t current_time = time(NULL);
    int remaining_time = locker->duration - difftime(current_time, locker->time);
    if (remaining_time < 0) {
        remaining_time = 0;
    }
    time_t reg_time = locker->time;
    time_t end_time = locker->time + locker->duration;

    char logBuffer[BUFFER_SIZE];
    sprintf(logBuffer, "regtime : %ld, end_time : %ld", reg_time, end_time);
    saveLogger(logBuffer);

    struct tm reg_time_tm;
    struct tm end_time_tm;

    localtime_r(&reg_time, &reg_time_tm);
    localtime_r(&end_time, &end_time_tm);

    sprintf(logBuffer, "regtime_tm : %ld, endtime_tm : %ld", (long)&reg_time_tm, (long)&end_time_tm);
    saveLogger(logBuffer);

    char reg_time_str[BUFFER_SIZE];
    char end_time_str[BUFFER_SIZE];

    switch(type) {
        case 0:
            strftime(buffer, BUFFER_SIZE, "%Y-%m-%d %H:%M:%S (KST)", &reg_time_tm);
            break;
        case 1:
            strftime(buffer, BUFFER_SIZE, "%Y-%m-%d %H:%M:%S (KST)", &end_time_tm);
            break;
        case 2:
            strftime(reg_time_str, BUFFER_SIZE, "%Y-%m-%d %H:%M:%S (KST)", &reg_time_tm);
            strftime(end_time_str, BUFFER_SIZE, "%Y-%m-%d %H:%M:%S (KST)", &end_time_tm);
            snprintf(buffer, BUFFER_SIZE, "Registered at: %s | Expires at: %s", reg_time_str, end_time_str);
            break;
        default:
            snprintf(buffer, BUFFER_SIZE, "Invalid time type");
            break;
    }
}


void handle_search(int client_socket) {
    saveLogger("handle_search entrance\n");
    char buffer[BUFFER_SIZE];
    int read_size;

    struct clientInfo* client = NULL;
    for (int i = 0; i < client_count; i++){
        if (clients[i].socket == client_socket){
            client = &clients[i];
            break;
        }
    }

    if (client == NULL){
        saveLogger("client is null\n");
        return;
    }

    if (time(NULL) < client->block_time) {
        send(client_socket, "Access temporarily blocked due to multiple incorrect password attempts.\n", 78, 0);
        saveLogger("Access temporarily blocked due to multiple incorrect password attempts.\n");
        close(client_socket);
        return;
    }

    FILE *db = fopen(DATABASE, "r");
    if (db == NULL) {
        perror("cannot access to DB");
        saveLogger("cannot access to DB");
        return;
    }

    fread(lockers, sizeof(struct Locker), MAX_CLIENTS, db);
    fclose(db);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        snprintf(buffer, sizeof(buffer), "Locker No: %d | Available: %s\n", i,
                 (lockers[i].in_use == 0 && lockers[i].draft == 0) ? "Yes" : "No");
        send(client_socket, buffer, strlen(buffer), 0);
    }
    strcpy(buffer, "end_of_list\n");
    saveLogger("end of list\n");

    send(client_socket, buffer, strlen(buffer), 0);

    while (1){
        int locker_num;
        int received = recv(client_socket, &locker_num, sizeof(locker_num), 0);
        if (received <= 0) {
            saveLogger("locker_num received may be null...\n");
            break;
        }

        if (locker_num < 0 || locker_num >= MAX_CLIENTS) {
            saveLogger("wrong locker number.. please check again!\n");
            strcpy(buffer, "wrong locker number.. please check again!\n");
            send(client_socket, buffer, strlen(buffer), 0);
        } else {
            loadDBbyId(locker_num);

            if (lockers[locker_num].in_use == 0) {
                saveLogger("locker is empty\n");
                snprintf(buffer, sizeof(buffer), "locker %d is empty\n", locker_num);
                send(client_socket, buffer, strlen(buffer), 0);
            } else {
                char reg_time_buffer[BUFFER_SIZE], dual_time_buffer[BUFFER_SIZE];
                calculate_remaining_time(&lockers[locker_num], reg_time_buffer, 0);
                snprintf(buffer, sizeof(buffer), "locker number: %d | availability: %s | contents: %s \n registered at : %s",
                         locker_num, (lockers[locker_num].in_use == 0) || (lockers[locker_num].draft == 0) ? "True":"False", "secured data", reg_time_buffer);
                send(client_socket, buffer, strlen(buffer), 0);

                char password[MAX_PASSWORD_SIZE];
                int password_received = recv(client_socket, password, sizeof(password), 0);
                if (password_received > 0) {
                    password[password_received] = '\0';
                    if (checkPassword(locker_num, password)) {
                        failed_attempts[locker_num] = 0;
                        calculate_remaining_time(&lockers[locker_num], dual_time_buffer, 2);
                        snprintf(buffer, sizeof(buffer), "locker number: %d | availability: %s | contents: %s\n %s",
                                 locker_num, (lockers[locker_num].in_use == 0) || (lockers[locker_num].draft == 0) ? "True":"False",
                                 lockers[locker_num].content, dual_time_buffer);
                    } else {
                        strcpy(buffer, "wrong password!\n");
                        failed_attempts[locker_num]++;
                        if (failed_attempts[locker_num] >= MAX_ATTEMPT){
                            client->block_time = time(NULL) + BLOCK_TIME;
                            send(client_socket, "too many incorrect password attemption", strlen(buffer), 0);
                            close(client_socket);
                            return;
                        }
                    }
                    send(client_socket, buffer, strlen(buffer), 0);
                }
            }
        }
    }
}

void handle_checkout(int client_socket) {
    saveLogger("handle_checkout entrance\n");
    char buffer[BUFFER_SIZE];
    int read_size;
    int locker_id = -1;

    while ((read_size = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        locker_id = atoi(buffer);

        char log_message[read_size];
        sprintf(log_message,"check out locker number : %d", locker_id);
        saveLogger(log_message);

        int fd = open(DATABASE, O_RDWR);
        if (fd == -1) {
            perror("DATABASE access failed");
            saveLogger("DATABASE access failed");
            close(client_socket);
            return;
        }

        if (locker_id < 0 || locker_id > MAX_CLIENTS) {
            char *error_message = "wrong locker_id received...";
            perror(error_message);
            saveLogger(error_message);
            send(client_socket, error_message, strlen(error_message), 0);
            break;
        }

        loadDBbyId(locker_id);

        while (1) {
            if (lockers[locker_id].in_use == 0 && lockers[locker_id].draft == 0){
                char *error_message = "Locker is already empty.";
                perror(error_message);
                saveLogger(error_message);
                send(client_socket, error_message, strlen(error_message), 0);
                break;
            } else if (lockers[locker_id].in_use == 0 && lockers[locker_id].draft == 1)
            {
                char *error_message = "Locker is on reservation";
                perror(error_message);
                saveLogger(error_message);
                send(client_socket, error_message, strlen(error_message), 0);
                break;
            }

            send(client_socket, "", BUFFER_SIZE, 0);

            while(1){
                int clientRecv;
                if ((clientRecv = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
                    buffer[clientRecv] = '\0';
                    char password[MAX_PASSWORD_SIZE];
                    sscanf(buffer, "%s", password);
                    if (strstr(password, lockers[locker_id].password) != NULL) {
                        failed_attempts[locker_id] = 0;
                        char *message = "password correct. Really want to checkout? (Y/N) : \n";
                        send(client_socket, message, strlen(message), 0);

                        clientRecv = recv(client_socket, buffer, BUFFER_SIZE, 0);
                        if (clientRecv > 0) {
                            buffer[clientRecv] = '\0';
                            if (strcmp(buffer, "Y") == 0 || strcmp(buffer, "y") == 0) {
                                lockers[locker_id].locker_id = locker_id;
                                lockers[locker_id].in_use = 0;
                                lockers[locker_id].draft = 0;
                                strcpy(lockers[locker_id].password, "");
                                strcpy(lockers[locker_id].content, "");
                                lockers[locker_id].time = 0;
                                lockers[locker_id].duration = 0;
                                saveDB(locker_id);

                                char logMessage[BUFFER_SIZE];
                                sprintf(logMessage, "Locker %d has been checked out and is now empty.",
                                        locker_id);
                                saveLogger(logMessage);

                                message = "Locker content deleted successfully.\n";
                                send(client_socket, message, strlen(message), 0);
                            } else {
                                char *message = "Checkout canceled.\n";
                                send(client_socket, message, strlen(message), 0);
                            }
                        } else {
                            perror("failed to fetch confirmation info");
                            saveLogger("failed to fetch confirmation info");
                        }
                    } else {
                        failed_attempts[locker_id]++;
                        if (failed_attempts[locker_id] >= MAX_ATTEMPT){
                            clients->block_time = time(NULL) + BLOCK_TIME;
                            send(client_socket, "too many incorrect password attemption", strlen(buffer), 0);
                            close(client_socket);
                            return;
                        } else {
                            char *message = "wrong password!";
                            perror(message);
                            saveLogger(message);
                            send(client_socket, message, strlen(message), 0);
                        }
                    }
                } else {
                    saveLogger("failed to fetch password info");
                }
            }
        }
    }
}

int handle_reservation(int client_socket) {
    saveLogger("handle_reservation entrance\n");
    char buffer[BUFFER_SIZE];
    int read_size;
    int locker_id = -1;

    while ((read_size = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        locker_id = atoi(buffer);

        int fd = open(DATABASE, O_RDWR);
        if (fd == -1) {
            perror("Database access failed");
            saveLogger("Database access failed");
            close(client_socket);
            return -1;
        }

        /**
         * file lock initialize
         */
        struct flock lock;
        lock.l_type = F_WRLCK; // set write lock
        lock.l_whence = SEEK_SET;
        lock.l_start = locker_id * sizeof(struct Locker);
        lock.l_len = sizeof(struct Locker);
        lock.l_pid = getpid();

        if (fcntl(fd, F_SETLK, &lock) == -1) {
            if (errno == EAGAIN) {
                char* message = "This locker is on reservation by another user.";
                saveLogger(message);
                send(client_socket, message, strlen(message), 0);
            } else {
                saveLogger("file lock fail");
            }
            close(fd);
            continue;
        }

        if (locker_id < 0 || locker_id >= MAX_CLIENTS) {
            char *message = "Invalid locker ID.\n";
            saveLogger(message);
            send(client_socket, message, strlen(message), 0);
            lock.l_type = F_UNLCK;
            fcntl(fd, F_SETLK, &lock);
            close(fd);
            continue;
        }

        loadDBbyId(locker_id);

        if (lockers[locker_id].in_use == 0 && lockers[locker_id].draft == 0) {
            saveLogger("locker is not in use and not in draft status\n");
            char *message = "Locker ID is available.\n";
            saveLogger(message);
            send(client_socket, message, strlen(message), 0);

            lockers[locker_id].draft = 1;
            saveDB(locker_id);

            while (1) {
                read_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
                // reset draft if connection failed
                if(read_size <= 0){
                    lockers[locker_id].draft = 0;
                    saveDB(locker_id);
                    return -1;
                }

                if (read_size > 0) {
                    buffer[read_size] = '\0';
                    char password[MAX_PASSWORD_SIZE];
                    sscanf(buffer, "%s", password);

                    read_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
                    if (read_size > 0) {
                        buffer[read_size] = '\0';
                        char confirm_password[MAX_PASSWORD_SIZE];
                        sscanf(buffer, "%s", confirm_password);

                        if (strcmp(password, confirm_password) != 0) {
                            failed_attempts[locker_id]++;
                            if (failed_attempts[locker_id] >= MAX_ATTEMPT){
                                clients->block_time = time(NULL) + BLOCK_TIME;
                                send(client_socket, "too many incorrect password attemption", strlen(buffer), 0);
                                close(client_socket);
                                return 1;
                            } else {
                                char *message = "password and confirm password not equal";
                                saveLogger(message);
                                send(client_socket, message, strlen(message), 0);
                                continue;
                            }
                        } else {
                            char *message1 = "password equal";
                            saveLogger(message1);
                            send(client_socket, message1, strlen(message1), 0);

                            read_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
                            if (read_size > 0) {
                                buffer[read_size] = '\0';
                                char content[BUFFER_SIZE];
                                sscanf(buffer, "%s", content);

                                saveLogger("content received from client server");

                                // 사용자 이용 시간 지정
                                read_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
                                if (read_size > 0){
                                    buffer[read_size] = '\0';
                                    int duration = atoi(buffer);

                                    lockers[locker_id].in_use = 1;
                                    strcpy(lockers[locker_id].password, password);
                                    strcpy(lockers[locker_id].content, content);
                                    lockers[locker_id].time = time(NULL);
                                    lockers[locker_id].duration = duration * 3600;
                                    saveDB(locker_id);

                                    saveLogger("file updated to DB");

                                    char *message = "Locker content saved.\n";
                                    saveLogger(message);
                                    send(client_socket, message, strlen(message), 0);
                                    buffer[read_size] = '\0';


                                    for (int i = 0; i < client_count; i++) {
                                        if (clients[i].socket == client_socket) {
                                            clients[i].locker_id = locker_id;
                                            break;
                                        }
                                    }

                                    break;

                                }
                            }
                        }
                    }
                }
            }
        } else {
            char *message = "Locker ID is already in use.\n";
            saveLogger(message);
            send(client_socket, message, strlen(message), 0);
            lock.l_type = F_UNLCK;
            if (fcntl(fd, F_SETLK, &lock) == -1) {
                saveLogger("file unlock failed");
            }
            close(fd);
            continue;
        }

        lock.l_type = F_UNLCK;
        if (fcntl(fd, F_SETLK, &lock) == -1) {
            saveLogger("file unlock fail");
        }
        close(fd);
    }

    if(locker_id >= 0){
        lockers[locker_id].draft = 0;
        saveDB(locker_id);
    }

    close(client_socket);
    return locker_id;
}

void handle_time(int client_socket){
    char buffer[BUFFER_SIZE];
    int read_size;
    int locker_id = -1;

    while ((read_size = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        locker_id = atoi(buffer);

        char log_message[read_size];
        sprintf(log_message,"received locker number : %d", locker_id);
        saveLogger(log_message);

        int fd = open(DATABASE, O_RDWR);
        if (fd == -1) {
            perror("DATABASE access failed");
            saveLogger("DATABASE access failed");
            close(client_socket);
            return;
        }

        if (locker_id < 0 || locker_id > MAX_CLIENTS) {
            char *error_message = "wrong locker_id received...";
            perror(error_message);
            saveLogger(error_message);
            send(client_socket, error_message, strlen(error_message), 0);
            break;
        }

        loadDBbyId(locker_id);

        while (1) {
            if (lockers[locker_id].in_use == 0){
                char *error_message = "Locker is already empty.";
                perror(error_message);
                saveLogger(error_message);
                send(client_socket, error_message, strlen(error_message), 0);
                break;
            } else {
                char *response = "enter locker's password : ";
                send(client_socket, response, strlen(response), 0);
            }

            int clientRecv;

            if ((clientRecv = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
                buffer[clientRecv] = '\0';
                char password[MAX_PASSWORD_SIZE];
                sscanf(buffer, "%s", password);
                if (strstr(password, lockers[locker_id].password) != NULL) {
                    char *message = "password correct.\n";
                    send(client_socket, message, strlen(message), 0);

                    loadDBbyId(locker_id);

                    char dual_time_buffer[BUFFER_SIZE];
                    calculate_remaining_time(&lockers[locker_id], dual_time_buffer, 2);
                    snprintf(buffer, sizeof(buffer),
                             "\n------------| locker info |-------------\n"
                             "locker number: %d | contents: %s\n %s\n",
                             locker_id, lockers[locker_id].content, dual_time_buffer);
                    send(client_socket, buffer, strlen(buffer), 0);

                    clientRecv = recv(client_socket, buffer, BUFFER_SIZE, 0);
                    if (clientRecv > 0) {
                        buffer[clientRecv] = '\0';
                        int input_time = atoi(buffer);

                        char logMessage[BUFFER_SIZE];
                        snprintf(logMessage, BUFFER_SIZE, "Received time from client: %d", input_time);
                        saveLogger(logMessage);

                        lockers[locker_id].duration += input_time * 3600;
                        saveDB(locker_id);

                        char logMessage2[BUFFER_SIZE];
                        snprintf(logMessage2, BUFFER_SIZE, "Updated duration for locker %d: %d seconds", locker_id, lockers[locker_id].duration);
                        saveLogger(logMessage2);

                        char* success_message = "success";
                        send(client_socket, success_message, strlen(success_message), 0);
                    } else {
                        perror("failed to fetch client info");
                        saveLogger("failed to fetch client info");
                    }
                } else {
                    char *message = "wrong password!";
                    perror(message);
                    saveLogger(message);
                    send(client_socket, message, strlen(message), 0);
                }
            } else {
                saveLogger("failed to fetch password info");
            }
        }
    }
}

void cleanup_client(int client_socket){
    saveLogger("cleanup_client entrance\n");
    for (int i = 0; i < client_count; i++){
        if (clients[i].socket == client_socket){
            int locker_id = clients[i].locker_id;
            lockers[locker_id].draft = 0;
            saveDB(locker_id);
            close(client_socket);
            remove_client(client_socket);
        }
    }
}

void handle_client(int client_socket) {
    saveLogger("handle_client entrance\n");
    int menu_choice;
    char buffer[BUFFER_SIZE];
    int read_size;

    add_client(client_socket, -1);

    while ((read_size = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';

        for (int i = 0; i < client_count; i++) {
            if (clients[i].socket == client_socket) {
                time_t current_time = time(NULL);
                if (current_time < clients[i].block_time) {
                    int remaining_time = (int)(clients[i].block_time - current_time);
                    snprintf(buffer, sizeof(buffer), "Access temporarily blocked due to multiple incorrect password attempts. Please try again in %d seconds.\n", remaining_time);
                    send(client_socket, buffer, strlen(buffer), 0);
                    close(client_socket);
                    return;
                }
                break;
            }
        }

        memcpy(&menu_choice, buffer, sizeof(menu_choice));
        int locker_id = -1;

        switch (menu_choice) {
            case 1:
                handle_search(client_socket);
                break;
            case 2:
                locker_id = handle_reservation(client_socket);
                if (locker_id >= 0){
                    for (int i = 0; i < client_count; i++) {
                        if (clients[i].socket == client_socket) {
                            clients[i].locker_id = locker_id;
                            break;
                        }
                    }
                }
                break;
            case 3:
                handle_checkout(client_socket);
                break;
            case 4:
                handle_time(client_socket);
                break;
            case 5:

                break;
            default:
                printf("wrong choice.. please check again\n");
                break;
        }
    }

    if (read_size == 0) {
        printf("client disconnected\n");
        cleanup_client(client_socket);
        saveLogger("client removed");
    } else if (read_size == -1) {
        perror("recv failed");
        saveLogger("recv failed");
    }

    close(client_socket);
}


void port_file(int port) {
    FILE* file = fopen(PORT_FILE, "w");
    if (file == NULL) {
        char* error_message = "error while opening port config file";
        perror(error_message);
        saveLogger(error_message);
        return;
    }
    fprintf(file, "%d\n", port);
    fclose(file);
}

int create_server_lock(const char* lock_file){
    saveLogger("create_server_lock entrance\n");
    int fd = open(lock_file, O_WRONLY | O_CREAT, 0666);
    if (fd == -1) {
        perror("Failed to open lock file");
        return 1;
    }
    struct flock server_lock;
    server_lock.l_type = F_WRLCK;
    server_lock.l_start = 0;
    server_lock.l_whence = SEEK_SET;
    server_lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &server_lock) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            saveLogger("Other process is using server...\n");
            return -1;
        }
        perror("Failed to lock file");
        close(fd);
        return 1;
    }

    return fd;

}

int main(int argc, char* argv[]) {

    if (argc != 2){
        fprintf(stderr, "Usage: %s <number_of_lockers>\n", argv[0]);
        return 1;
    }

    MAX_CLIENTS = atoi(argv[1]);
    if (MAX_CLIENTS <= 0){
        fprintf(stderr, "invalid number of locker, please type bigger than 0 again!\n");
        return 1;
    }

    int lock_fd = create_server_lock(SERVER_LOCK_FILE);
    if (lock_fd == -1) {
        fprintf(stderr, "Server already open...\n");
        return 1;
    } else if (lock_fd == 1) {
        fprintf(stderr, "Failed to create or lock the lock file...\n");
        return 1;
    }


    lockers = malloc( sizeof(struct Locker) * MAX_CLIENTS);
    if (lockers == NULL){
        saveLogger("locker is null [memory error]");
        return 1;
    }

    signal_handler();
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int port = PORT;
    int port_increment = 1;
    int bind_attempts = 5;

    printf("-----------------------------------------------------\n");
    printf("server binding port... in progress...\n");

    while (bind_attempts > 0) {
        if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Could not create socket");
            saveLogger("could not create socket");
            return -1;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Bind failed");
            saveLogger("Bind failed");

            printf("bind port [ %d ] failed ...\n", port);
            printf("retry with 1 increment... \n");

            port += port_increment;
            bind_attempts--;

            close(server_socket);
            if (bind_attempts == 0) {
                return -1;
            }

            continue;
        }

        char logMessage[BUFFER_SIZE];
        snprintf(logMessage, BUFFER_SIZE, "Server bound to port %d", port);
        saveLogger(logMessage);
        port_file(port);
        break;
    }

    printf("server bind port successfully with port [ %d ]\n", port);

    listen(server_socket, MAX_CLIENTS);

    loadDB();

    printf("server started.. waiting for any connections\n");

    while ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len))) {
        printf("Connection accepted\n");

        pid_t pid = fork();
        if (pid < 0){
            saveLogger("fork fail");
            continue;
        } else if (pid == 0)
        {
            close(server_socket);
            handle_client(client_socket);
            exit(0);
        } else {
            close(client_socket);
        }
    }

    if (client_socket < 0) {
        perror("Accept failed");
        return -1;
    }

    close(server_socket);
    flock(lock_fd, LOCK_UN);
    close(lock_fd);
    unlink(SERVER_LOCK_FILE);
    return 0;
}
