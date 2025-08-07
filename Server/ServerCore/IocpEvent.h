#pragma once

class Session;

enum class EventType : uint8
{
	Connect,
	Disconnect,
	Accept,
	Recv,
	Send
};

/*--------------
	IocpEvent
---------------*/

// 가상함수 없고, 다중 상속 없으면 MSVC에선 상속으로 OVERLAPPED사용해도 됨
/*
상속의 경우, OVERLAPPED는 기본적으로 첫 번째 멤버로 배치됨 (MSVC에서, 다중상속 X일 때)

하지만 C++에서 구조체 상속 시 컴파일러가 추가적인 패딩이나 가상 함수 테이블(vtable) 등을 넣는다면 주소가 어긋날 수 있어

→ 즉, 표준상으로는 안전하지 않다고 보는 게 맞아.
*/

struct IocpEvent : public OVERLAPPED
{
	IocpEvent(EventType type);

	void		Init();

	EventType		type;
	IocpObjectRef	owner = nullptr;
	SessionRef		session = nullptr; // Accept Only	
};