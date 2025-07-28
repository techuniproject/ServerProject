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
// Overlapped + Callback ���


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
/*========�̰� �ٽ� ����=======
static_cast<Session*>(overlapped)
-> ������ Ÿ�ӿ� Ÿ�� ������ �˻縦 ����
��, �̰� overlapped�� Session���� �Ļ��� Ŭ������ ���� ��ȿ��
ex) Base* -> Derived*ó��
�׷��Ƿ� �� ��� static_cast�� ������ ������

reinterpret_cast<Session*>(overlapped)
-> Ÿ�� ������� �����͸� ������ ��ȯ
-> C��� ���� ĳ���ð� ��� ������ �������

*/

const int32 BUF_SIZE = 1000;

struct Session
{
	WSAOVERLAPPED overlapped = {};
	//***�������� ������ Session ����ü ù��° ���ڷ� ��ġ
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUF_SIZE] = {};
	int32 recvBytes = 0;
};
/*
�츮�� wsaBuf�� ��� �����͸� ä�� �������� �˷��ְ�, overlapped�� �۾� ���¸� ��� �ִ� ����ü �����͸� �ѱ�
�׸��� �ݹ鵵 ���� �Ѱ� Ŀ���� �츮�� �Ѱ��� WSAOVERLAPPED�����͸� �����س��� �ִٰ� ��ȯ
�츮�� �� �����͸� �����Ͽ� �ٸ� ����� �����Ϸ��� WSAOVERLAPPED�� ù ����������� �ٸ� ����� �����ּ� ������� ���ٰ���
���� overlapped�� �ι�° ���ڸ� Session*�� ĳ�����ϸ� �ش� �ּҷκ��� Sessionũ�⸸ŭ �����ϴ� �������̹Ƿ� ��ȿ���� ����
*/

void CALLBACK RecvCallback(DWORD error, DWORD recvLen, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	cout << "Data Recv Len Callback = " << recvLen << endl;
	//TODO

	// ���⼭ recvBuffer�� �����͸� �˷��� overlapped�� �˾ƾ��ϴµ�,
	// overlapped�� ĳ������ �Ϸ��� overlapped�� ù ���ڷ� �;� �����ͷ� ĳ���� �� �� �ֱ⶧��
	Session* session = (Session*)overlapped;
	cout << session->recvBuffer << endl;
	// �̷��� �ϸ� 

}

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

		cout << "Client Connected" << endl;

		while (true)
		{
			WSABUF wsaBuf;
			wsaBuf.buf = session.recvBuffer;
			wsaBuf.len = BUF_SIZE;

			DWORD recvLen = 0;
			DWORD flags = 0;

			// �Ϸ� ������ �Լ������ͷ� �־��� �Լ��� ȣ���ϱ⸦ ������ ����
			if (::WSARecv(clientSocket, &wsaBuf, 1,/*Out*/&recvLen,/*Out*/ &flags, &session.overlapped, RecvCallback) == SOCKET_ERROR)
			{
				if (::WSAGetLastError() == WSA_IO_PENDING)
				{
					//TODO
					//...
				   // Alertable Wait
				   // �ῡ ���������, �ݹ��Լ��� ȣ��Ǵ� ��Ȳ���� ����鼭 �ش� �Լ� ȣ��
				   // ������� �Ϸ�Ǹ� Ŀ���� ���ͷ�Ʈ�� �߻��ϰ� ������� �ƾ�� ����� �ݹ� �Լ� ȣ��
					::SleepEx(INFINITE, TRUE);
					// �ι�° ���ڰ� TRUE���� Alertable ���°� �Ǿ� IO�Ϸ�� �ݹ��� ȣ���
				}
				else
				{// ����� ������ ������ �ִ� ��
					//TODO
					break;
				}
			}
			cout << "Data Recv = " << session.recvBuffer << endl;
		}

	}

	SocketUtils::Clear();
}