#include <iostream>
#include <string>
#include <format>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main() {
    // Winsock �ʱ�ȭ
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup ����" << endl;
        return 1;
    }

    // ���� ����
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "���� ���� ����: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // ���� �ּ� ����
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // ��� IP �ּ� ����
    serverAddr.sin_port = htons(12345); // ��Ʈ 12345 ���

    // ���� ���ε�
    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "���ε� ����: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // ������ ���·� ��ȯ
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "������ ����: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "������ ���۵Ǿ����ϴ�. Ŭ���̾�Ʈ�� ��ٸ��� ��..." << endl;

    // Ŭ���̾�Ʈ ���� ����
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    SOCKET clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);

    if (clientSocket == INVALID_SOCKET) {
        cerr << "Ŭ���̾�Ʈ ���� ���� ����: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Ŭ���̾�Ʈ IP ���
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    cout << format("Ŭ���̾�Ʈ �����: {}:{}\n", clientIP, ntohs(clientAddr.sin_port));

    // ������ ���� �� ����
    const int bufferSize = 1024;
    char buffer[bufferSize];

    while (true) {
        // ������ ����
        int bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
        if (bytesReceived <= 0) {
            if (bytesReceived == 0) {
                cout << "Ŭ���̾�Ʈ ���� ����" << endl;
            }
            else {
                cerr << "���� ����: " << WSAGetLastError() << endl;
            }
            break;
        }

        // ���ŵ� ������ ó��
        buffer[bytesReceived] = '\0';
        cout << format("Ŭ���̾�Ʈ�κ��� ����: {}\n", buffer);

        // ���� ����
        string response = "�޽��� ���� �Ϸ�: ";
        response += buffer;

        if (send(clientSocket, response.c_str(), (int)response.length(), 0) == SOCKET_ERROR) {
            cerr << "���� ����: " << WSAGetLastError() << endl;
            break;
        }
    }

    // ���� �ݱ�
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}