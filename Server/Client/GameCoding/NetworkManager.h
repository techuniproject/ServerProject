#pragma once

class NetworkManager
{
public:
	void Init();
	void Update();

	shared_ptr<class ServerSession> CreateSession();
	void SendPacket(shared_ptr<SendBuffer> sendBuffer);
private:
	shared_ptr<ClientService> _service;
	shared_ptr<ServerSession> _session;
};

