#include "pch.h"
#include <iostream>

// Ŭ��
// 1) ���� ����
// 2) ������ ���� ��û
// 3) ���

int main()
{

	SocketUtils::Init();

	SOCKET clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
		return 0;

	//�� ���ŷ
	u_long on = 1;
	if (::ioctlsocket(clientSocket, FIONBIO, &on) == INVALID_SOCKET)
		return 0;

	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = ::htons(7777);

	//Connect
	while (true) {
		if (::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			//�̹� ����� ���¶�� ���� break
			if (::WSAGetLastError() == WSAEISCONN)//�̹� connect��û������ �ٽ� connect�Ϸ��� �Ҷ� ����
				break;
		}
	}

	//send
	while (true) {
		char sendBuffer[100] = "Hello I am Client!";
		int32 sendLen = sizeof(sendBuffer);

		if (::send(clientSocket, sendBuffer, sendLen, 0) == SOCKET_ERROR)
		{
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;


		}
		cout << "Send Data Len = " << sendLen << endl;
		this_thread::sleep_for(1s);
	}

	SocketUtils::Clear();

}

