
class ProtoParser():
    def __init__(self, start_id, recv_prefix, send_prefix) -> None:
        self.recv_packet = []   # recv packet list
        self.send_packet = []   # send packet list
        self.total_packet = []  # total packet list
        self.start_id = start_id
        self.id = start_id
        self.recv_prefix = recv_prefix
        self.send_prefix = send_prefix
        
    def parse_proto(self, path):
        f = open(path, 'r')
        lines = f.readlines()
        
        for line in lines:
            if False == line.startswith("message"):
                continue
            # split �Լ��� �ƹ� �͵� ���� ������ ���� ���� �������� �ڸ���(\\n, \\r, \\t, \\f)
            # message �ڿ� �����ϴ� ���� ��Ŷ�� �̸��� �ȴ�
            # packet_name = line.split()[1].upper()
            packet_name = line.split()[1]
            # ���� ��Ŷ�� recv_packet ��������� �ְ�
            if packet_name.startswith(self.recv_prefix):
                self.recv_packet.append(Packet(packet_name, self.id))
            # �۽� ��Ŷ�� send_packet ��������� �ִ´�
            elif packet_name.startswith(self.send_prefix):
                self.send_packet.append(Packet(packet_name, self.id))
            else:
                continue
            
            # ��Ŷ ���̵� �ο��Ϸ��� ���� �˾ƾ� �ϹǷ� ���������� �ִ´�
            self.total_packet.append(Packet(packet_name, self.id))
            self.id += 1
        
        f.close()
        
class Packet:
    def __init__(self, name, id):
        self.name = name
        self.id = id