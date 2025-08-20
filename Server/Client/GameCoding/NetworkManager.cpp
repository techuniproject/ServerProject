#include "pch.h"
#include "NetworkManager.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "ServerSession.h"

void NetworkManager::Init()
{
	SocketUtils::Init();

	_service = make_shared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		make_shared<IocpCore>(),
		[=]() { return CreateSession(); }, // TODO : SessionManager 등
		1);

	assert(_service->Start());

	//for (int32 i = 0; i < 1; i++)
	//{
	//	GThreadManager->Launch([=]()
	//		{
	//			while (true)
	//			{
	//				service->GetIocpCore()->Dispatch();
	//			}
	//		});
	//}//워커 스레드로 따로 네트워크 처리하면 메인스레드와의 동기화가 컨텐츠적으로 복잡해짐



}

void NetworkManager::Update()
{
	_service->GetIocpCore()->Dispatch(0);//Timeout 0으로해서 기다리지않고 빠져나가도록
}

shared_ptr<ServerSession> NetworkManager::CreateSession()
{
	return _session = make_shared<ServerSession>();
}

void NetworkManager::SendPacket(shared_ptr<SendBuffer> sendBuffer)
{
	if (_session)
		_session->Send(sendBuffer);
}