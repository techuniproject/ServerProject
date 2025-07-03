#include "pch.h"
#include "ThreadManager.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"

/*------------------
	ThreadManager
-------------------*/

ThreadManager::ThreadManager()
{
	// Main Thread
	InitTLS();
}

ThreadManager::~ThreadManager()
{
	Join();
}

void ThreadManager::Launch(function<void(void)> callback)
{
	LockGuard guard(_lock);

	_threads.push_back(thread([=]()
		{
			InitTLS();
			callback();
			DestroyTLS();
		}));
}

void ThreadManager::Join()
{
	for (thread& t : _threads)
	{
		if (t.joinable())
			t.join();
	}
	_threads.clear();
}

void ThreadManager::InitTLS()
{
	static Atomic<uint32> SThreadId = 1;
	// �ش� ������ ��� �����尡 �����ϴ� ���� �����̹Ƿ� atomic���� ����

	LThreadId = SThreadId.fetch_add(1);
	// fetch_add�� ���������� ���� ����, ���ο� CAS �Ǵ� lock����� ���� �ϵ���� ���ؿ��� ���������� ���� ����
	// ��������� ���� ������ ���� �����ص� �ѹ��� �ϳ��� ����, �� ������� ���� �ٸ� ���� ID�� ��Ȯ�ϰ� �ο�����
}

void ThreadManager::DestroyTLS()
{

}