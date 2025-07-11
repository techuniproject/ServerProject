#include <iostream>
#include <windows.h>
#include <atomic>
#include <mutex>
#include <queue>
using namespace std;

mutex m;
queue<int> q;
HANDLE hEvent; //Ŀ�� ������Ʈ ����Ű�� �ڵ� ����
condition_variable cv; // Ŀ�� ������Ʈ�� �ƴ� �������� ������Ʈ

void Producer()
{
	while (true) {

		// CV ��� ��,
		// 1) Lock ���
		// 2) ���� ���� �� ����
		// 3) Lock �� Ǯ�� 
		// 4) CV ���ؼ� ����
		{
			unique_lock<mutex> lock(m);
			q.push(100);
			//���⼭ notify_one������ ���� Ǯ���� ���� �ð����� �ݴ����� wait��
			// cv.notify_one();
			// ���⼭ ȣ���ϸ� ���� ������� mutexȹ���� ���� �ٽ� ����� �� ����. lock�����̹Ƿ�

		}//���� Ǯ�� CV������ ���� ���� ����
		cv.notify_one(); //��� Ÿ�� �ִ� �ֵ� �� �ϳ��� ����, notifyall�� �� ����


		//event���
	//	unique_lock<mutex> lock(m);
	//	q.push(100);

	//	::SetEvent(hEvent); //Signal (�Ķ��ҷ� �ٲ�)
	//
	//	this_thread::sleep_for(100ms); // ���� �����带 ��� �纸�Ͽ� ���ؽ�Ʈ ����Ī ��Ű��
									   // blocked���� �� �ٲٰ� 100ms ���� ready ���� ť�� ����



	}
}

void Consumer()
{
	while (true) {

		// ::WaitForSingleObject(hEvent, INFINITE); // ���� �����带 Ŀ���� Blocked ���·� ��ȯ - CPU ��ȯ + ���� ������ ���
												 // SetEvent�� ȣ��Ǿ� Signaled���µǸ� OS�� ��� ���� �����带 Ready���·� �ƿ�
												 // Event�� �Ķ��ҵɶ����� 
												 // CPU �˻縦 ���� ������ ����ϴ� ���� �ƴ� �����ִٰ� ����

		// ::ResetEvent(hEvent); //Manual reset�� �������� �ϸ� �� �Լ��� �ʱ���� ������ �ʱ�ȭ

		//�̺�Ʈ ����� �ᱹ ����� �ϴ��� ���ǿ� ���� �ǹ̾��� CPU Ÿ�ӽ����̽� �Ҹ� ��, �ڿ����� ��
		// �׷��Ƿ� condition_variable�� ������ ���ÿ� üŷ�� �� �ִ� ���·� �ϸ� ����.

		//-> cv�������
		unique_lock<mutex> lock(m);
		cv.wait(lock, []() {return q.empty() == false; });
		// 1) Lock�� �������� �õ� (�̹� �������� skip)
		// 2) ������ Ȯ�� (unique_lock�� �� ���� : �߰��� lock�� �ɰ� Ǯ�� �����ϰ� �ϱ� ����)
		//  -> ���� �� : �ٷ� �̾�� �ڵ� ���� (ó���� �����Ͱ� �ִٴ� ���̹Ƿ� lock������ ä ����)
		//  -> �Ҹ��� �� : Lock�� Ǯ�� ��� ���·� ��ȯ


		//event��� 	unique_lock<mutex> lock(m);
		//if (q.empty() == false) 
		{
			int data = q.front();
			q.pop();
			cout << data << '\n';
		}
	}
}

int main() {

	// Ŀ�� ������Ʈ
	// - Usage Count
	// - Signal (�Ķ���) / Non-Signal (������)

	//bool -> Auto/Manual
	hEvent = ::CreateEvent(NULL/*���ȼӼ�*/, FALSE/*bManualReset*/, FALSE/*�ʱ����*/, NULL);


	thread t1(Producer);
	thread t2(Consumer);

	t1.join();
	t2.join();

	::CloseHandle(hEvent);
}