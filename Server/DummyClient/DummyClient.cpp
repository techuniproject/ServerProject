#include "pch.h"
#include <iostream>

// 클라
// 1) 소켓 생성
// 2) 서버에 연결 요청
// 3) 통신

int main()
{
	// 1) 소켓 생성
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	// ipv4 버전, TCP 방식
	SOCKET clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
		return 0;

	// 2) 주소/포트 번호 설정 (bind)
	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	// serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = ::htons(7777); // 80 : HTTP

	if (::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;

	// -----------
	// 연결 성공!
	cout << "Connected To Server!" << endl;

	while (true)
	{
		// 패킷
		char sendBuffer[100] = "Hello ! I am Client!";
		int32 resultCode = ::send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);
		if (resultCode == SOCKET_ERROR)
			return 0;

		char recvBuffer[100];
		int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
		if (recvLen <= 0)
			return 0;

		cout << "Echo Data : " << recvBuffer << endl;

		this_thread::sleep_for(chrono::seconds(1));
	}
	::closesocket(clientSocket);
	::WSACleanup();
}

