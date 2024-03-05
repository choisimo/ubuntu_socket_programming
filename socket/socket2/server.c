#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>

// 최대 접속 가능 인원 수
#define max_client 10

int client_socket[max_client];
int client_count = 0;

int main(){
	
	int sockfd;
	int sockcln;

	int clnt_addr_size;
	struct sockaddr_in serv_addr, clnt_addr;
	
	char buffer[200];
	int received_len = 0;
	/* TCP IPv4 로소켓 열기
	 * AF_INET : TCP
	 * SOCK_STREAM : STREAM, TCP (소켓 타입)
	 * 0 : TCP 프로토콜 사용명시
	 */
	
	 sockfd = socket(PF_INET, SOCK_STREAM, 0);
	
	
	 /*
	  *  sockaddr_in 구조체 설정
	  * */
	 serv_addr.sin_family = AF_INET;
	 // INADDR_ANY : 현재 서버의 ip 를 의미함 host to network
	 serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	 serv_addr.sin_port = htons(4744);

	if( bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
		printf("socket bind error!");
	}

	// 대기 가능 수 5 
	if (listen(sockfd, 5) == -1){
		printf("listen error");
	}

	while(1){
		
		clnt_addr_size = sizeof(clnt_addr);
		// &clnt_addr : client ip 주소들 
		sockcln = accept(sockfd, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
		// sockcln 담기
		client_socket[client_count++] = sockcln;
		
		while(received_len = read(sockcln, buffer, sizeof(buffer)-1) > 0)
		{
			printf("message received: ");
			for (int i = 0; i < received_len; i++){					
				printf("%02X", buffer[i]);
			}
		}	
	}	
	printf("\n"); 	
}

