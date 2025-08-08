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
	// key->고유주소 원래 기입했었음
	// 여기서 넘겨준 key값은 단순한 값이고 그 값을 GetQueuedCompletionStatus에서 그대로 반환
	// 하지만 반환 할 때 수명 체크 등을 안하기 때문에 그 key값이 유효한 값이 아니라면 의미가 없어짐
	// key를 입력해놓은 고유 주소 객체가 사라지고 나서 io작업이 남아있어 나중에 꺼내쓸때 해당 주소를 반환하고 그 주소를 사용하면 문제생김.
	// 우리는 앞으로 key 방식이 아닌 OVERLAPPED관리 구조체 내부 owner 방식으로 iocp큐 처리
	// key방식은 weak_from_this().lock() 등으로 직접 refcount관리 해야하지만, owner은 등록해주고 수명관리 보장받아 안전함 
}

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
			// TODO : 로그 찍기
			IocpObjectRef iocpObject = iocpEvent->owner;
			iocpObject->Dispatch(iocpEvent, numOfBytes);
			break;
		}
	}

	return true;
}
