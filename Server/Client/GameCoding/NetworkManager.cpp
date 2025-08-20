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
		[=]() { return CreateSession(); }, // TODO : SessionManager ��
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
	//}//��Ŀ ������� ���� ��Ʈ��ũ ó���ϸ� ���ν�������� ����ȭ�� ������������ ��������



}

void NetworkManager::Update()
{
	_service->GetIocpCore()->Dispatch(0);//Timeout 0�����ؼ� ��ٸ����ʰ� ������������
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