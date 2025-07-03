#include "pch.h"
#include <iostream>

// Ŭ��
// 1) ���� ����				- ex) ���濡�� ���Ǹ� �ִ� �ڵ���
// 2) ������ ���� ��û
// 3) ���

int main()
{
	// 1) ���� ����	- ���� ����ϴ� ����
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	// ipv4 ����, TCP ���
	SOCKET clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
		return 0;

	// 2) �ּ�/��Ʈ ��ȣ ���� (bind) - �����ϰ� ���� ����� �ּ� ����
	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	// serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // "127.0.0.1": ������ �ּ�(�ڽ��� �ּ�)
	serverAddr.sin_port = ::htons(7777); // 80 : HTTP

	if (::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;

	// -----------
	// ���� ����!
	cout << "Connected To Server!" << endl;

	while (true)
	{
		// ��Ŷ
		char sendBuffer[100] = "Hello ! I am Client!";

		// send: �޼����� ������ �Լ�
		// ��ȭ�� �� ��뿡�ٰ� �ش� ���� ũ�⸸ŭ ����
		int32 resultCode = ::send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);
		if (resultCode == SOCKET_ERROR)
			return 0;

		// ������ ���ڷ� ���� ���� �޴� ��
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

