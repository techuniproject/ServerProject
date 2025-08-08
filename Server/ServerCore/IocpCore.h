#pragma once

/*----------------
	IocpObject
-----------------*/

/*
shared_ptr�� ���޸𸮸� �����ϴ� ��ü
���������� control block�� �����ϴµ�, ������ refcount, weakcount, T�� �Ҹ����̴�.
refcount�� 0�̵Ǹ� �Ҹ��ڸ� ȣ���ϰ� ��.
���ø޸𸮸� shared_ptr�� ȣ���ϴ� ���� ���̾ȵ�, ������ ����� ���� �޸𸮴� ��ȯ�Ǳ� ������ ���� �޸𸮸� ���ַ��� �õ���. 

weak_ptr�� ��ü�� ������ ������ ��.
���� refcount�� �ø���������, weak_count�� �ø��� �ȴ�.
weak_ptr�� ��ü�� �����ϰ� �ִ� ��Ȳ���� �� ��ü�� refcount�� 0�� �Ǹ� �ش� ��ü�� ���������,
weak_count�� 0�� �ƴϱ� ������ control block�� ������� �ʴ´�. �׷� ���� control block���� �����ϴ�
.lock() �Լ��� ����Ͽ� �ش� ��ü�� �����ֱ� ���θ� ��� �� �ְ� �ȴ�.
.lock() ȣ��� ���� ��ü�� ��ȿ�ϸ�(refcount�� 0�� �ƴ϶��) shared_ptr<T>���·� ��ȯ, ��ȿ ���ϸ� nullptr��ȯ
.expired() �� �Ҹ� ���� boolean���� ��ȯ
.use_count() �� ���� ����ִ� shared_ptr���� ��ȯ

control block �Ҹ� ���� -> refcount==0 && weakcount==0
��ü �Ҹ����� -> refcount==0

*/


// �ڽſ� ���� shared_ptr�� shared_ptr<T> self(this) ���·� ���� ��,
// �ܺο��� �� ��ü�� ���� �̹� shared_ptr�� ������ ��� ���� ���� ���� ����. (refcount �ΰ��� ������)��
//enable_shared_from_this�� �� ��ü�� �����ϴ� ���� shared_ptr�� ������� shared_ptr�������
//-> �ϳ��� refcount�� �����ǰԲ� ��.
// ���������δ� enabled_shared_from_this�� weak_ptr<T> weak_this ����� ������,
// �̰� ��ӹ��� ��ü�� ó������ shared_ptr�� ������, weak_this.lock()�� ȣ���Ͽ� ������ ���� ��� ������ shared_ptr����
// �׷��� ������ new�� ���� ��ü�ε� �̰� ��ӹ����� ���ܰ� �����, �ݵ�� shared_ptr�� ��������.

class IocpObject : public enable_shared_from_this<IocpObject>
{
public:
	virtual HANDLE GetHandle() abstract;
	virtual void Dispatch(struct IocpEvent* iocpEvent, int32 numOfBytes = 0) abstract;
};

/*--------------
	IocpCore
---------------*/

class IocpCore
{
public:
	IocpCore();
	~IocpCore();

	HANDLE		GetHandle() { return _iocpHandle; }

	bool		Register(IocpObjectRef iocpObject);
	bool		Dispatch(uint32 timeoutMs = INFINITE);

private:
	HANDLE		_iocpHandle;
};