//#include "pch.h"
//#include <iostream>
//
//// 클라
//// 1) 소켓 생성
//// 2) 서버에 연결 요청
//// 3) 통신
//
//int main()
//{
//	
//	SocketUtils::Init();
//
//	SOCKET clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
//	if (clientSocket == INVALID_SOCKET)
//		return 0;
//
//	//논 블로킹
//	u_long on = 1;
//	if (::ioctlsocket(clientSocket, FIONBIO, &on) == INVALID_SOCKET)
//		return 0;
//
//	SOCKADDR_IN serverAddr;
//	::memset(&serverAddr, 0, sizeof(serverAddr));
//	serverAddr.sin_family = AF_INET;
//	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
//	serverAddr.sin_port = ::htons(7777);
//
//	//Connect
//	while (true) {
//		if (::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
//		{
//			if (::WSAGetLastError() == WSAEWOULDBLOCK)
//				continue;
//
//			//이미 연결된 상태라고 보고 break
//			if (::WSAGetLastError() == WSAEISCONN)//이미 connect요청했을때 다시 connect하려고 할때 오류
//				break;
//		}
//	}
//
//	//send
//	while (true) {
//		char sendBuffer[100] = "Hello I am Client!";
//		int32 sendLen = sizeof(sendBuffer);
//
//		if (::send(clientSocket, sendBuffer, sendLen, 0) == SOCKET_ERROR)
//		{
//			if (::WSAGetLastError() == WSAEWOULDBLOCK)
//				continue;
//
//			
//		}
//		cout << "Send Data Len = " << sendLen << endl;
//		this_thread::sleep_for(1s);
//
//		char recvBuffer[100];
//		int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
//		if (recvLen <= 0)
//			return 0;
//
//		cout << "Echo Data : " << recvBuffer << endl;
//	}
//
//	SocketUtils::Clear();
//	
//}

#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "ClientPacketHandler.h"

char sendData[] = "Hello World";

class ServerSession : public PacketSession
{
public:
	~ServerSession()
	{
		cout << "~ServerSession" << endl;
	}

	virtual void OnConnected() override
	{
		cout << "Connected To Server" << endl;

		SendBufferRef sendBuffer = make_shared<SendBuffer>(4096);
		sendBuffer->CopyData(sendData, sizeof(sendData));
		Send(sendBuffer);
	}

	virtual void OnRecvPacket(BYTE* buffer, int32 len)override
	{
		cout << "OnRecv Len = " << len << endl;

		ClientPacketHandler::HandlePacket(buffer, len);
		
	}

	/*virtual int32 OnRecv(BYTE* buffer, int32 len) override
	{
		
			cout << "OnRecv Len = " << len << endl;
		
		this_thread::sleep_for(0.1s);

		SendBufferRef sendBuffer = make_shared<SendBuffer>(4096);
		sendBuffer->CopyData(sendData, sizeof(sendData));
		Send(sendBuffer);

		return len;
	}*/

	virtual void OnSend(int32 len) override
	{
		cout << "OnSend Len = " << len << endl;
	}

	virtual void OnDisconnected() override
	{
		cout << "Disconnected" << endl;
	}
};

int main()
{
	this_thread::sleep_for(1s);

	SocketUtils::Init();

	ClientServiceRef service = make_shared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		make_shared<IocpCore>(),
		[]() { return make_shared<ServerSession>(); }, // TODO : SessionManager 등
		5);

	assert(service->Start());

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					service->GetIocpCore()->Dispatch();
				}
			});
	}

	GThreadManager->Join();
	SocketUtils::Clear();
}