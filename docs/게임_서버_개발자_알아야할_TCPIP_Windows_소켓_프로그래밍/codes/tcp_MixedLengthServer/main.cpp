#include "Defines.h"
#include "MixedLengthServer.h"



int main() 
{
    // �ѱ� ����� ���� ����
    SetConsoleOutputCP(CP_UTF8);
        
    MixedLengthServer server;
    if (server.Start()) {
        std::cout << "������ ���۵Ǿ����ϴ�. �����Ϸ��� �ƹ� Ű�� ��������.\n";
        std::cin.ignore();
        std::cin.get();
        server.Stop();
    }
    

    return 0;
}
