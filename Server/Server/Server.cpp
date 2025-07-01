#include "pch.h"
#include <iostream>
#include <thread>
#include <vector>
using namespace std;
#include <atomic>
#include <mutex>
#include <windows.h>
#include "TestMain.h"
#include "ThreadManager.h"

void TestThread()
{
	cout << "Hi! I am thread : " << LThreadId << endl;

	while (true)
	{

	}
}

int main()
{
	TestServerCore();

	for (int32 i = 0; i < 10; i++)
		GThreadManager->Launch(TestThread);

	GThreadManager->Join();
}