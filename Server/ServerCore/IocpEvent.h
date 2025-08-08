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
	//owner�� weak_ptr�� �����ؾ����� �ʳ�?
	//shared_ptr�� owner�����ϸ�, ���� session�� �ּҸ� owner�� ���ٵ�,
	//owner�� session�� ����Ű�� �س��� �� event�� iocp�� ����� ����, �ش� session�� ���ŵǸ�,
	//shared_ptr�� ���� ��, refcount�� 0�� �����ʾ� ���Ű� �ȵ� ������ ���� ���� ���� ó���ϴ� ��Ȳ����
	//���������� ���̻� ��ȿ���� �����Ƿ� ���̻� ������ϴ°� �����̹Ƿ� weak_ptr�� owner�����Ͽ�
	// ���� owner�� ��ȿ���� ������ lock()���� Ȯ���ؼ�, ���̻� ���� x
	SessionRef		session = nullptr; // Accept Only

	// TEMP
	vector<BYTE> buffer;
};