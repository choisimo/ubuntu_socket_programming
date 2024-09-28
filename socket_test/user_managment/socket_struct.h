//
// Created by csimo on 5/6/2024.
//
struct sockaddr_in{
    sa_family_t sin_family; //주소 체계
    uint16_t sin_port; //16비트 TCP/UDP PORT번호
    struct in_addr sin_addr; //32비트 IP주소
    char sin_zero[8]; //사용되지 않음(단, 0으로 채워줘야 함)
}