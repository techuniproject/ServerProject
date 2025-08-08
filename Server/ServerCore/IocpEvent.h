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
	//owner은 weak_ptr로 관리해야하지 않나?
	//shared_ptr로 owner관리하면, 보통 session의 주소를 owner로 줄텐데,
	//owner에 session을 가리키게 해놓고 이 event를 iocp에 등록한 이후, 해당 session이 제거되면,
	//shared_ptr로 관리 시, refcount가 0이 되지않아 제거가 안돼 만료한 세션 상대로 무언가 처리하는 상황생김
	//원래같으면 더이상 유효하지 않으므로 더이상 진행안하는게 정석이므로 weak_ptr로 owner관리하여
	// 만약 owner도 유효하지 않으면 lock()으로 확인해서, 더이상 진행 x
	SessionRef		session = nullptr; // Accept Only

	// TEMP
	vector<BYTE> buffer;
};