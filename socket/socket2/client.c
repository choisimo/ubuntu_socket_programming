#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_MSG_SIZE 200

int main() {
	
	int client_sock;
	struct sockaddr_in serv_addr;
	char message[MAX_MSG_SIZE];
	
	client_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (client_sock == -1){
		printf("socket error!");
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(4744);

	int server_connection =
		(connect(client_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)));

	if ((server_connection) == -1)
	{
		printf("server connection failed");
		return -1;
	}
	
	while(1){
		printf("write a message : ");

		fgets(message, MAX_MSG_SIZE, stdin);
		write(client_sock, message, strlen(message));
		sleep(1);
	}	

	close(client_sock);
	return 0;
}

