#pragma once

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <format>
#include <vector>
#include <thread>
#include <string>
#include <map>

#include "Defines.h"

#pragma comment(lib, "ws2_32.lib")


class MixedLengthServer {
private:
    SOCKET listenSocket;
    std::vector<std::thread> clientThreads;
    bool running;

    // Ŭ���̾�Ʈ ID�� ���� ����
    std::map<uint32_t, SOCKET> clientSockets;

public:
    MixedLengthServer() : listenSocket(INVALID_SOCKET), running(false) {}

    ~MixedLengthServer() {
        Stop();
    }

    bool Start(int port = 27015) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup ����\n";
            return false;
        }

        listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << std::format("���� ���� ����: {}\n", WSAGetLastError());
            WSACleanup();
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << std::format("���ε� ����: {}\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return false;
        }

        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << std::format("���� ����: {}\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return false;
        }

        running = true;
        std::cout << std::format("ȥ�� ���� ������ ������ ��Ʈ {}���� ���۵�\n", port);

        // Ŭ���̾�Ʈ ���� ���� ������ ����
        std::thread acceptThread(&MixedLengthServer::AcceptClients, this);
        acceptThread.detach();

        return true;
    }

    void Stop() {
        running = false;

        if (listenSocket != INVALID_SOCKET) {
            closesocket(listenSocket);
            listenSocket = INVALID_SOCKET;
        }

        // ��� Ŭ���̾�Ʈ ���� �ݱ�
        for (const auto& [userId, socket] : clientSockets) {
            closesocket(socket);
        }
        clientSockets.clear();

        for (auto& thread : clientThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        clientThreads.clear();
        WSACleanup();
        std::cout << "������ ������\n";
    }

private:
    void AcceptClients() {
        while (running) {
            sockaddr_in clientAddr;
            int clientAddrLen = sizeof(clientAddr);

            SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
            if (clientSocket == INVALID_SOCKET) {
                if (running) {
                    std::cerr << std::format("Ŭ���̾�Ʈ ���� ���� ����: {}\n", WSAGetLastError());
                }
                continue;
            }

            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
            std::cout << std::format("�� Ŭ���̾�Ʈ ����: {}:{}\n", clientIP, ntohs(clientAddr.sin_port));

            // �ӽ� ����� ID ���� (���� ���ӿ����� �α��� �������� ó��)
            static uint32_t nextUserId = 1000;
            uint32_t userId = nextUserId++;

            // Ŭ���̾�Ʈ ���� ����
            clientSockets[userId] = clientSocket;

            // Ŭ���̾�Ʈ ó�� ������ ����
            clientThreads.emplace_back(&MixedLengthServer::HandleClient, this, clientSocket, userId);
        }
    }

    void HandleClient(SOCKET clientSocket, uint32_t userId) {
        std::cout << std::format("Ŭ���̾�Ʈ ID {}�� �����\n", userId);

        // ��Ŷ ó�� ����
        while (running) {
            // 1. ��Ŷ ��� ����
            PacketHeader header;
            int bytesReceived = recv(clientSocket, reinterpret_cast<char*>(&header), sizeof(header), 0);

            if (bytesReceived <= 0) {
                if (bytesReceived == 0) {
                    std::cout << std::format("Ŭ���̾�Ʈ ID {}�� ���� ����\n", userId);
                }
                else {
                    std::cerr << std::format("��� ���� ����: {}\n", WSAGetLastError());
                }
                break;
            }

            if (bytesReceived != sizeof(header)) {
                std::cerr << "�ҿ����� ��� ���ŵ�\n";
                break;
            }

            // 2. ��Ŷ ũ�� ����
            uint16_t totalSize = header.totalSize;
            uint16_t dataSize = totalSize - sizeof(header);

            if (totalSize < sizeof(header) || dataSize > 8192) {
                std::cerr << std::format("�߸��� ��Ŷ ũ��: {}\n", totalSize);
                break;
            }

            // 3. ��Ŷ ������ ����
            std::vector<char> packetData(dataSize);
            int totalBytesReceived = 0;

            while (totalBytesReceived < dataSize) {
                bytesReceived = recv(clientSocket,
                    packetData.data() + totalBytesReceived,
                    dataSize - totalBytesReceived,
                    0);

                if (bytesReceived <= 0) {
                    if (bytesReceived == 0) {
                        std::cout << "Ŭ���̾�Ʈ ���� ����\n";
                    }
                    else {
                        std::cerr << std::format("������ ���� ����: {}\n", WSAGetLastError());
                    }
                    closesocket(clientSocket);
                    clientSockets.erase(userId);
                    return;
                }

                totalBytesReceived += bytesReceived;
            }

            // 4. ��Ŷ ������ ���� ó��
            switch (header.packetType) {
            case PT_TRADE_REQUEST:
                HandleTradeRequest(clientSocket, userId, packetData.data(), dataSize);
                break;

            case PT_CHAT_MESSAGE:
                // ä�� �޽��� ó�� (���� ����)
                std::cout << "ä�� �޽��� ��Ŷ ���ŵ�\n";
                break;

            case PT_PLAYER_MOVE:
                // �÷��̾� �̵� ó�� (���� ����)
                std::cout << "�÷��̾� �̵� ��Ŷ ���ŵ�\n";
                break;

            default:
                std::cerr << std::format("�� �� ���� ��Ŷ ����: {}\n", header.packetType);
                break;
            }
        }

        // ���� ���� ó��
        closesocket(clientSocket);
        clientSockets.erase(userId);
    }

    void HandleTradeRequest(SOCKET clientSocket, uint32_t senderId, const char* data, uint16_t dataSize) {
        // �ŷ� ��û ��Ŷ �Ľ�
        if (dataSize < sizeof(TradeRequestPacket)) {
            std::cerr << "�ŷ� ��û ��Ŷ�� �ʹ� ����\n";
            return;
        }

        const TradeRequestPacket* tradeRequest = reinterpret_cast<const TradeRequestPacket*>(data);

        // ������ ����� ���� ��ġ
        const char* itemsData = data + sizeof(TradeRequestPacket);
        uint16_t itemsDataSize = sizeof(Item) * tradeRequest->itemCount;

        // ������ ������ ũ�� ����
        if (sizeof(TradeRequestPacket) + itemsDataSize > dataSize) {
            std::cerr << "������ �����Ͱ� ��Ŷ���� ŭ\n";
            return;
        }

        // ������ ��� �Ľ�
        std::vector<Item> items;
        for (uint16_t i = 0; i < tradeRequest->itemCount; i++) {
            const Item* item = reinterpret_cast<const Item*>(itemsData + i * sizeof(Item));
            items.push_back(*item);
        }

        // �߰� �޽��� �Ľ� (�ִ� ���)
        std::string message;
        if (sizeof(TradeRequestPacket) + itemsDataSize < dataSize) {
            const char* messageData = itemsData + itemsDataSize;
            uint16_t messageSize = dataSize - sizeof(TradeRequestPacket) - itemsDataSize;
            message = std::string(messageData, messageSize);
        }

        // �ŷ� ��û ���� ���
        std::cout << std::format("�ŷ� ��û ����: ��û��={}, �����={}, ������ ��={}\n",
            tradeRequest->requesterId, tradeRequest->targetId, tradeRequest->itemCount);

        for (const auto& item : items) {
            std::cout << std::format("  ������ ID: {}, ����: {}, ī�װ�: {}\n",
                item.itemId, item.quantity, item.category);
        }

        if (!message.empty()) {
            std::cout << std::format("  �޽���: {}\n", message);
        }

        // ��� Ŭ���̾�Ʈ���� �ŷ� ��û ���� (���� ���� ����)
        uint32_t targetId = tradeRequest->targetId;
        if (clientSockets.find(targetId) != clientSockets.end()) {
            std::cout << std::format("��� Ŭ���̾�Ʈ ID {}���� �ŷ� ��û ����\n", targetId);

            // ���⼭ ��� Ŭ���̾�Ʈ���� ��Ŷ ���� ���� ���� �ʿ�
        }
        else {
            std::cout << std::format("��� Ŭ���̾�Ʈ ID {}�� ����Ǿ� ���� ����\n", targetId);

            // ��û�ڿ��� ���� ���� ������
            SendTradeResponse(clientSocket, senderId, tradeRequest->requesterId, false, "��� �÷��̾ ���� ���� �ƴմϴ�.");
        }
    }

    void SendTradeResponse(SOCKET clientSocket, uint32_t senderId, uint32_t targetId, bool accepted, const std::string& message) {
        // 1. ���� ��Ŷ ��� �غ�
        PacketHeader header;
        header.packetType = PT_TRADE_RESPONSE;

        // 2. ���� ������ �غ�
        struct TradeResponseData {
            uint32_t senderId;
            uint32_t targetId;
            uint8_t accepted;
        };

        TradeResponseData responseData;
        responseData.senderId = senderId;
        responseData.targetId = targetId;
        responseData.accepted = accepted ? 1 : 0;

        // 3. ��ü ��Ŷ ũ�� ���
        uint16_t totalSize = sizeof(header) + sizeof(responseData) + static_cast<uint16_t>(message.length());
        header.totalSize = totalSize;

        // 4. ��Ŷ ���� ���� �� ������ ����
        std::vector<char> packetBuffer(totalSize);
        char* bufferPtr = packetBuffer.data();

        // ��� ����
        memcpy(bufferPtr, &header, sizeof(header));
        bufferPtr += sizeof(header);

        // ���� ������ ����
        memcpy(bufferPtr, &responseData, sizeof(responseData));
        bufferPtr += sizeof(responseData);

        // �޽��� ����
        memcpy(bufferPtr, message.c_str(), message.length());

        // 5. ��Ŷ ����
        int totalBytesSent = 0;
        while (totalBytesSent < totalSize) {
            int bytesSent = send(clientSocket,
                packetBuffer.data() + totalBytesSent,
                totalSize - totalBytesSent,
                0);

            if (bytesSent == SOCKET_ERROR) {
                std::cerr << std::format("�ŷ� ���� ���� ����: {}\n", WSAGetLastError());
                return;
            }

            totalBytesSent += bytesSent;
        }

        std::cout << std::format("Ŭ���̾�Ʈ ID {}���� �ŷ� ���� ���۵�. ����: {}\n", targetId, accepted ? "��" : "�ƴϿ�");
    }
};