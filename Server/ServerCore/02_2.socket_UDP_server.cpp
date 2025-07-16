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

//UDP 서버
// 1) 새로운 소켓 생성
// 2) 소켓에 IP주소/ 포트번호 설정(bind)
// -------
// 3) 클라와 통신

int main()
{
	// 1) 소켓 생성
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;


	// ipv4 버전, TCP 방식
	SOCKET listenSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (listenSocket == INVALID_SOCKET) // 오류 반환값이 아니라면 통과
		return 0;



	// 2) 주소/포트 번호 설정 (bind)
	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr)); //0으로 밀어 초기화
	serverAddr.sin_family = AF_INET; //전송 주소의 주소 패밀리(체계) : 항상 AF_INET(IPv4)으로 설정
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY); // 0.0.0.0 서버가 가지고 있는 모든 IP주소에서의 요청을 받는 뜻
	serverAddr.sin_port = ::htons(7777); // 80까지는 HTTP가 사용 중 - 사용할 포트 번호를 지정 -7777

	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;


	//// 3) 업무 개시 (listen)
	//if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	//	return 0;
	//TCP에선 클라의 listening 소켓이 연결을 기다리지만, UDP는 없음

	while (true)
	{
		SOCKADDR_IN clientAddr;
		::memset(&clientAddr, 0, sizeof(clientAddr));
		int32 addrLen = sizeof(clientAddr); //상대 클라의 주소를 저장하기 위함

		//SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		//if (clientSocket == INVALID_SOCKET)
		//	return 0;

		////----여기까진 이제 상대 클라와 직접 통신할 소켓 만들어져 상대 클라와 통신 가능 상태

		//char ip[16];
		//::inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip)); // ip추출 함수
		//cout << "Client Connected! IP = " << ip << endl;
		//TCP에서 connect 기반 맺는 과정 UDP에선 없음


		// 패킷
		char recvBuffer[100];


		int32 recvLen = ::recvfrom(listenSocket, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&clientAddr, &addrLen);
		if (recvLen <= 0)
			return 0;

		//int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
		//if (recvLen <= 0)
		//	return 0; //TCP형식

		cout << "Recv Data : " << recvBuffer << endl;
		cout << "Recv Data Len: " << recvLen << endl;

		this_thread::sleep_for(chrono::seconds(1));

	}

	::closesocket(listenSocket);
	::WSACleanup(); //WSAStartUp 호출 시 호출해야 하는 함수
}