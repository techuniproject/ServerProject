// 서버
#include "pch.h"
#include <iostream>
#include <thread>
#include <vector>
using namespace std;
#include <atomic>
#include <mutex>
#include <windows.h>
#include "TestMain.h"
#include "ThreadManager.h"

int main()
{
	// 1) 새로운 소켓 생성 (socket)				- ex) 직원 자리 배치
	// 2) 소켓에 주소/포트 번호 설정 (bind)		- ex) 직원 교육
	// 3) 소켓 일 시키기 (listen)				- ex) 직원 근무 시작(영업 개시)
	// 4) 손님 접속 (accept)					- ex) 손님 입장
	// 5) 클라와 통신							- ex) 서비스 시작

	// 소켓
	// 네트워크 통신을 도와주는 핸드폰
	
	// 브로드캐스팅
	// 인접한 지역에 있는 캐릭터 하나씩에게 패킷을 쏴주는 것

	WSADATA wsaData;
	// == 0x202

	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // 초기화
		return 0;

	// MAKEWORD
	// 1바이트 1바이트로 하이와 로우로 묶어서 하나의 WORD를 만듦.

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	// ipv4 버전으로 TCP 방식으로 만듬
	// 어떤 식으로 통신을 할지를 정해줌.
	// 
	// 리턴 값(SOCKET): 커널에서 관리하는 리소스에 정수 번호를 부여한 값
	// 해당 소켓을 대상으로 무언가를 요청할 때 해당 정수를 사용한다는 개념

	// 1) 소켓 생성
	// ad : Address Family (AF_INET = IPv4, AF_INET6 = IPv6)
	// type : TCP(SOCK_STREAM) vs UDP(SOCK_DGRAM)
	// protocol: 0
	// return : descriptor
	// int32 errorCode = ::WSAGetLastError();
	
	// 2) 주소/포트 번호 설정 (bind)
	// 소켓 == 핸드폰(네트워크 통신을 도와주는)
	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY); // 어떤 IP 주소를 맵핑하고 싶은지(INADDR_ANY: 상관X)
	serverAddr.sin_port = ::htons(7777); // 80 : HTTP, 사용하지 않을 숫자로 설정

	// 리틀 엔디언 - 현재 환경에서 표준
	// 빅 엔디언 - 네트워크 상에서 표준

	// ex) 0x12345678
	// low [0x78][0x56][0x34][0x12] high << little
	// low [0x12][0x34][0x56][0x78] high << big
	=> htonl(host to network long), htons(host to network short) 사용 - 동일한 환경으로 맞춰주기 위해 사용
	// 네트워크 표준에 맞는 방식으로 바꿔주는 함수

	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;
	// 어떠한 주소를 바인딩(맵핑) - IP주소와 포트 번호 연결

	// 3) 업무 개시 (listen)
	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		return 0;
	// SOMAXCONN: 대기할 수 있는 숫자 관련 옵션 - 최대치

	// 4)
	while (true)
	{
		SOCKADDR_IN clientAddr;
		::memset(&clientAddr, 0, sizeof(clientAddr));
		int32 addrLen = sizeof(clientAddr);

		// 상대방 쪽 소켓
		// accept는 입장한 손님이 없으면 멈춤
		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
			return 0;

		char ip[16];
		::inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
		cout << "Client Connected! IP = " << ip << endl;

		while (true)
		{
			// 패킷
			char recvBuffer[100];

			// recv: 메세지를 받는 함수
			int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
			if (recvLen <= 0)
				return 0;

			cout << "Recv Data : " << recvBuffer << endl;
			cout << "Recv Data Len: " << recvLen << endl;

			// 에코 서버 - 상대방이 보내준 메세지를 그대로 토스 (거울)
			// 받은 것을 다시 send
			int32 resultCode = ::send(clientSocket, recvBuffer, recvLen, 0);
			if (resultCode == SOCKET_ERROR)
				return 0;
		}
	}

	::WSACleanup();
	// 코드가 끝나면 대칭적으로 호출해주어야 함.
}

// 현재 사용하는 방식은 데이터를 보내지 않는다면 서버는 멈추게 된다.


// 클라 - 서버 간 짝이 맞지 않을 경우
// send <-> recv
// ex) 서버에서 send를 하고 클라이언트에서 recv로 받지 않을 경우
// => 결과: send는 성공!

// 커널 내부에서 각 소켓은 내부적으로 recv, send Buffer를 소유하고 있음.
// 자신의 버퍼에다가 해당 내용을 복사하는 것을 성공이라고 인식, 그 이후는 운영체제가 처리함.
// Send 성공 == Send Buffer에 데이터를 복사하는 것
// Receive 성공 == Receive Buffer에 데이터가 있으면 복사해서 가지고 오는 것
// Send 실패 == 샌드 버퍼가 꽉참 (ex) Receive를 하지 않아서)