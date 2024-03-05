# SOCKET() 함수
# socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)

1. AF_INET 
통신 영역 명시, AF_INET 은 IPv4 주소 체계 사용

2. SOCK_STREAM 
프로토콜 타입 설정, 
SOCK_STREAM : TCP,
SOCK_DGRAM : UDP

3. IPPROTO_TCP 
프로토콜 값을 결정

# serveraddr 변수

구조체를 활용하여 서버와 클라이언트 정보를 담음

1. bind 함수
주소 정보를 앞서 생성한 소켓에 할당하는 함수

2. listen 함수
주소가 할당된 소켓이 연결 요청 대기상태로 들어감

3. accept 함수

