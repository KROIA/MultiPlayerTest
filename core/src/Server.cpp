#include "Server.h"
#include "ServerClient.h"

namespace Game
{

	Log::LogObject& Server::getLogger()
	{
		static Log::LogObject logger("Server");
		return logger;
	}
	Server::Server()
	{


	}
	Server::~Server()
	{
		stop();
	}

	bool Server::start(unsigned short port)
	{
		m_threadRunning = true;
		m_listener.setBlocking(false);
		sf::Socket::Status status = m_listener.listen(port);
		switch (status)
		{
			case sf::Socket::Status::Done:
			{
				getLogger().logInfo("Server started on port: " + std::to_string(port));
				m_thread = std::thread(&Server::handleServer, this);
				return true;
				break;
			}
			case sf::Socket::Status::Error:
			{
				getLogger().logError("Failed to start server on port: " + std::to_string(port));
				return false;
				break;
			}
		}
		return false;
	}
	void Server::stop()
	{
		m_threadRunning = false;
		m_thread.join();
		m_listener.close();
		for (sf::TcpSocket* client : m_clients)
		{
			delete client;
		}
		m_clients.clear();
	}

	void Server::update()
	{
		std::vector<std::pair<sf::TcpSocket*, sf::Packet>> received = getReceivedPackets();
		std::vector<std::pair<sf::TcpSocket*, sf::Packet>> sending;
		sending.reserve(received.size());
		for (auto& pair : received)
		{
			sf::TcpSocket* client = pair.first;
			sf::Packet& packet = pair.second;

			sf::Packet response;
			
			std::string name;
			int command;
			packet >> name >> command;
			sf::Packet subPacket;
			ServerClient::extractNextPacket(packet, subPacket);
			
			int responseCommand = -1;
			if (processPacket(client, name, command, subPacket, response, responseCommand))
			{
				if (response.getDataSize() > 0)
				{
					sf::Packet packet;
					packet << name << responseCommand;
					ServerClient::appendPacketWithSize(packet, response);
					sending.emplace_back(std::pair<sf::TcpSocket*, sf::Packet>{client, std::move(packet)});
				}
			}
			
		}
		setSendingPackets(sending);
	}
	bool Server::processPacket(sf::TcpSocket* client, const std::string& name, int command, sf::Packet& packet, sf::Packet& response, int& responseCommand)
	{
		getLogger().logInfo("Received packet from client: " + name);
		return false;
	}

	std::vector<std::pair<sf::TcpSocket*, sf::Packet>> Server::getReceivedPackets()
	{
		std::vector<std::pair<sf::TcpSocket*, sf::Packet>> received;
		if (m_hasPacketReceived)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			received = std::move(m_received);
			m_received.clear();
			m_received.reserve(10);
			m_hasPacketReceived = false;
		}
		return received;
	}
	void Server::setSendingPackets(const std::vector<std::pair<sf::TcpSocket*, sf::Packet>>& packets)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_sending = packets;
		m_hasPacketToSend = true;
	}
	void Server::sendPacket(sf::TcpSocket* client, const std::string& name, int command, sf::Packet& packet)
	{
		sf::Packet packet2;
		packet2 << name << command;
		ServerClient::appendPacketWithSize(packet2, packet);
		std::unique_lock<std::mutex> lock(m_mutex);
		m_sending.emplace_back(std::pair<sf::TcpSocket*, sf::Packet>{client, std::move(packet2)});
		m_hasPacketToSend = true;
	}


	void Server::handleServer()
	{
		sf::TcpSocket* newClient = nullptr;
		while (m_threadRunning)
		{
			// Check for new clients
			if (!newClient)
			{
				newClient = new sf::TcpSocket;
				newClient->setBlocking(false);
			}
			if (m_listener.accept(*newClient) == sf::Socket::Done)
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				m_clients.push_back(newClient);
				getLogger().logInfo("New client connected.");
				newClient = nullptr;
			}

			// Check for incoming data from clients
			for (int i=0; i<(int)m_clients.size(); ++i)
			{
				sf::TcpSocket& client = *m_clients[i];
				if (!receivePackets(client))
				{
					// Disconnect the client if there's an error or if the client disconnected
					getLogger().logInfo("Client disconnected.");
					delete m_clients[i];
					auto it = m_clients.begin() + i;
					m_clients.erase(it);
					i--;
				}
			}

			if (m_hasPacketToSend)
			{
				std::vector<std::pair<sf::TcpSocket*, sf::Packet>> sending;
				{
					std::unique_lock<std::mutex> lock(m_mutex);
					sending = std::move(m_sending);
					m_sending.clear();
					m_sending.reserve(10);
					m_hasPacketToSend = false;
				}
				for (auto& pair : sending)
				{
					sf::TcpSocket& client = *pair.first;
					sf::Packet& packet = pair.second;
					sf::Socket::Status status = client.send(packet);
					switch (status)
					{
						case sf::Socket::Status::Done:
						{
							//getLogger().logInfo("Sent packet to client");
							break;
						}
						case sf::Socket::Disconnected:
						{
							break;
						}
					}
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		getLogger().logInfo("Server thread stopped.");
	}
	bool Server::receivePackets(sf::TcpSocket& client)
	{
		sf::Packet packet;
		std::vector<std::pair<sf::TcpSocket*, sf::Packet>> received;
		received.reserve(10);

		sf::Socket::Status status; 
		do {
			status = client.receive(packet);
			switch (status)
			{
				case sf::Socket::Status::Done:
				{
					received.emplace_back(std::pair<sf::TcpSocket*, sf::Packet>{ &client, std::move(packet) });
					//getLogger().logInfo("Received packet from server");
					break;
				}
				case sf::Socket::Disconnected:
				{
					//getLogger().logInfo("Server disconnected");
					return false;
				}
			}
		} while (status == sf::Socket::Status::Done);

		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_received.insert(m_received.end(), received.begin(), received.end());
			m_hasPacketReceived = true;
		}

		return true;
	}

}