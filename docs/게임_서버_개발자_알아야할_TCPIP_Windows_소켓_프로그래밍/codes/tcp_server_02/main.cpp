#include <iostream>
#include <string>
#include <format>
#include <thread>
#include <vector>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class TCPServer {
private:
    SOCKET listenSocket;
    std::vector<std::thread> clientThreads;
    bool running;

    static constexpr int BUFFER_SIZE = 1024;
    static constexpr int DEFAULT_PORT = 27015;

public:
    TCPServer() : listenSocket(INVALID_SOCKET), running(false) {}

    ~TCPServer() {
        Stop();
    }

    bool Start(int port = DEFAULT_PORT) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << std::format("WSAStartup ����: {}\n", result);
            return false;
        }

        // ���� ����
        listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << std::format("���� ���� ����: {}\n", WSAGetLastError());
            WSACleanup();
            return false;
        }

        // ���� �ּ� ����
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = INADDR_ANY;  // ��� �������̽����� ���� ���

        // ���� ���ε�
        result = bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
        if (result == SOCKET_ERROR) {
            std::cerr << std::format("���ε� ����: {}\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return false;
        }

        // ���� ��� ����
        result = listen(listenSocket, SOMAXCONN);
        if (result == SOCKET_ERROR) {
            std::cerr << std::format("���� ����: {}\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return false;
        }

        running = true;
        std::cout << std::format("TCP ������ ��Ʈ {}���� ���۵Ǿ����ϴ�.\n", port);

        // Ŭ���̾�Ʈ ���� ���� ������ ����
        std::thread acceptThread(&TCPServer::AcceptClients, this);
        acceptThread.detach();

        return true;
    }

    void Stop() {
        running = false;

        if (listenSocket != INVALID_SOCKET) {
            closesocket(listenSocket);
            listenSocket = INVALID_SOCKET;
        }

        for (auto& thread : clientThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        clientThreads.clear();
        WSACleanup();
        std::cout << "TCP ������ �����Ǿ����ϴ�.\n";
    }

private:
    void AcceptClients() {
        while (running) {
            // Ŭ���̾�Ʈ ���� ����
            sockaddr_in clientAddr;
            int clientAddrLen = sizeof(clientAddr);

            SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
            if (clientSocket == INVALID_SOCKET) {
                if (running) {
                    std::cerr << std::format("Ŭ���̾�Ʈ ���� ���� ����: {}\n", WSAGetLastError());
                }
                continue;
            }

            // Ŭ���̾�Ʈ IP �ּ� ���
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
            std::cout << std::format("�� Ŭ���̾�Ʈ ����: {}:{}\n", clientIP, ntohs(clientAddr.sin_port));

            // Ŭ���̾�Ʈ ó�� ������ ����
            clientThreads.emplace_back(&TCPServer::HandleClient, this, clientSocket, std::string(clientIP));
        }
    }

    void HandleClient(SOCKET clientSocket, std::string clientIP) {
        char buffer[BUFFER_SIZE];

        while (running) {
            // ������ ����
            int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
            if (bytesReceived <= 0) {
                if (bytesReceived == 0) {
                    std::cout << std::format("Ŭ���̾�Ʈ {}�� ������ �����߽��ϴ�.\n", clientIP);
                }
                else {
                    std::cerr << std::format("recv ����: {}\n", WSAGetLastError());
                }
                break;
            }

            // ���ŵ� ������ ó��
            buffer[bytesReceived] = '\0';
            std::cout << std::format("{}�κ��� ����: {}\n", clientIP, buffer);

            // Ŭ���̾�Ʈ���� ���� ����
            std::string response = std::format("���� ����: {}", buffer);
            int bytesSent = send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
            if (bytesSent == SOCKET_ERROR) {
                std::cerr << std::format("send ����: {}\n", WSAGetLastError());
                break;
            }
        }

        // Ŭ���̾�Ʈ ���� �ݱ�
        closesocket(clientSocket);
    }
};


int main() 
{
    // �ѱ� ����� ���� ����
    SetConsoleOutputCP(CP_UTF8);

    TCPServer server;
    if (server.Start()) {
        std::cout << "������ �����Ϸ��� �ƹ� Ű�� ��������...\n";
        std::cin.get();
        server.Stop();
    }

    return 0;
}
