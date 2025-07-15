#include <iostream>
#include <string>
#include "IOCPServer.h"

int main() {
    IOCPServer server;

    std::string port = "8080";
    int workerThreadCount = 4;

    std::cout << "IOCP Echo ���� ����\n";
    std::cout << "��Ʈ: " << port << "\n";
    std::cout << "��Ŀ ������ ��: " << workerThreadCount << "\n\n";

    // ���� �ʱ�ȭ
    if (!server.Initialize(port, workerThreadCount)) {
        std::cerr << "���� �ʱ�ȭ ����\n";
        return -1;
    }

    // ���� ����
    server.Start();

    // ����� �Է� ��� (���� ���Ḧ ����)
    std::cout << "���� ���Ḧ ���Ͻø� 'q'�� �Է��ϰ� Enter�� ��������.\n";
    std::string input;
    while (true) {
        std::getline(std::cin, input);
        if (input == "q" || input == "Q") {
            break;
        }
        std::cout << "���� ���Ḧ ���Ͻø� 'q'�� �Է��ϰ� Enter�� ��������.\n";
    }

    // ���� ����
    std::cout << "������ �����մϴ�...\n";
    server.Stop();
    std::cout << "���� ���� �Ϸ�\n";

    return 0;
}