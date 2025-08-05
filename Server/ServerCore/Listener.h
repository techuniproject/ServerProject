#pragma once
#include "IocpCore.h"
#include "NetAddress.h"


/*--------------
	Listener
---------------*/

class Listener : public IocpObject
{
public:
	Listener() = default;
	~Listener();

public:
	/* �ܺο��� ��� */
	bool StartAccept(NetAddress netAddress);
	void CloseSocket();

public:
	/* �������̽� ���� */
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(struct IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	/* ���� ���� */
	void RegisterAccept(IocpEvent* acceptEvent);
	void ProcessAccept(IocpEvent* acceptEvent);

protected:
	SOCKET _socket = INVALID_SOCKET;
	vector<IocpEvent*> _acceptEvents;
};

