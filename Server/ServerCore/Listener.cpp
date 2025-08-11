#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "IocpEvent.h"
#include "Session.h"
#include "Service.h"

/*--------------
	Listener
ServerService에서 Listener 생성 후 StartAccept() 호출.

TCP 리스닝 소켓 생성 → IOCP에 등록.

소켓 옵션 세팅 (재사용, Linger).

IP/Port에 바인드 후 Listen 상태.

MaxSessionCount만큼 AcceptEx 요청 사전 등록.

클라이언트 접속 시, 해당 Accept 이벤트가 IOCP에 완료로 올라오고 세션 생성.
---------------*/

Listener::~Listener()
{
	SocketUtils::Close(_socket);

	for (IocpEvent* acceptEvent : _acceptEvents)
	{
		// TODO
		delete acceptEvent;
	}
}

bool Listener::StartAccept(ServerServiceRef service)
{
	_service = service;
	if (_service == nullptr)
		return false;

	_socket = SocketUtils::CreateSocket();
	if (_socket == INVALID_SOCKET)
		return false;

	if (_service->GetIocpCore()->Register(shared_from_this()) == false)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	if (SocketUtils::SetLinger(_socket, 0, 0) == false)
		return false;

	if (SocketUtils::Bind(_socket, _service->GetNetAddress()) == false)
		return false;

	if (SocketUtils::Listen(_socket) == false)
		return false;

	const int32 acceptCount = _service->GetMaxSessionCount();
	// 클라의 연결 요청을 받을 준비하는 부분 (Connect)
	for (int32 i = 0; i < acceptCount; i++)
	{	//동시에 여러개의 acceptex요청을 미리 걸어놓는
		IocpEvent* acceptEvent = new IocpEvent(EventType::Accept);
		acceptEvent->owner = shared_from_this();
		_acceptEvents.push_back(acceptEvent);
		RegisterAccept(acceptEvent);
	}
	//MaxSessionCount 개수만큼 AcceptEx 비동기 요청 걸어둠
	//이렇게 하여 여러 클라이언트가 동시에 접속해도 대기없이 처리 가능
	// owner은 이 이벤트를 처리할 주인(Listener)을 추적하기위함
	// RegisterAccept()에서 실제 AcceptEx()호출

	return true;
}

void Listener::CloseSocket()
{
	SocketUtils::Close(_socket);
}

HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

// 첫 listener에서 accept에 대한 처리는 event로 StartAccept로 만들어줬던거에 대한 처리 Dispatch에서
/*
for (int32 i = 0; i < acceptCount; i++)
	{	//동시에 여러개의 acceptex요청을 미리 걸어놓는
		IocpEvent* acceptEvent = new IocpEvent(EventType::Accept);
		acceptEvent->owner = shared_from_this();
		_acceptEvents.push_back(acceptEvent);
		RegisterAccept(acceptEvent);
	}
*/
void Listener::Dispatch(IocpEvent* acceptEvent, int32 numOfBytes)
{
	assert(acceptEvent->type == EventType::Accept);
	ProcessAccept(acceptEvent);
}

void Listener::RegisterAccept(IocpEvent* acceptEvent)
{
	SessionRef session = _service->CreateSession(); // Register IOCP

	//클라의 입출력용 session을 여기서 만듦
	acceptEvent->Init();
	acceptEvent->session = session;


	DWORD bytesReceived = 0;
	if (false == SocketUtils::AcceptEx(_socket, session->GetSocket(), session->_recvBuffer.WritePos(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, OUT & bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent)))
	{//마지막 overlapped기반 acceptevent그대로 getqueuedcompletionstatus 시 완료통지 받으면 그대로 받음
		const int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			// 일단 다시 Accept 걸어준다
			RegisterAccept(acceptEvent);
		}
	}
}

void Listener::ProcessAccept(IocpEvent* acceptEvent)//클라연결시 트리거 iocp->dispatch를 통해
{
	SessionRef session = acceptEvent->session;

	if (false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), _socket))
	{//내부적으로 setsockopt호출 acceptex로 얻은 소켓에 리스너 컨텍스트 연결하여야 getpeername정상동작
		// 실패 시 이번 accept포기후 다시 acceptex 재등록
		RegisterAccept(acceptEvent);
		return;
	}

	SOCKADDR_IN sockAddress;
	int32 sizeOfSockAddr = sizeof(sockAddress);
	if (SOCKET_ERROR == ::getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
	{//getpeername -> 방금 연결된 클라 ip/포트 언음. 실패시 이번 건 건너뛰고 다시 AcceptEx
		RegisterAccept(acceptEvent);
		return;
	}

	cout << "Client Connected!" << endl;

	session->SetNetAddress(NetAddress(sockAddress));
	session->ProcessConnect(); //여기서 wsarecv첫 세팅 및 호출 이후 클라 io기다림
	RegisterAccept(acceptEvent);
}