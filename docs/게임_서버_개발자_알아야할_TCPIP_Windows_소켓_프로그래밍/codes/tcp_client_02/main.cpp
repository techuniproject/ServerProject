#include <iostream>
#include <string>
#include <format>
#include <thread>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class TCPClient {
private:
    SOCKET clientSocket;
    bool connected;
    std::thread receiveThread;

    static constexpr int BUFFER_SIZE = 1024;
    static constexpr int DEFAULT_PORT = 27015;

public:
    TCPClient() : clientSocket(INVALID_SOCKET), connected(false) {}

    ~TCPClient() {
        Disconnect();
    }

    bool Connect(const std::string& serverIP, int port = DEFAULT_PORT) {
        // Winsock �ʱ�ȭ
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << std::format("WSAStartup ����: {}\n", result);
            return false;
        }

        // ���� ����
        clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << std::format("���� ���� ����: {}\n", WSAGetLastError());
            WSACleanup();
            return false;
        }

        // ���� �ּ� ����
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

        // ������ ����
        result = connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
        if (result == SOCKET_ERROR) {
            std::cerr << std::format("���� ���� ����: {}\n", WSAGetLastError());
            closesocket(clientSocket);
            WSACleanup();
            return false;
        }

        connected = true;
        std::cout << std::format("{}:{}�� ����Ǿ����ϴ�.\n", serverIP, port);

        // ���� ������ ����
        receiveThread = std::thread(&TCPClient::ReceiveMessages, this);

        return true;
    }

    void Disconnect() {
        connected = false;

        if (clientSocket != INVALID_SOCKET) {
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
        }

        if (receiveThread.joinable()) {
            receiveThread.join();
        }

        WSACleanup();
        std::cout << "�������� ������ ����Ǿ����ϴ�.\n";
    }

    bool SendMessage(const std::string& message) {
        if (!connected || clientSocket == INVALID_SOCKET) {
            std::cerr << "������� �ʾҽ��ϴ�.\n";
            return false;
        }

        int bytesSent = send(clientSocket, message.c_str(), static_cast<int>(message.length()), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << std::format("�޽��� ���� ����: {}\n", WSAGetLastError());
            return false;
        }

        return true;
    }

private:
    void ReceiveMessages() {
        char buffer[BUFFER_SIZE];

        while (connected) {
            // ������ ����
            int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
            if (bytesReceived <= 0) {
                if (bytesReceived == 0) {
                    std::cout << "������ ������ �����߽��ϴ�.\n";
                }
                else {
                    std::cerr << std::format("recv ����: {}\n", WSAGetLastError());
                }
                connected = false;
                break;
            }

            // ���ŵ� ������ ó��
            buffer[bytesReceived] = '\0';
            std::cout << "�����κ��� ����: " << buffer << std::endl;
        }
    }
};


int main() 
{
    // �ѱ� ����� ���� ����
    SetConsoleOutputCP(CP_UTF8);

    TCPClient client;
    std::string serverIP;

    std::cout << "���� IP�� �Է��ϼ��� (localhost�� 127.0.0.1): ";
    std::getline(std::cin, serverIP);

    if (client.Connect(serverIP)) {
        std::string message;
        while (true) {
            std::cout << "������ �޽��� (����: exit): ";
            std::getline(std::cin, message);

            if (message == "exit") {
                break;
            }

            client.SendMessage(message);
        }

        client.Disconnect();
    }

    return 0;
}