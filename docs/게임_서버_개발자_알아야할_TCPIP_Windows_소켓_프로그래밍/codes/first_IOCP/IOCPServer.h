#pragma once

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
#include <vector>
#include <string>
#include <thread>
#include <memory>
#include <format>

// ���̺귯�� ��ũ
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

// ��� ����
constexpr int BUFFER_SIZE = 4096;

// IOCP ���� Ÿ�� ����
enum class IOType {
    ACCEPT,
    RECV,
    SEND
};

// IOCP���� ����� Ȯ�� OVERLAPPED ����ü
struct IOContext {
    OVERLAPPED overlapped;
    WSABUF wsaBuf;
    IOType ioType;
    SOCKET socket;
    char buffer[BUFFER_SIZE];

    IOContext(IOType type);
    ~IOContext();
};

// Ŭ���̾�Ʈ ���� ����ü
struct ClientInfo {
    SOCKET socket;
    SOCKADDR_IN clientAddr;
    std::shared_ptr<IOContext> recvContext;
    std::shared_ptr<IOContext> sendContext;

    ClientInfo();
    ~ClientInfo();
};

// IOCP ���� Ŭ����
class IOCPServer {
private:
    SOCKET listenSocket;
    HANDLE iocpHandle;
    std::vector<std::thread> workerThreads;
    bool isRunning;
    LPFN_ACCEPTEX lpfnAcceptEx;

public:
    IOCPServer();
    ~IOCPServer();

    bool Initialize(const std::string& port, int workerThreadCount);
    void Start();
    void Stop();

private:
    void WorkerThread();
    void PostAccept();
    void HandleAccept(IOContext* pIOContext);
    void PostRecv(ClientInfo* pClientInfo);
    void HandleRecv(ClientInfo* pClientInfo, DWORD bytesTransferred);
    void PostSend(ClientInfo* pClientInfo, DWORD bytesToSend);
    void HandleSend(ClientInfo* pClientInfo, DWORD bytesTransferred);
};
