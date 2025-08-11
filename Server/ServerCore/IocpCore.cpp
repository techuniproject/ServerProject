#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"


/*--------------
	IocpCore
---------------*/

IocpCore::IocpCore()
{
	_iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	assert(_iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
	::CloseHandle(_iocpHandle);
}

bool IocpCore::Register(IocpObjectRef iocpObject)
{
	return ::CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, /*key*/0, 0);
	// key->�����ּ� ���� �����߾���
	// ���⼭ �Ѱ��� key���� �ܼ��� ���̰� �� ���� GetQueuedCompletionStatus���� �״�� ��ȯ
	// ������ ��ȯ �� �� ���� üũ ���� ���ϱ� ������ �� key���� ��ȿ�� ���� �ƴ϶�� �ǹ̰� ������
	// key�� �Է��س��� ���� �ּ� ��ü�� ������� ���� io�۾��� �����־� ���߿� �������� �ش� �ּҸ� ��ȯ�ϰ� �� �ּҸ� ����ϸ� ��������.
	// �츮�� ������ key ����� �ƴ� OVERLAPPED���� ����ü ���� owner ������� iocpť ó��
	// key����� weak_from_this().lock() ������ ���� refcount���� �ؾ�������, owner�� ������ְ� ������� ����޾� ������ 
}

/*
EventType	���� owner�� ������	� Dispatch�� ȣ���
Accept	Listener	Listener::Dispatch() �� ProcessAccept()
Connect	Session	Session::Dispatch() �� ProcessConnect()
Recv	Session	Session::Dispatch() �� ProcessRecv()
Send	Session	Session::Dispatch() �� ProcessSend()
Disconnect	Session	Session::Dispatch() �� ProcessDisconnect()

1) Accept ����
AcceptEx �Ϸ� �� Listener::Dispatch() �� ProcessAccept()
���⼭ SO_UPDATE_ACCEPT_CONTEXT �ϰ� session->ProcessConnect()�� ȣ����.

2) ù ���� ����
session->ProcessConnect() �ȿ��� RegisterRecv()�� ȣ���ؼ� ù WSARecv�� �����.
(��, WSARecv�� Listener�� ���� �Ŵ� �� �ƴ϶�, ���� �ʿ��� ��ϵ�.)

3) ���� I/O�� ���� ������ ó��
Ŭ�� �����͸� ������ Session::Dispatch(Recv) �� ProcessRecv() �� �ٽ� RegisterRecv()
�����⵵ ��������: Session::Dispatch(Send) �� ProcessSend()
���⵵: Session::Dispatch(Disconnect) �� ProcessDisconnect()
*/

bool IocpCore::Dispatch(uint32 timeoutMs)
{
	DWORD numOfBytes = 0;
	ULONG_PTR key = 0;
	IocpEvent* iocpEvent = nullptr;

	if (::GetQueuedCompletionStatus(_iocpHandle, OUT & numOfBytes, OUT reinterpret_cast<PULONG_PTR>(&key), OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
	{
		IocpObjectRef iocpObject = iocpEvent->owner;
		iocpObject->Dispatch(iocpEvent, numOfBytes);
	}
	else
	{
		int32 errCode = ::WSAGetLastError();
		switch (errCode)
		{
		case WAIT_TIMEOUT:
			return false;
		default:
			// TODO : �α� ���
			IocpObjectRef iocpObject = iocpEvent->owner;
			iocpObject->Dispatch(iocpEvent, numOfBytes);
			break;
		}
	}

	return true;
}
