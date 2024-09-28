#include <stdio.h>


int main(){
	/*
	 * client
	 * */

	int sockfd;
	// 소켓 주소 구조체 
	struct sockaddr_in addr;
	sockfd = socket(AF_INET, SOCKET_STREAM, 0);
	/*
	 *  sin_family : 주소 체계를 나타내며, 부호 있는 16비트 정수 값 사용.
	 *  항상 AF_INET 값을 사용한다.
	 *  TCP/IP 프로토콜 (AF_INET_)
	 */
	addr.sin_family = AF_INET;
	// 연결 서버 주소 INADDR_ANY server 주소로 변경
	// inet_addr("111.111.111.111");
	addr.sin_addr.s_addr = INADDR_ANY;
	// server port 지정 
	addr.sin_port = htons(4744);
	
	
	connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
	/*
	 * 
	 * */
	bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

}
