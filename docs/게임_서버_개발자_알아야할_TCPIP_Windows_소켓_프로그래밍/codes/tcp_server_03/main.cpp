#include <iostream>
#include <string>
#include <format>
#include <thread>
#include <vector>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class TCPServerIPv6 {
private:
    SOCKET listenSocket;
    std::vector<std::thread> clientThreads;
    bool running;
    
    static constexpr int BUFFER_SIZE = 1024;
    static constexpr int DEFAULT_PORT = 27015;

public:
    TCPServerIPv6() : listenSocket(INVALID_SOCKET), running(false) {}
    
    ~TCPServerIPv6() {
        Stop();
    }
    
    bool Start(int port = DEFAULT_PORT) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << std::format("WSAStartup ����: {}\n", result);
            return false;
        }
        
        // IPv6 ���� ����
        listenSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << std::format("���� ���� ����: {}\n", WSAGetLastError());
            WSACleanup();
            return false;
        }
        
        // IPv4 ���ε� IPv6 �ּ� ��� (��� ����)
        int ipv6Only = 0; // 0: ��� ����(IPv4+IPv6), 1: IPv6��
        result = setsockopt(listenSocket, IPPROTO_IPV6, IPV6_V6ONLY, 
                          reinterpret_cast<char*>(&ipv6Only), sizeof(ipv6Only));
        if (result == SOCKET_ERROR) {
            std::cerr << std::format("IPv6 �ɼ� ���� ����: {}\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return false;
        }
        
        // ���� �ּ� ���� (IPv6)
        sockaddr_in6 serverAddr;
        ZeroMemory(&serverAddr, sizeof(serverAddr));
        serverAddr.sin6_family = AF_INET6;
        serverAddr.sin6_port = htons(port);
        serverAddr.sin6_addr = in6addr_any; // ��� IPv6 �������̽����� ���� ���
        
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
        std::cout << std::format("TCP IPv6 ������ ��Ʈ {}���� ���۵Ǿ����ϴ�.\n", port);
        
        // Ŭ���̾�Ʈ ���� ���� ������ ����
        std::thread acceptThread(&TCPServerIPv6::AcceptClients, this);
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
        std::cout << "TCP IPv6 ������ �����Ǿ����ϴ�.\n";
    }
    
private:
    void AcceptClients() {
        while (running) {
            // Ŭ���̾�Ʈ ���� ����
            sockaddr_in6 clientAddr;
            int clientAddrLen = sizeof(clientAddr);
            
            SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
            if (clientSocket == INVALID_SOCKET) {
                if (running) {
                    std::cerr << std::format("Ŭ���̾�Ʈ ���� ���� ����: {}\n", WSAGetLastError());
                }
                continue;
            }
            
            // Ŭ���̾�Ʈ IP �ּ� ���
            char clientIP[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &clientAddr.sin6_addr, clientIP, INET6_ADDRSTRLEN);
            std::cout << std::format("�� Ŭ���̾�Ʈ ����: [{}]:{}\n", clientIP, ntohs(clientAddr.sin6_port));
            
            // Ŭ���̾�Ʈ ó�� ������ ����
            clientThreads.emplace_back(&TCPServerIPv6::HandleClient, this, clientSocket, std::string(clientIP));
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
                } else {
                    std::cerr << std::format("recv ����: {}\n", WSAGetLastError());
                }
                break;
            }
            
            // ���ŵ� ������ ó��
            buffer[bytesReceived] = '\0';
            std::cout << std::format("{}�κ��� ����: {}\n", clientIP, buffer);
            
            // Ŭ���̾�Ʈ���� ���� ����
            std::string response = std::format("IPv6 ���� ����: {}", buffer);
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
    
    TCPServerIPv6 server;
    if (server.Start()) {
        std::cout << "������ �����Ϸ��� �ƹ� Ű�� ��������...\n";
        std::cin.get();
        server.Stop();
    }
    
    return 0;
}