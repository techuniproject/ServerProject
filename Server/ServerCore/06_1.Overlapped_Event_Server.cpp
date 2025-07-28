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

// Overlapped I/O (�񵿱� + ����ŷ)
// Overlapped + Event ���
// I/O ��û ��������(WSARecv) �񵿱�/ ����ŷ
// I/O �Ϸ� ��� ���� (WSAWaitForMultipleEvents) ���������� ���ŷ

/*
�߰� ����

�ý��� ȣ�� ��
-> I/O �۾��� ��û�ϴ� �ý��� ȣ�� ��(����ڰ� Ŀ�� ���� ���) ��Ȳ�� ���� ��� ���·� �ٲٰ� ���ؽ�Ʈ ����Ī
	�Ǵ� �� ���ŷ�� ��� CPU����(Ÿ�̸� ���ͷ�Ʈ ��)

- �Ϲ� ���� + recv() -> ���ŷ �Ǵ� �ý��� ��
- ����ŷ ���� + recv -> ����ŷ �Ǵ� �ý��� ��
- Overlapeed���� + WSARecv() -> �� ���ŷ�Ǵ� �ý��� ��

���ͷ�Ʈ
-> I/O ��ġ�κ��� I/O�Ϸᰡ �Ǿ� ���ͷ�Ʈ �߻��Ͽ� �ش� ������(��ϵȰ�)�� �غ� ť�� �ű�ų�
	�켱����(�����ٸ�)�� ���� ���ؽ�Ʈ ����Ī
-> Ÿ�� �����̽� �� ��� ��, Ÿ�̸� ���ͷ�Ʈ �߻��Ͽ� �غ� ����Ʈ�� �ְ� ���ؽ�Ʈ ����Ī

*/


const int32 BUF_SIZE = 1000;

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUF_SIZE] = {};
	int32 recvBytes = 0;
	// Overlapped �߰�
	WSAOVERLAPPED overlapped = {};
};


int main()
{
	SocketUtils::Init();

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return 0;

	//�� ���ŷ
	u_long on = 1;
	if (::ioctlsocket(listenSocket, FIONBIO, &on) == INVALID_SOCKET)
		return 0;

	SocketUtils::SetReuseAddress(listenSocket, true);

	if (SocketUtils::BindAnyAddress(listenSocket, 7777) == false)
		return 0;

	if (SocketUtils::Listen(listenSocket) == false)
		return 0;


	// Overlapped �Լ��� �Ǵ� (WSARecv, WSASend)
	// Overlapped �Լ��� �����ߴ��� Ȯ��
	// - ���������� ��� �� ó��
	// - ���������� ������ Ȯ��

	// [�񵿱�] I/O �۾� ��ü�� ��׶���� ����, I/O ��û �� ����� ���߿� �˾Ƽ� ���� ����
	// � �۾��� ��û�� ��, �� �۾��� �Ϸ�� ������ ��ٸ��� �ʰ�, �ٸ� �۾��� ��� ������ �� �ִ� ���
	//[I/O���� �񵿱�] : I/O�۾��� ��׶��忡�� Ŀ�ο� ���� ó���ǰ�, ���ø����̼��� �� �۾��� ����
	//                    ������ ��ٸ� �ʿ� ���� �ٸ� ���� �� �� ������, �۾� �Ϸ�� Ŀ���� �˷���.


	// WSAEventSelect��İ� �ٸ���
	// WSAEventSelect�� Ŀ�ο� Ư�� ���Ͽ��� Ư�� ���� ����� �˷��޶�� ��û
	// Ŀ���� ��Ʈ��ũ �̺�Ʈ�� ��������� �˷���, ���� I/O�� ������ recv()/send() ȣ��� ���� �а� ��
	// Event ��������� event ������ Ŀ���� ���� ���޹ް� �� ���� ���� i/oó���� �ϹǷ�
	// Ŀ���� ������ �غ�Ȱ͸� �˷��ְ� ���ø����̼��� ���� ������ �ۼ����ϹǷ� ������ �񵿱�� �ƴ�
	// �񵿱ⰰ�� ��� �־� �̺�Ʈ ��� �񵿱�ε� �� / ������ �ۼ��� �������� �񵿱� �ƴ�

	// Overlapped + WSARecv , WSASend ����� I/O ��ü�� Ŀ�ο� �̸� ��û
	// Ŀ���� �Ϸ�Ǹ� �˾Ƽ� �뺸
	// WSARecv�� WSASend ȣ�� �� �Լ��� ��� ���ϵ� / ������ �ۼ����� OS Ŀ���� ���
	// I/O�� ������ Ŀ���� �̺�Ʈ ��ü �Ǵ� IOCP �Ǵ� Callback���� �Ϸ� �뺸


	while (true) {

		SOCKADDR_IN clientAddr;
		int32 addrLen = sizeof(clientAddr);

		SOCKET clientSocket;
		while (true) {
			clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

			if (clientSocket != INVALID_SOCKET)
				break;

			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			return 0;
		}

		//Overlapped ȸ���� �̺�Ʈ ���
		Session session = Session{ clientSocket };
		WSAEVENT wsaEvent = ::WSACreateEvent();
		session.overlapped.hEvent = wsaEvent;
		// session.overlapped�� WSARecv/WSASend�� ���� �񵿱� I/O ��û��,
		// �ش� ��û�� ���� ����, ���, �̺�Ʈ ���� �����ϴ� ������ ��
		// hEvent�� I/O�Ϸ� �� signal�� �ٲ��� �̺�Ʈ �ڵ�
		// I/O ��û���� ������ OVERLAPPED�� �ʿ���
		cout << "Client Connected" << endl;

		while (true)
		{
			WSABUF wsaBuf;
			wsaBuf.buf = session.recvBuffer;
			wsaBuf.len = BUF_SIZE;

			DWORD recvLen = 0;
			DWORD flags = 0;

			// WSARecv �ι�° ���� wsaBuf�� �츮�� ������ �ּҿ� �� ũ�⸸ŭ ��ȿ�� �޸𸮸� ä������
			// ���̶�� �ϴ� ���̹Ƿ� �߰��� �� �޸𸮰� �����Ǹ� �ȵ�.

			//WSARecv(�ý��� ��) ��ȯ�� 0�̸� I/O��� �Ϸ��, ERROR�̸� ������ ���д� �ƴ�-> ���� �ڵ带 Ȯ���ؾ���
			if (::WSARecv(clientSocket, &wsaBuf, 1,/*Out*/&recvLen,/*Out*/ &flags, &session.overlapped, nullptr) == SOCKET_ERROR)
			{//�� Recv�� ������ �� �Ϸ�� ������ ��� �˾ƿð������� ����
				if (::WSAGetLastError() == WSA_IO_PENDING)
				{// WSAGetLastError�� ���� �ֱٿ� ȣ��� Winsock�Լ� �ϳ��� ���� ���� �ڵ� ��ȯ( ��ȯ ���� TLS�� ������ ���� ����)
				 // ���� �ٷδ� ����� ���� , ��밡 ������ ����ϹǷ� �ٷ� ������ ����ϸ� ����� �ȵ���
				 //�׷��Ƿ� ���⼭ �ٸ��� �Ұ� �ϰ� ������ ��.


				 //TODO
				 //...
					//1) Ŀ���� I/O �Ϸ��ؼ� �̺�Ʈ�� signaled �Ǿ����� Ȯ��
					//  Event signaled���� ��ٸ�
					// �� ��ĵ� �ᱹ ��ٸ��� ���� cpu�Ҵ��� �ȵ� ���·� ��ϵǾ� ����ڰ� ����� ��Ű�� ��
					// recv�� ���� �ڵ����� ��ϵǴ� ������ �ƴ����� �ᱹ ����� �Ǳ���
					// ���� WSA_INFINITE�� �ƴ� Ÿ���� ���ϸ� Ÿ�̸� ť�� ��ϵ� Ÿ�̸Ӹ��ῡ ����
					// Ŀ���� ������� �����ٸ� ����
					::WSAWaitForMultipleEvents(1, &wsaEvent, TRUE, WSA_INFINITE, FALSE);

					//2) signal�� ���� ���� ������ ���� ����, ����Ʈ �� Ȯ��
					::WSAGetOverlappedResult(session.socket, &session.overlapped, &recvLen, FALSE, &flags);
					//wsarecv�� �̹� recv�޴� ���� ��ġ�� ���������Ƿ� ���⼭ �̺�Ʈ üũ�� ����� �ϷḦ üũ, �����ʹ� �̹� ����
					// ���� WSAEventSelect��Ŀ����� �츮�� IO�� �غ���� ����������, 
					// �� ����� �̹� �˾Ƽ� Ŀ���� ������� �Ϸ��ϹǷ� �츰 I/O�� �Ϸ�Ǿ������� �̺�Ʈ�� �˾ƿ��� ��
				}
				else
				{// ����� ������ ������ �ִ� ��
					//TODO
					break;
				}
			}
			//�츰 Ŀ���� �̺�Ʈ �Ϸ����� �˰� ����(������)�� ��� �������� �����
			//�˾Ƽ� �񵿱������� Ŀ���� ����� ó���� ������ �׷��ٰ� �׳� ���°� �ƴ� ���� �Ϸ�Ǿ����� Ȯ�� �� ���
			cout << "Data Recv = " << session.recvBuffer << endl;
		}
		::WSACloseEvent(wsaEvent);
	}

	SocketUtils::Clear();
}
