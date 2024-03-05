#include <stdio.h>
#include <stdlb.h>
#include <string.h>
#include <winsock2.h>
#include <fcntl.h>

int main(){
	
	int open(const char *pathname, int flags);
	int open(const char *pathname, int flags, mode_t mode);

	// "hello.txt" 쓰기 권한, chmod 0755 주기 //
	/*
	 * O_RDONLY 읽기 전용
	 * O_WRONLY 쓰기 전용
	 * O_RDWR 읽기 / 쓰기
	 * O_CREATE 파일 생성
	 * O_EXCL 파일 존재 유무검사
	 * O_APPEND 추가 모드로 열기
	 * O_NONBLOCK 비 봉쇄 모드로 열기
	 */
	fd = open("test_page/hello.txt", O_WRONLY|O_EXCL|O_CREAT, 0755);

}


