#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"

/*--------------
	Session
---------------*/

Session::Session() : _recvBuffer(BUFFER_SIZE)
{
	_socket = SocketUtils::CreateSocket();
	//세션 1개 = 소켓 1개 = 해당 연결에 대한 모든 이벤트 처리 주체
}

Session::~Session()
{
	SocketUtils::Close(_socket);
}

//void Session::Send(BYTE* buffer, int32 len)
//{
//	// 생각할 문제
//	// 1) 버퍼 관리?
//	// 2) sendEvent 관리? 단일? 여러개? WSASend 중첩?
//
//	// TEMP
//	IocpEvent* sendEvent = new IocpEvent(EventType::Send);
//	sendEvent->owner = shared_from_this(); // ADD_REF
//	sendEvent->buffer.resize(len);
//	::memcpy(sendEvent->buffer.data(), buffer, len);
//
//	WRITE_LOCK;//멀티스레드 환경에서 어느 스레드가 언제 send를 호출할지 모르기 때문에 락
//	RegisterSend(sendEvent);
//}

void Session::Send(SendBufferRef sendBuffer)
{
	bool registerSend = false;

	{
		WRITE_LOCK;
		_sendQueue.push(sendBuffer);
		registerSend = _sendRegistered.exchange(true) == false;
	}
	//Send자체는 멀티스레드 환경에서 경쟁을 하여 하나의 스레드가 들어와서 lock을 걸것임
	// 그 와중에 lock이 안걸린 registersend부분은 다른스레드가 실행할 수 있도록 해볼 것임
	// 하지만, 우리는 lock 경쟁에서 이긴 스레드가 registersend까지 하도록 하기 위해 bool이 false였을때 
	// 스택의 registersend를 true로 하여 registersend까지 실행하도록 함.

	/*if (_sendRegistered == false)
	{
		_sendRegistered = true;
		RegisterSend();
	}*/

	if (registerSend)
		RegisterSend();
}

bool Session::Connect()
{
	return RegisterConnect();
}

void Session::Disconnect(const WCHAR* cause)
{
	if (_connected.exchange(false) == false)
		return;

	// TEMP
	wcout << "Disconnect : " << cause << endl;

	OnDisconnected(); // 컨텐츠 코드에서 재정의
	GetService()->ReleaseSession(GetSessionRef());

	RegisterDisconnect();
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}
/*
AcceptEx 완료 → Listener::Dispatch → ProcessAccept

session->ProcessConnect() → 첫 WSARecv 등록

클라가 보냄 → Session::Dispatch(Recv) → ProcessRecv → 다시 RegisterRecv()

이후에도 보낼 때마다 **항상 Session::Dispatch(Recv)**로 처리

*/
void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	switch (iocpEvent->type)
	{
	case EventType::Connect:
		ProcessConnect();
		break;
	case EventType::Disconnect:
		ProcessDisconnect();
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		//ProcessSend(iocpEvent, numOfBytes);
		ProcessSend(numOfBytes);
		break;
	default:
		break;
	}
}

bool Session::RegisterConnect()//클라입장에서 다른 서버로 연결해야할때
{
	if (IsConnected())
		return false;

	if (GetService()->GetServiceType() != ServiceType::Client)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	if (SocketUtils::BindAnyAddress(_socket, 0/*남는거*/) == false)
		return false;

	_connectEvent.Init();
	_connectEvent.owner = shared_from_this(); // ADD_REF

	DWORD numOfBytes = 0;
	SOCKADDR_IN sockAddr = GetService()->GetNetAddress().GetSockAddr();
	if (false == SocketUtils::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0, &numOfBytes, &_connectEvent))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_connectEvent.owner = nullptr; // RELEASE_REF
			return false;
		}
	}

	return true;
}

bool Session::RegisterDisconnect()
{
	_disconnectEvent.Init();
	_disconnectEvent.owner = shared_from_this(); // ADD_REF

	if (false == SocketUtils::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_disconnectEvent.owner = nullptr; // RELEASE_REF
			return false;
		}
	}

	return true;
}

void Session::RegisterRecv()
{
	if (IsConnected() == false)
		return;

	_recvEvent.Init();
	_recvEvent.owner = shared_from_this(); // ADD_REF

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WritePos());
	wsaBuf.len = _recvBuffer.FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if (SOCKET_ERROR == ::WSARecv(_socket, &wsaBuf, 1, OUT & numOfBytes, OUT & flags, &_recvEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_recvEvent.owner = nullptr; // RELEASE_REF
		}
	}
}
void Session::RegisterSend()
{
	if (IsConnected() == false)
		return;

	_sendEvent.Init();
	_sendEvent.owner = shared_from_this(); // ADD_REF

	// 보낼 데이터를 sendEvent에 등록
	{
		WRITE_LOCK;

		int32 writeSize = 0;
		while (_sendQueue.empty() == false)
		{
			SendBufferRef sendBuffer = _sendQueue.front();

			writeSize += sendBuffer->WriteSize();
			// TODO : 예외 체크

			_sendQueue.pop();
			_sendEvent.sendBuffers.push_back(sendBuffer);
		}
	}

	// Scatter-Gather (흩어져 있는 데이터들을 모아서 한 방에 보낸다)
	vector<WSABUF> wsaBufs;
	wsaBufs.reserve(_sendEvent.sendBuffers.size());
	for (SendBufferRef sendBuffer : _sendEvent.sendBuffers)
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
		wsaBufs.push_back(wsaBuf);
	}

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT & numOfBytes, 0, &_sendEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_sendEvent.owner = nullptr; // RELEASE_REF
			_sendEvent.sendBuffers.clear(); // RELEASE_REF
			_sendRegistered.store(false);
		}
	}
}
//void Session::RegisterSend(IocpEvent* sendEvent)
//{
//	if (IsConnected() == false)
//		return;
//
//	WSABUF wsaBuf;
//	wsaBuf.buf = (char*)sendEvent->buffer.data();
//	wsaBuf.len = (ULONG)sendEvent->buffer.size();
//
//	DWORD numOfBytes = 0;
//	if (SOCKET_ERROR == ::WSASend(_socket, &wsaBuf, 1, OUT & numOfBytes, 0, sendEvent, nullptr))
//	{
//		int32 errorCode = ::WSAGetLastError();
//		if (errorCode != WSA_IO_PENDING)
//		{
//			HandleError(errorCode);
//			sendEvent->owner = nullptr; // RELEASE_REF
//			delete sendEvent;
//		}
//	}
//}
/*
서버 Accept 완료 → 세션 생성

session->ProcessConnect() → RegisterRecv() → 첫 WSARecv 호출

클라가 send()

커널이 우리가 등록해 둔 WSARecv 요청을 완료 상태로 만듦

완료 정보를 IOCP 큐에 넣음

워커 스레드에서 GetQueuedCompletionStatus()로 꺼내서 처리

처리 끝나면 다시 WSARecv 등록 → 다음 데이터 기다림
*/
void Session::ProcessConnect()
{
	_connectEvent.owner = nullptr; // RELEASE_REF

	_connected.store(true);

	// 세션 등록
	GetService()->AddSession(GetSessionRef());

	// 컨텐츠 코드에서 재정의
	OnConnected();

	// 수신 등록
	RegisterRecv();
}

void Session::ProcessDisconnect()
{
	_disconnectEvent.owner = nullptr; // RELEASE_REF
}

void Session::ProcessRecv(int32 numOfBytes)
{
	_recvEvent.owner = nullptr; // RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect(L"Recv 0");
		return;
	}


	if (_recvBuffer.OnWrite(numOfBytes) == false) {
		Disconnect(L"OnWrite Overflow");
		return;
	}
	int32 dataSize = _recvBuffer.DataSize();
	// 컨텐츠 코드에서 재정의
	int32 processLen = OnRecv(_recvBuffer.ReadPos(), dataSize);

	if (processLen < 0 || dataSize < processLen || _recvBuffer.OnRead(processLen) == false)
	{
		Disconnect(L"OnRead Overflow");
		return;
	}

	// 커서 정리
	_recvBuffer.Clean();

	// 수신 등록
	RegisterRecv();
}


void Session::ProcessSend(int32 numOfBytes)
{
	_sendEvent.owner = nullptr; // RELEASE_REF
	_sendEvent.sendBuffers.clear(); // RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	// 컨텐츠 코드에서 재정의
	OnSend(numOfBytes);

	/*WRITE_LOCK;
	if (_sendQueue.empty())
		_sendRegistered.store(false);
	else
		RegisterSend();*/

	bool registerSend = false;

	{
		WRITE_LOCK;
		if (_sendQueue.empty())
			_sendRegistered.store(false);
		else
			registerSend = true;
	}

	if (registerSend)
		RegisterSend();
}

//void Session::ProcessSend(IocpEvent* sendEvent, int32 numOfBytes)
//{
//	sendEvent->owner = nullptr; // RELEASE_REF
//	delete sendEvent;
//
//	if (numOfBytes == 0)
//	{
//		Disconnect(L"Send 0");
//		return;
//	}
//
//	// 컨텐츠 코드에서 재정의
//	OnSend(numOfBytes);
//}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect(L"HandleError");
		break;
	default:
		// TODO : Log
		cout << "Handle Error : " << errorCode << endl;
		break;
	}
}