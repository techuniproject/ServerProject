#include "Defines.h"
#include "MixedLengthClient.h"


int main() {
    // �ѱ� ����� ���� ����
    SetConsoleOutputCP(CP_UTF8);

    
    MixedLengthClient client;
    std::string serverIP;
    uint32_t userId, targetId;

    std::cout << "���� IP�� �Է��ϼ���: ";
    std::cin.ignore();
    std::getline(std::cin, serverIP);

    std::cout << "����� ����� ID�� �Է��ϼ���: ";
    std::cin >> userId;

    if (client.Connect(serverIP, 27015, userId)) {
        while (true) {
            std::cout << "\n1: �ŷ� ��û ������, 2: ���� - ����: ";
            int choice;
            std::cin >> choice;

            if (choice == 2) {
                break;
            }
            else if (choice == 1) {
                std::cout << "�ŷ��� ��� ID�� �Է��ϼ���: ";
                std::cin >> targetId;

                // ������ ��� ����
                std::vector<Item> items;
                int itemCount;
                std::cout << "���� ������ ���� �Է��ϼ���: ";
                std::cin >> itemCount;

                for (int i = 0; i < itemCount; i++) {
                    Item item;
                    std::cout << std::format("������ #{}:\n", i + 1);
                    std::cout << "������ ID: ";
                    std::cin >> item.itemId;
                    std::cout << "����: ";
                    std::cin >> item.quantity;
                    std::cout << "ī�װ�(0-����, 1-��, 2-�Ҹ�ǰ): ";
                    std::cin >> item.category;

                    items.push_back(item);
                }

                std::string message;
                std::cout << "�ŷ� �޽����� �Է��ϼ���: ";
                std::cin.ignore();
                std::getline(std::cin, message);

                client.SendTradeRequest(targetId, items, message);
            }
        }

        client.Disconnect();
        
    }

    return 0;
}
