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
#include "SocketUtils.h"



/*
[Overlapped IOCP ���]

[1] CreateIoCompletionPort -> IOCP ť ���� �� ���� ���
[2] WSARecv / WSASend -> �񵿱� I/O ��û ����
[3] GetQueuedCompletionStatus -> I/O �Ϸ� ���� ���
[4] �ݹ� �ƴ� - ��Ŀ �����尡 �Ϸ� ���� ���� ó��


[��ü �帧]
1. IOCP �غ� �ܰ� ���� �ʱ�ȭ

	HANDLE iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

- IOCP ť ����
- ���� ���ϵ�� ��Ŀ �����带 �� ť�� �����ؼ� ���� I/O ó��

2. ��Ŀ ������ Ǯ ����

	for (int i = 0; i < numThreads; ++i)
		GThreadManager->Launch([=]() { WorkerThreadMain(iocpHandle); });

- **GetQueuedCompletionStatus()**�� ť ����ϴ� ��Ŀ ������ N�� ����
- �� ��Ŀ ������� I/O �Ϸ� �̺�Ʈ�� ť�� ���� ������ ���ŷ(waiting) ����
- �� ���� CPU�� �Ҹ����� ���� �� �ſ� ȿ����

3. Ŭ���̾�Ʈ ���� ���� �� IOCP ���

	CreateIoCompletionPort((HANDLE)clientSocket, iocpHandle, (ULONG_PTR)session, 0);

- ���ο� Ŭ���̾�Ʈ ������ IOCP ť�� ���
- completionKey�� Session*�� ��� �� I/O �Ϸ� �� �ĺ� �뵵

4. �񵿱� I/O ��û - WSARecv

	WSARecv(session->socket, ..., &overlappedEx->overlapped, NULL);

- �񵿱� recv() ��û ����
- **overlapped ����ü(OverlappedEx)**�� I/O �۾� ������ �����ؼ� �ѱ�
- �ݹ��� ������� ����(null) �� IOCP ����� �ݹ� ��� ť ��� ����

5. I/O �Ϸ� �� Ŀ���� IOCP ť�� �Ϸ� ���� ����

- ������ recv/send �� I/O �۾��� Ŀ�ο��� �Ϸ�Ǹ�
- Ŀ���� IOCP ť�� ���� ������ �����:
- completionKey: Session*
- OVERLAPPED*: OverlappedEx*
- bytesTransferred: ���۵� ����Ʈ ��
- �̰� Ŀ�� �������� �ڵ����� �߻�

6. ��Ŀ �����尡 ť���� �Ϸ� ������ ó��

	GetQueuedCompletionStatus(iocpHandle, &bytesTransferred, (PULONG_PTR)&session, (LPOVERLAPPED*)&overlapped, INFINITE);

- ��Ŀ �����尡 �Ϸ�� �۾��� ť���� ����
- session, overlapped ������ �̿��� ó�� ����
- ó�� �� �ʿ� �� ���� I/O ��û(WSARecv ��)�� �ٽ� �����

7. �ݺ� ����

- I/O ��û (WSARecv)
- �Ϸ�Ǹ� Ŀ���� IOCP ť�� ��� ���
- ��Ŀ �����尡 ������ ó��
- �ٽ� I/O ��û ���
- �� ������ ���� �������δ� "���ӵ� I/O"ó��,
���������δ� �񵿱� �̺�Ʈ ������� ���� ó�� ����



1) ������ ����� ���

- Ŀ���� I/O �Ϸ� ������ IOCP ť�� ����
- ť�� ����ϴ� ��Ŀ �����尡 ���� ó��
- N���� ��Ŀ ������ -> M���� I/Oó�� ���� (���� ����� �ѹ��� ���ν����常 �ݹ� �޴� ����)

2) ������ ���� ����

- ���� ��Ŀ �����尡 GQCS ���
- ��Ŀ �����忡�� ���� �б� ó��
- �츮�� ���� ��Ʈ�� (������ ����, ���� Ȯ�� ����)

3) �޸� ���� ���� �� ĳ���� ���

- OVERLAPPED* �� reinterpret_cast�Ͽ� ����ü ����
- IOCP�� �߰����� completionKey�� ���� �� �־� ������
- GetQueuedCompletionStatus()���� �ٷ� session�� �޾Ƽ� ���� ����

4) ���ؽ�Ʈ ����Ī ���

- ��Ŀ �����尡 �̹� ���� ���̹Ƿ� �ܼ� �Լ� ȣ��� ���ϰ� ����
- ���ʿ��� Ŀ�� <-> ����� ���� ��ȯ�� �پ�� �ݹ�, �̺�Ʈ ��Ŀ� ����


[�ٽ� ���� 7�� ����]
IOCP ť�� �񵿱� �۾� �Ϸ� ������ Ŀ���� �ְ�, �츮�� ������ ������.

WSARecv/WSASend ���� OVERLAPPED�� �ѱ�� Ŀ���� �Ϸ� �� IOCP�� ������ش�.

�Ϸ� ó���� �ݹ��� �ƴ�, ��Ŀ �����尡 GetQueuedCompletionStatus()�� ���� �޴´�.

completionKey�� OVERLAPPED*�� ���� �ʿ��� ��� ������ �ǵ��� �� �ִ�.

��Ŀ ������� �۾� ���� �� CPU�� ���� �Ҹ����� �ʰ� ����Ѵ�.

��õ ���� ���ϵ� ��Ŀ ������ 4~8���� ȿ�������� ó�� �����ϴ�.

IOCP�� ���� ����, MMO, ��� ��ſ� ����ȭ�� I/O ���̴�.

*/

/*
HANDLE�̳� SOCKET�� OS Ŀ���� ���ҽ��� �߻������� �����ϱ� ���� ���� �ڵ� ��
������ó�� �������� void*(HANDLE), uintptr_t(SOCKET) ���� ������ �ƴ�.

Ŀ�� ���ο����� �̸� �ε����� ������ ó�� ����ϰ�, �ܺο����� �аų� ���� ���� ���� OS API��
���ؼ� ����ؾ� ��.

*/





const int32 BUF_SIZE = 1000;

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUF_SIZE] = {};
	int32 recvBytes = 0;
};

enum IO_TYPE
{
	READ,
	WRITE,
	ACCEPT,
	CONNECT,
};

struct OverlappedEx
{
	// �� ������� �ڵ�(������) �ϳ��� ������ ������ �� ���� (�ܺ� �ڷᱸ�� ����)
	// ĳ�� ȿ���� ���� overlapped�� �ּ��ϳ��� buffer, socket�� ������ �޸� �����ϹǷ�
	// reinterpret_cast�� �ܼ� Ÿ�� �ؼ��̶� ����� ����
	// �޸� ũ�� �������� �̹� �Ҵ��س��� WSAOVERLAPPED�� �ּҷ� �ش� ���� ������ �ؼ��ϹǷ�
	// �޸� ũ�� ��뿡�� ū �ǹ̰�������, �̷� ���� ������ �ؼ��ϴ� �������� �ٸ� �ڷᱸ�� ���� �޸� ����
	WSAOVERLAPPED overlapped = {};
	int32 type = 0;
	//TODO

};

/*
WSAOVERLAPPED�� ������� ���� ���ϴ� ����
-> C++������ ����ü�� ù��° ����� �ּҰ� ����ü ��ü�� �ּҿ� �����ϴٰ� ������. ������ ����� �ٸ�

Base�� �ְ� Derived�� ������ Base������ Derived ������ �޸� ���̾ƿ� ������ �����Ϸ��� ����
�׻� Base�� �տ� �´ٴ� ������ ����.
���� Base* -> Derived*�� ������ reinterpret_cast�ؼ� Derived ����� �����ϸ� Base������ �������� �ʴ� ������ ���� ħ�� ����

���� ����϶� ���� �幰����, ���� ����� ��� ���� ���� ����.

�޸� ���̾ƿ��� �ٲٴ��� ��� ���ٿ��� ���� ���� ������
�����Ϸ��� ��� Ÿ���� ���̾ƿ��� �������� �˰��ְ�, ��� �� �ڵ����� �������� ��������
������ reinterpret_cast�� �츮�� ���� �ּ� �ǵ鿩�� ����ϴϱ� ������ ������ ����
*/

/*
	Session�� Ŭ���̾�Ʈ ������ ������
	-�׷��� ���ν����忡�� ���� �����Ҵ��� Ŭ��� ����ɶ�����

	Overlapped�� I/O��û ������ �����ǰ� ���� ����




*/


void WorkerThreadMain(HANDLE iocpHandle)
{
	while (true)
	{
		//TODO

		//GQCS
		DWORD bytesTransferred = 0;
		Session* session = nullptr;
		OverlappedEx* overlappedEx = nullptr;
		// ��Ŀ ������� I/O�Ϸᰡ ť�� ������ ���ŷ �Ǿ� CPU�Ҹ���ϰ�
		// sleep���¿� ����� ȿ������ ���·� ����ϸ� CPU ���ҽ� ���� �Ҹ� ����
		// �̺�Ʈ ��� wait�Լ��̹Ƿ� ������� wait���·� ��ȯ�ǰ� �ش� IOCP ť�� �۾� ����� Ŀ���� �̺�Ʈ �߻����� �����.
		bool ret = ::GetQueuedCompletionStatus(iocpHandle, &bytesTransferred, (ULONG_PTR*)&session, (LPOVERLAPPED*)&overlappedEx, INFINITE);
		// LPOVERLAPPED�� typedef struct *LPOVERLAPPED�ε� ���ǵǾ��־� OVERLAPPED*�� ��ü�Ȱ� LPOVERLAPPED�̹Ƿ� LPOVERLAPPED*�� OVERLAPPED*�� ����Ű�� ������
		//WSARecv�� �Ϸ�Ǹ� �� �ڵ尡 ���������� ����ɰ���
		// ����� �������� key�� overlapped ������ �־��� �� �־� �̸� �� Ȱ���Ͽ��� ��
		// CreateIoCompletionPort���� �־��� key�� WSARecv�� �־��� overlapped������ ������ 
		// GetQueuedCompletionStatus���� �ٽ� ULONG_PTR* session�� LPOVERLAPPEd* overlappedEx�� ��ȯ�ؼ� ���п����� ���
		// Overlapped�� ���ο� ����ü�� �츮�� ���ϴ� ������ �߰��Ͽ� �װ� ��ȯ�޾��� �� �پ��� ����â���� ���
		// �� �Լ� ���� IOCP ������ I/O �̺�Ʈ�� ����, ��Ŀ �����尡 ���� ������� ������ ���������� ����˴ϴ�.

		// SESSION�� CreateIoCompletionPort �� �Ѱ��� key�� �״�� ��ȯ
		// overlappedEx -> WSARecv()�� �ѱ� ������ �״�� ��ȯ
		// bytesTransferred -> Ŭ���̾�Ʈ�� ���� ���� ����Ʈ ��
		// ť���� �ش� �̺�Ʈ ���� �� �� �������� �̹� I/O�Ϸ�� ����
		// �׷��Ƿ� �츮�� �� session �� overlapped�� ���� �ٽ� I/O��û�� ���� �� ����.
		if (ret == false || bytesTransferred == 0)
			continue;


		cout << "Recv Data Len = " << bytesTransferred << endl;
		cout << "Recv Data IOCP = " << session->recvBuffer << endl;

		//Thread Safe�Ѱ�?
		// Recv�ϴ°ſ� ���ؼ� �츮�� WSARecv�� �ѹ� ������ �Ͽ����Ƿ�
		// ����� ��û�� �ϳ� ���� ����Ǿ� �ϳ��� �����常 ó���ϴ� ���� ����Ǿ� ������ �������ϴ�
		// ���� �ش� ó�� �ϷḦ �� �����尡 �ٽ� I/O��û�� �Ͽ� ���� �ݺ��ϴ� �����̴�.
		// GetQueuedCompletionStatus�� I/O �Ϸ� �˸� 1�Ǵ� �� �ϳ��� �����常 ���� ó���ϱ� ������ Safe��

		// �׸��� ���� Ŭ���̾�Ʈ�� ���ÿ� �����ϸ� ���ν����忡�� �̹� �� Ŭ�� ����
		// ���� ���� ���ۿ� overlapped�� Ȱ���Ͽ� �����ϸ� ��� io�����ϹǷ� ����������
		// ���� Ŭ�� ���� ó���� ������.

		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUF_SIZE;


		// �� �ڵ�� �� ������?
		// �츮�� ���������� �����尡 I/O�۾��� Ŭ��κ��� ó���ϱ� ���ؼ���
		// �ٽ� I/O ��û ������ �����߸� �ϱ� ������ (�ݺ��� ����)
		DWORD recvLen = 0;
		DWORD flags = 0;
		::WSARecv(session->socket, &wsaBuf, 1, &recvLen, &flags,/*�߿�!*/&overlappedEx->overlapped, NULL);



	}
}

int main()
{
	SocketUtils::Init();

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return 0;

	// iocp ������� ���� ��ϵǸ� �ڵ����� ����ŷ���� �Ǿ� ���� ����ŷ ���� ���ص���

	SocketUtils::SetReuseAddress(listenSocket, true);

	if (SocketUtils::BindAnyAddress(listenSocket, 7777) == false)
		return 0;

	if (SocketUtils::Listen(listenSocket) == false)
		return 0;

	HANDLE iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	// �۾��� ť�� ������ִ°� - CP

	//WorkerThreads
	for (int32 i = 0; i < 5; ++i)
		GThreadManager->Launch([=]() {WorkerThreadMain(iocpHandle); });

	vector<Session*> sessionManager;

	//MainThread - accept ���
	// ���� ���� ������ �帧)
	// Ŭ��� ��Ʈ��ũ ����� ���� connect�� �ۼ��ؾ��ϹǷ� ������ accept�� ���� Ŭ���� ������ ����
	// �� �ð� ���� ���ŷ ������� ������ ����� �� ����
	// ����, Ŭ���� ��û�� ������ �ش� Ŭ�� �������� 1:1�� ���������ϴ� ������ ����� �� ������ Ŭ�� ������ ���
	// ���� �װ� Iocp�� ����ϰ� ���� �� key�� ����Ͽ� �ش� ������ ����
	// ����, �ϳ��� ����� �۾��� ����Ͽ� �ʿ��� overlapped�� �����, ����� ��û ��������
	// �� ��Ŀ�����尡 ���Ŀ� �ش� ��ȯ ������ ������� �ٽ� �ش� Ŭ���� io�۾��� �ٷ��,
	// overlapped�� io�۾��� �Ϸ�� ���� ���� �����ϹǷ� ���������� ó���ϸ� ��� Ŭ����
	// ����� ���ѷ����� ���� connect��û�� ����� ������ ���� �� ���Ŀ� ���������� ��ũ �����带 ����
	// ���ôٹ������� ó������.
	// 
	while (true)
	{
		SOCKADDR_IN clientAddr;
		int32 addrLen = sizeof(clientAddr);

		// ���� ���ŷ ��� �Լ� accept�� ������, ���߿� �ٲܰ���
		// Ŭ�� connect���� ����ϹǷ� ���ѷ��� ���� �ȵ�
		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
			return 0;

		Session* session = new Session(); //���� ������ Ŭ�� ���� ������ ��� Session
		session->socket = clientSocket;
		sessionManager.push_back(session);

		cout << "Client Connected" << endl;

		//[KEY ���]!!!!
		::CreateIoCompletionPort(HANDLE(clientSocket), iocpHandle,/*key*/(ULONG_PTR)session, 0);
		//key�� ������ session�� �ּҰ� ����

		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUF_SIZE;

		OverlappedEx* overlappedEx = new OverlappedEx();
		overlappedEx->type = IO_TYPE::READ;

		DWORD recvLen = 0;
		DWORD flags = 0;
		// IO ������ �Ϸ�, Ŭ�� ������ ���������� ���������� �ƹ� �ϵ� ���Ͼ
		::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags,/*�߿�!*/&overlappedEx->overlapped, NULL);
		// ������ �츮�� clientSocket�� ť�� ����Ͽ����Ƿ� IO�� �Ϸ�Ǹ� ť�� �� �츮�� ����� �� ����
		// ť�� �� ���� �����ϱ� ���ؼ� ���� �̺�Ʈ�� �ݹ����� �˾ƿ�����, ���� ��Ŀ�����带 ��ġ�Ͽ� 
		// �� �����忡�� ó���ϵ����� WorkerThreadMain�Լ����� (�� ��Ŀ �����尡 �����ϴ°�)

		/*
		[���� Session�� �ߺ� WSARecv ��û ����]

		// ù ��° ��û
		WSARecv(s->socket, &buf, 1, &recvLen, &flags, &s->overlapped, NULL);

		// ���� �Ϸ���� �ʾҴµ�...

		// �� ��° ��û (���� Session, ���� ����, ���� overlapped��!)
		WSARecv(s->socket, &buf, 1, &recvLen, &flags, &s->overlapped, NULL);
		��, ���� ����(Session)�� ���� ���� �Ϸ���� ���� I/O�� ���� ���ε�
		�ٽ� WSARecv()�� ȣ���ؼ� �ߺ� ��û�� ������ ��Ȳ�Դϴ�.
		-> ������ recvBuffer	�� ���� I/O�� ���� �޸𸮿� �����͸� ���ÿ� ���� ��
		-> ������ OVERLAPPED ����ü	Ŀ���� ������ ����ü �ּҿ� �� ���� ����� ������ ä����� �浹 �߻�
		-> �Ϸ� ������ ���� �� ����	��� WSARecv�� � ����� ���� ���� ���� �Ұ�

		[����]
		-> �� WSARecv() ��û�� �ݵ�� ������ OVERLAPPED ����ü�� ����ؾ� ��
			-�Ϲ������δ� new OverlappedEx()�� ���� �����, �Ϸ� �� delete or pool�� ����

		-> Session���� recvPending ���¸� bool�� ����Ͽ� �ߺ� ȣ�� ����
			if (session->recvPending)
				return; // �̹� ��û ������

			session->recvPending = true;
			WSARecv(...);
		-> �Ϸ�� ��Ŀ �����忡���� ���� ��û�� �������� ����
	*/


	/*
	 [������ �޸� ��ȿ ����]
	 session�� overlapped ��δ� �����ڰ� ���� ���� �����ؾ� �ϸ�,
	�ϳ��� ���� �����Ǹ� �������� dangling pointer�� �Ǿ� �ſ� �����մϴ�.
	�츮�� �ᱹ ���� �����忡�� ���� ��, ���� �ٸ� �����忡�� �Ѱ��ֱ⶧���� ����ؼ� ��ȿ�� �ּҿ����ϴµ�,
	Ư�� �����忡�� �ش� ���ǿ� ���� �������� ó���Ͽ� �ش� �޸𸮸� �ǵ��̸� ����������
	�׷��Ƿ� �ش� �޸� ������ŭ�� ����Ǿ�� �ϹǷ� smartpointer ������ �����ؾ���.
	*/
	}



	SocketUtils::Clear();
}