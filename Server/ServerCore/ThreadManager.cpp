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
	// 해당 변수는 모든 스레드가 공유하는 전역 변수이므로 atomic으로 선언

	LThreadId = SThreadId.fetch_add(1);
	// fetch_add는 원자적으로 연산 수행, 내부에 CAS 또는 lock방식을 통해 하드웨어 수준에서 원자적으로 연산 보장
	// 결과적으로 여러 스레드 동시 접근해도 한번에 하나씩 수행, 각 스레드는 서로 다른 고유 ID를 정확하게 부여받음
}

void ThreadManager::DestroyTLS()
{

}