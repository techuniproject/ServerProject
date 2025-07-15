#include <iostream>
#include <string>
#include <format>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main() 
{
    // Winsock �ʱ�ȭ
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup ����" << endl;
        return 1;
    }

    // ���� ����
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "���� ���� ����: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // ���� �ּ� ����
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345); // ������ ������ ��Ʈ ���

    // ���� IP �ּ� ���� (�� ���������� ����ȣ��Ʈ)
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        cerr << "�߸��� ���� �ּ�" << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // ������ ����
    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "���� ���� ����: " << WSAGetLastError() << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    cout << "������ ����Ǿ����ϴ�." << endl;

    // �޽��� �ۼ���
    const int bufferSize = 1024;
    char buffer[bufferSize];
    string message;

    while (true) {
        // ����ڷκ��� �޽��� �Է� �ޱ�
        cout << "������ ���� �޽��� (�����Ϸ��� 'exit' �Է�): ";
        getline(cin, message);

        if (message == "exit") {
            break;
        }

        // �޽��� ����
        if (send(clientSocket, message.c_str(), (int)message.length(), 0) == SOCKET_ERROR) {
            cerr << "���� ����: " << WSAGetLastError() << endl;
            break;
        }

        // ���� ����
        int bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
        if (bytesReceived <= 0) {
            if (bytesReceived == 0) {
                cout << "���� ���� ����" << endl;
            }
            else {
                cerr << "���� ����: " << WSAGetLastError() << endl;
            }
            break;
        }

        // ���ŵ� ���� ó��
        buffer[bytesReceived] = '\0';
        cout << format("�����κ��� ����: {}\n", buffer);
    }

    // ���� �ݱ�
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}