#include <stdio.h>

int main(){
	
	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(4745);

	bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
	/*
	 * listen 함수로 수신 대기열 만들기
	 * */
	listen(sockfd, 5);

	for(;;){
		
		client = sizeof(cliaddr);
		cli_sockfd = accept(addr, (struct sockaddr *)&cliaddr,
				&client);

		readn = read(cli_sockfdf, buf, MAXLINE);
		// buffer 에서 읽기 
		write(cli_sockfd, buf, readn);
		close(cli_sockfd);
	}

}
