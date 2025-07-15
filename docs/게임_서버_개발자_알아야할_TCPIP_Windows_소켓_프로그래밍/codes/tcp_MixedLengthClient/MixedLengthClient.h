#pragma once

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <format>
#include <vector>
#include <thread>
#include <string>
#include <map>

#pragma comment(lib, "ws2_32.lib")


class MixedLengthClient {
private:
    SOCKET clientSocket;
    bool connected;
    uint32_t userId;

public:
    MixedLengthClient() : clientSocket(INVALID_SOCKET), connected(false), userId(0) {}

    ~MixedLengthClient() {
        Disconnect();
    }

    bool Connect(const std::string& serverIP, int port = 27015, uint32_t userIdParam = 0) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup ����\n";
            return false;
        }

        clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << std::format("���� ���� ����: {}\n", WSAGetLastError());
            WSACleanup();
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

        if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << std::format("���� ���� ����: {}\n", WSAGetLastError());
            closesocket(clientSocket);
            WSACleanup();
            return false;
        }

        connected = true;
        userId = userIdParam;
        std::cout << std::format("���� {}:{}�� �����\n", serverIP, port);
        return true;
    }

    void Disconnect() {
        if (connected && clientSocket != INVALID_SOCKET) {
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
            connected = false;
            WSACleanup();
            std::cout << "������ ���� ����\n";
        }
    }

    bool SendTradeRequest(uint32_t targetId, const std::vector<Item>& items, const std::string& message) {
        if (!connected || clientSocket == INVALID_SOCKET) {
            std::cerr << "������ ������� ����\n";
            return false;
        }

        // 1. ��Ŷ ũ�� ���
        uint16_t headerSize = sizeof(PacketHeader);
        uint16_t tradeRequestSize = sizeof(TradeRequestPacket);
        uint16_t itemsSize = static_cast<uint16_t>(items.size() * sizeof(Item));
        uint16_t messageSize = static_cast<uint16_t>(message.length());
        uint16_t totalSize = headerSize + tradeRequestSize + itemsSize + messageSize;

        // 2. ��Ŷ ���� ����
        std::vector<char> packetBuffer(totalSize);
        char* bufferPtr = packetBuffer.data();

        // 3. ��� ����
        PacketHeader header;
        header.totalSize = totalSize;
        header.packetType = PT_TRADE_REQUEST;
        memcpy(bufferPtr, &header, headerSize);
        bufferPtr += headerSize;

        // 4. �ŷ� ��û ���� ����
        TradeRequestPacket tradeRequest;
        tradeRequest.requesterId = userId;
        tradeRequest.targetId = targetId;
        tradeRequest.itemCount = static_cast<uint16_t>(items.size());
        memcpy(bufferPtr, &tradeRequest, tradeRequestSize);
        bufferPtr += tradeRequestSize;

        // 5. ������ ���� ����
        for (const auto& item : items) {
            memcpy(bufferPtr, &item, sizeof(Item));
            bufferPtr += sizeof(Item);
        }

        // 6. �޽��� �߰� (�ִ� ���)
        if (!message.empty()) {
            memcpy(bufferPtr, message.c_str(), messageSize);
        }

        // 7. ��Ŷ ����
        int totalBytesSent = 0;
        while (totalBytesSent < totalSize) {
            int bytesSent = send(clientSocket,
                packetBuffer.data() + totalBytesSent,
                totalSize - totalBytesSent,
                0);

            if (bytesSent == SOCKET_ERROR) {
                std::cerr << std::format("�ŷ� ��û ���� ����: {}\n", WSAGetLastError());
                return false;
            }

            totalBytesSent += bytesSent;
        }

        std::cout << std::format("�÷��̾� ID {}���� �ŷ� ��û ���۵�. ������ {}��, �޽���: {}\n",
            targetId, items.size(), message);

        // 8. ���� ���� (���� �����峪 �ݹ����� ó���� �� ����)
        // ���⼭�� �����ϰ� ���������� ���� ��ٸ�
        PacketHeader responseHeader;
        int bytesReceived = recv(clientSocket, reinterpret_cast<char*>(&responseHeader), sizeof(responseHeader), 0);

        if (bytesReceived != sizeof(responseHeader)) {
            std::cerr << "���� ��� ���� ����\n";
            return false;
        }

        if (responseHeader.packetType != PT_TRADE_RESPONSE) {
            std::cerr << std::format("����ġ ���� ���� ����: {}\n", responseHeader.packetType);
            return false;
        }

        uint16_t responseDataSize = responseHeader.totalSize - sizeof(responseHeader);
        std::vector<char> responseBuffer(responseDataSize);

        bytesReceived = recv(clientSocket, responseBuffer.data(), responseDataSize, 0);
        if (bytesReceived != responseDataSize) {
            std::cerr << "���� ������ ���� ����\n";
            return false;
        }

        // ���� ������ �Ľ� (����)
        if (responseDataSize >= 9) {  // �ּ� ���� ũ�� (senderId(4) + targetId(4) + accepted(1))
            uint32_t responderId = *reinterpret_cast<uint32_t*>(responseBuffer.data());
            uint32_t respTargetId = *reinterpret_cast<uint32_t*>(responseBuffer.data() + 4);
            uint8_t accepted = responseBuffer[8];

            std::string responseMessage;
            if (responseDataSize > 9) {
                responseMessage = std::string(responseBuffer.data() + 9, responseDataSize - 9);
            }

            std::cout << std::format("�ŷ� ���� ����: �߽���={}, ������={}, ����={}, �޽���={}\n",
                responderId, respTargetId, accepted ? "��" : "�ƴϿ�", responseMessage);
        }

        return true;
    }
};