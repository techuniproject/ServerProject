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

// �����Լ� ����, ���� ��� ������ MSVC���� ������� OVERLAPPED����ص� ��
/*
����� ���, OVERLAPPED�� �⺻������ ù ��° ����� ��ġ�� (MSVC����, ���߻�� X�� ��)

������ C++���� ����ü ��� �� �����Ϸ��� �߰����� �е��̳� ���� �Լ� ���̺�(vtable) ���� �ִ´ٸ� �ּҰ� ��߳� �� �־�

�� ��, ǥ�ػ����δ� �������� �ʴٰ� ���� �� �¾�.
*/

struct IocpEvent : public OVERLAPPED
{
	IocpEvent(EventType type);

	void		Init();

	EventType		type;
	IocpObjectRef	owner = nullptr;
	SessionRef		session = nullptr; // Accept Only	
};