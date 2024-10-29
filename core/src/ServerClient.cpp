#include "ServerClient.h"
#include "NetworkObject.h"

namespace Game
{
	ServerClient::ServerClient()
	{
		
	}

	ServerClient::~ServerClient()
	{
		if (isConnected_internal())
			return;
		disconnect_internal();
	}

	//ServerClient& ServerClient::getInstance()
	//{
	//	static ServerClient instance;
	//	return instance;
	//}

	void ServerClient::addListener(NetworkObject* listener)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_listeners[listener->getName()] = listener;
	}
	void ServerClient::removeListener(NetworkObject* listener)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_listeners.erase(listener->getName());
	}
	/*
	void ServerClient::updateListeners()
	{
		ServerClient& instance = ServerClient::getInstance();
		std::vector<sf::Packet> received;
		if(instance.m_hasPacketReceived)
		{
			std::unique_lock<std::mutex> lock(instance.m_mutex);
			received = std::move(instance.m_received);
			instance.m_received.clear();
			instance.m_received.reserve(10);
			instance.m_hasPacketReceived = false;
		}
		for (sf::Packet& packet : received)
		{
			std::string name;
			int command;
			packet >> name >> command;
			sf::Packet subPacket; 
			if (ServerClient::extractNextPacket(packet, subPacket))
			{
				auto it = instance.m_listeners.find(name);
				if (it == instance.m_listeners.end())
				{
					instance.m_logger.logError("Failed to find listener with name: " + name);
					continue;
				}
				NetworkObject* listener = it->second;
					listener->handlePacket(command, subPacket);
			}
		}
	}*/

	void ServerClient::connect(const sf::IpAddress& ip, unsigned short port)
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		if (isConnected_internal())
		{
			disconnect_internal();
		}
		
		if (m_socket.connect(ip, port) != sf::TcpSocket::Done)
		{
			m_logger.logError("Failed to connect to server IP: "+ip.toString() + " Port: "+std::to_string(port));
		}
		else
		{
			m_logger.logInfo("Connected to server IP: " + ip.toString() + " Port: " + std::to_string(port));
			m_socket.setBlocking(false);
			m_threadRunning = true;
			// Create a thread to handle the client
			m_thread = std::thread(&ServerClient::handleClient, this);
		}
	}
	void ServerClient::update()
	{

	}
	bool ServerClient::isConnected()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return isConnected_internal();
	}
	bool ServerClient::isConnected_internal()
	{
		sf::IpAddress ip = m_socket.getRemoteAddress();
		return ip != sf::IpAddress::None;
	}
	void ServerClient::disconnect()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (!isConnected_internal())
			return;
		disconnect_internal();
	}
	void ServerClient::disconnect_internal()
	{
		if (!m_threadRunning)
			return;
		m_socket.disconnect();
		m_logger.logInfo("Disconnected from server");
		m_threadRunning = false;
		m_thread.join();
	}

	void ServerClient::send(const sf::Packet& packet)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_sending.emplace_back(packet);
		m_hasPacketToSend = true;
	}
	void ServerClient::send(const std::vector<sf::Packet>& packets)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_sending.insert(m_sending.end(), packets.begin(), packets.end());
		m_hasPacketToSend = true;
	}
	bool ServerClient::hasPacket()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return !m_hasPacketReceived;
	}
	std::vector<sf::Packet> ServerClient::getPackets()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		std::vector<sf::Packet> packets = std::move(m_received);
		m_received.clear();
		m_received.reserve(10);
		m_hasPacketReceived = false;
		return packets;
	}


	void ServerClient::handleClient()
	{
		
		sf::TcpSocket& socket = m_socket;
		std::vector<sf::Packet>& sending = m_sending;
		std::vector<sf::Packet>& received = m_received;
		Log::LogObject& logger = m_logger;
		std::atomic<bool>& hasPacketReceived = m_hasPacketReceived;
		std::atomic<bool>& hasPacketToSend = m_hasPacketToSend;
		std::atomic<bool>& threadRunning = m_threadRunning;
		while (threadRunning)
		{
			sf::Packet packet;
			sf::Socket::Status status = socket.receive(packet);
			switch (status)
			{
				case sf::Socket::Status::Done:
				{
					{
						std::unique_lock<std::mutex> lock(m_mutex);
						received.emplace_back(std::move(packet));
						hasPacketReceived = true;
					}
					//logger.logInfo("Received packet from server");
					break;
				}
				case sf::Socket::Disconnected:
				{
					logger.logInfo("Server disconnected");
					break;
				}
			}
			

			if (hasPacketToSend)
			{
				std::vector<sf::Packet> sendingCpy;
				{
					std::unique_lock<std::mutex> lock(m_mutex);
					sendingCpy = std::move(sending);
					sending.clear();
					sending.reserve(10);
					hasPacketToSend = false;
				}
				for (sf::Packet& packet : sendingCpy)
				{
					sf::Socket::Status sendStatus = socket.send(packet);
					switch (sendStatus)
					{
						case sf::Socket::Status::Done:
						{
							//logger.logInfo("Sent packet to server");
							break;
						}
						case sf::Socket::Disconnected:
						{
							logger.logInfo("Server disconnected");
							break;
						}
						default:
						{
							logger.logError("Failed to send packet to server");
							break;
						}
					}
				}
				sending.clear();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		logger.logInfo("Client thread stopped");
	}
}