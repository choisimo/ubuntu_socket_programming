#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <arpa/inet.h>
#include "locker.h"

int sock;

void print_menu(){
    printf(" please choose one menu following \n");
    printf(" 1. search \n");
    printf(" 2. reservation \n");
    printf(" 3. checkout \n");
    printf(" 4. handle time \n");
    printf("\n");
}

int get_menu_num(int menu_num){
    if(menu_num == 1){
        printf("search page\n");
        printf("enter locker number your own\n");
        int locker_num = -1;
        scanf("%d", &locker_num);
        return locker_num;
    } else if (menu_num == 2) {
        printf("reservation page\n");
        return 2;
    } else if (menu_num == 3) {
        printf("check out page\n");
        return 3;
    } else if (menu_num == 4) {
        printf("handle time page\n");
        return 4;
    } else if (menu_num == 5) {
        return 5;
    } else {
        return 0;
    }
}

void saveLogger(const char *message) {
    FILE *Logger = fopen(CLoggerFile, "a");
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

void handle_search(int sock) {
    char buffer[BUFFER_SIZE];

    while (1) {
        int received = recv(sock, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            break;
        }
        buffer[received] = '\0';
        printf("%s", buffer);
        if (strstr(buffer, "end_of_list") != NULL){
            break;
        }
    }

    while(1){
        printf("want to see more specific info? (Y | N) \n");
        char choice;
        scanf(" %c", &choice);

        if (choice == 'Y' || choice == 'y') {
            printf("enter the number of locker : ");
            int locker_num;
            scanf("%d", &locker_num);
            send(sock, &locker_num, sizeof(locker_num), 0);

            int detail_info = recv(sock, buffer, sizeof(buffer), 0);
            if (detail_info > 0) {
                buffer[detail_info] = '\0';
                printf("-------------| normal info |--------------\n");
                printf("%s\n", buffer);

                if (strstr(buffer, "is empty") != NULL) {
                    printf("this is an empty locker...\n");
                    saveLogger("this is an empty locker...\n");
                    continue;
                }
                if (strstr(buffer, "wrong locker number.. please check again!\n") != NULL){
                    printf("wrong locker number input error\n");
                    saveLogger("wrong locker number input error\n");
                    continue;
                }
                if (strstr(buffer, "on reservation") != NULL){
                    continue;
                }

                printf("enter password to see secured content\n");
                printf("------------------------------------------\n");
                printf("password : ");
                char password[MAX_PASSWORD_SIZE];
                scanf("%s", password);
                send(sock, password, strlen(password), 0);

                int content_received = recv(sock, buffer, BUFFER_SIZE, 0);
                if (content_received > 0) {
                    buffer[content_received] = '\0';
                    printf("-------------| detail info |--------------\n");
                    printf("%s\n", buffer);
                    printf("------------------------------------------\n");
                } else {
                    printf("failed to fetch secured content\n");
                    saveLogger("failed to fetch secured content\n");
                }
            } else {
                printf("error to fetch detail info\n");
                saveLogger("error to fetch detail info\n");
            }
        } else {
            printf("search menu off...\n");
            saveLogger("search menu off...\n");
            close(sock);
            execl("./client", "client", (char *)NULL);
            perror("excl failed");
            saveLogger("execl failed");
            exit(0);
            break;
        }
    }
}


int handle_reservation(int sock) {
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    while (1) {
        printf("Enter locker ID: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        send(sock, buffer, strlen(buffer), 0);

        int read_size = recv(sock, message, BUFFER_SIZE, 0);
        if (read_size > 0) {
            message[read_size] = '\0';
            printf("Server: %s\n", message);

            if (strcmp(message, "Locker ID is available.\n") == 0) {
                while (1) {
                    printf("Enter password: ");
                    fgets(buffer, BUFFER_SIZE, stdin);
                    buffer[strcspn(buffer, "\n")] = 0;
                    send(sock, buffer, strlen(buffer), 0);

                    printf("Confirm password: ");
                    fgets(buffer, BUFFER_SIZE, stdin);
                    buffer[strcspn(buffer, "\n")] = 0;
                    send(sock, buffer, strlen(buffer), 0);

                    read_size = recv(sock, message, BUFFER_SIZE, 0);
                    if (read_size > 0) {
                        message[read_size] = '\0';
                        printf("Server: %s\n", message);

                        if (strcmp(message, "password and confirm password not equal") == 0) {
                            continue;
                        } else {
                            printf("\n");
                            printf("Enter content to store in the locker: ");
                            fgets(buffer, BUFFER_SIZE, stdin);
                            buffer[strcspn(buffer, "\n")] = 0;
                            send(sock, buffer, strlen(buffer), 0);

                            printf("Enter duration [hour type] : ");
                            fgets(buffer, BUFFER_SIZE, stdin);
                            buffer[strcspn(buffer, "\n")] = 0;
                            send(sock, buffer, strlen(buffer), 0);

                            read_size = recv(sock, message, BUFFER_SIZE, 0);
                            if (read_size > 0) {
                                printf("Server: %s \n", message);
                                return 0;
                            } else {
                                perror("server error");
                                continue;
                            }
                        }
                    }
                }
            } else if (strcmp(message, "Locker ID is already in use.\n") == 0) {
                printf("Enter locker ID again.\n");
                continue;
            }
        }
    }
}


void handle_checkout(int sock) {
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    while (1) {
        printf("Enter locker ID : ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        send(sock, buffer, strlen(buffer), 0);

        int read_size = recv(sock, message, BUFFER_SIZE, 0);
        if (read_size > 0) {
            message[read_size] = '\0';

            if (strstr(message, "wrong locker_id") != NULL) {
                printf("Server: %s\n", message);
                continue;
            } else if (strstr(message, "Locker is already empty") != NULL){
                printf("Server: %s\n", message);
                continue;
            }

            printf("Enter password: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = 0;
            send(sock, buffer, strlen(buffer), 0);

            read_size = recv(sock, message, BUFFER_SIZE, 0);
            if (read_size > 0) {
                message[read_size] = '\0';
                printf("Server: %s\n", message);

                if (strstr(message, "password correct") != NULL) {
                    char choice;
                    scanf(" %c", &choice);
                    getchar();

                    if (choice == 'Y' || choice == 'y') {
                        send(sock, "Y", 1, 0);
                    } else {
                        send(sock, "N", 1, 0);
                    }

                    read_size = recv(sock, message, BUFFER_SIZE, 0);
                    if (read_size > 0) {
                        message[read_size] = '\0';
                        printf("Server: %s\n", message);
                        break;
                    }
                }
            }
        }
    }
}


void handle_time(int sock) {
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    while (1) {
        printf("Enter locker ID : ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        send(sock, buffer, strlen(buffer), 0);

        int read_size = recv(sock, message, BUFFER_SIZE, 0);
        if (read_size > 0) {
            message[read_size] = '\0';

            if (strstr(message, "wrong locker_id") != NULL) {
                printf("Server: %s\n", message);
                continue;
            } else if (strstr(message, "Locker is already empty") != NULL){
                printf("Server: %s\n", message);
                continue;
            }

            printf("Enter password: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = 0;
            send(sock, buffer, strlen(buffer), 0);

            read_size = recv(sock, message, BUFFER_SIZE, 0);
            if (read_size > 0) {
                message[read_size] = '\0';
                printf("Server: %s\n", message);
                int input_time = -1;

                if (strstr(message, "password correct") != NULL) {
                    int info = recv(sock, message, BUFFER_SIZE, 0);
                    if (info > 0) {
                        message[info] = '\0';
                        printf("Server : %s\n", message);
                    } else {
                        printf("password not valid!\n");
                        continue;
                    }

                    while (1) {
                        printf("enter time[hour] you want (should be bigger than 0) : ");
                        if (scanf("%d", &input_time) != 1 || input_time <= 0) {
                            printf("Invalid input. Please enter a positive integer.\n");
                            while (getchar() != '\n');
                        } else {
                            break;
                        }
                    }

                    snprintf(message, BUFFER_SIZE, "%d", input_time);
                    send(sock, message, strlen(message), 0);
                    int status = recv(sock, message, strlen(message), 0);
                    if (status > 0){
                        message[status] = '\0';
                        if (strstr(message, "success") != NULL){
                            printf("successfully change time\n");
                            close(sock);
                            exit(0);
                        }
                    } else {
                        perror("failed to fetch data");
                        saveLogger("failed to fetch data");
                        continue;
                    }
                }
            }
        }
    }
}

int get_menu_choice() {
    int menu_choice;
    char buffer[BUFFER_SIZE];
    while (1) {
        print_menu();
        printf("choose one menu num... : ");
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            if (sscanf(buffer, "%d", &menu_choice) == 1) {
                if (menu_choice >= 1 && menu_choice <= 5) {
                    break;
                } else {
                    printf("Invalid choice...\n");
                }
            } else {
                printf("Invalid input. Please enter a valid number.\n");
            }
        }
    }
    return menu_choice;
}

int read_port(const char *filename){
    FILE* file = fopen(filename, "r");
    if (file == NULL){
        char* error_message = "failed to read port file";
        perror(error_message);
        saveLogger(error_message);
        return -1;
    }
    int new_port;

    if(fscanf(file, "%d", &new_port) != 1){
        perror("error reading port from file\n");
        fclose(file);
        return -1;
    }
    fclose(file);
    return new_port;
}


void sigint_handler(int sig){
    char* message = "client shutdown";
    send(sock, message, strlen(message), 0);
    close(sock);
    exit(0);
}
    /**
====================================================================================
                               소켓 연결 시작
====================================================================================
 * */
int main() {

    signal(SIGINT, sigint_handler);

    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    int port = read_port(PORT_FILE);
    if (port == -1){
        port = PORT;
        printf("failed to read port file\n");
        return -1;
    }
    // socket create attempt
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error \n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        printf("IPv4 address not accepted \n");
        return -1;
    }

    // connect to server socket
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("error while connecting to server socket \n");
        return -1;
    }

    /**
====================================================================================
                                   소켓 연결 끝
====================================================================================
     * */

        int menu_choice = get_menu_choice();
        send(sock, &menu_choice, sizeof(menu_choice), 0);

        if (menu_choice == 1){
            handle_search(sock);
        } else if (menu_choice == 2){
            handle_reservation(sock);
        } else if (menu_choice == 3){
            handle_checkout(sock);
        } else if (menu_choice == 4){
            handle_time(sock);
        } else if (menu_choice == 5){

        } else {
            printf("invalid choice.. please enter a valid menu\n");
        }

    close(sock);
    return 0;
}