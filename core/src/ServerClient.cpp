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

	ServerClient& ServerClient::getInstance()
	{
		static ServerClient instance;
		return instance;
	}

	void ServerClient::addListener(NetworkObject* listener)
	{
		ServerClient& instance = ServerClient::getInstance();
		std::unique_lock<std::mutex> lock(instance.m_mutex);
		instance.m_listeners[listener->getName()] = listener;
	}
	void ServerClient::removeListener(NetworkObject* listener)
	{
		ServerClient& instance = ServerClient::getInstance();
		std::unique_lock<std::mutex> lock(instance.m_mutex);
		instance.m_listeners.erase(listener->getName());
	}
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
	}

	void ServerClient::connect(const sf::IpAddress& ip, unsigned short port)
	{
		ServerClient& instance = getInstance();
		std::unique_lock<std::mutex> lock(instance.m_mutex);

		if (instance.isConnected_internal())
		{
			instance.disconnect_internal();
		}
		
		if (instance.m_socket.connect(ip, port) != sf::TcpSocket::Done)
		{
			instance.m_logger.logError("Failed to connect to server IP: "+ip.toString() + " Port: "+std::to_string(port));
		}
		else
		{
			instance.m_logger.logInfo("Connected to server IP: " + ip.toString() + " Port: " + std::to_string(port));
			instance.m_socket.setBlocking(false);
			instance.m_threadRunning = true;
			// Create a thread to handle the client
			instance.m_thread = std::thread(&ServerClient::handleClient);
		}
	}
	bool ServerClient::isConnected()
	{
		ServerClient& instance = getInstance();
		std::unique_lock<std::mutex> lock(instance.m_mutex);
		return instance.isConnected_internal();
	}
	bool ServerClient::isConnected_internal()
	{
		sf::IpAddress ip = m_socket.getRemoteAddress();
		return ip != sf::IpAddress::None;
	}
	void ServerClient::disconnect()
	{
		ServerClient& instance = getInstance();
		std::unique_lock<std::mutex> lock(instance.m_mutex);
		if (!instance.isConnected_internal())
			return;
		instance.disconnect_internal();
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
		ServerClient& instance = getInstance();
		std::unique_lock<std::mutex> lock(instance.m_mutex);
		instance.m_sending.emplace_back(packet);
		instance.m_hasPacketToSend = true;
	}
	void ServerClient::send(const std::vector<sf::Packet>& packets)
	{
		ServerClient& instance = getInstance();
		std::unique_lock<std::mutex> lock(instance.m_mutex);
		instance.m_sending.insert(instance.m_sending.end(), packets.begin(), packets.end());
		instance.m_hasPacketToSend = true;
	}
	bool ServerClient::hasPacket()
	{
		ServerClient& instance = getInstance();
		std::unique_lock<std::mutex> lock(instance.m_mutex);
		return !instance.m_hasPacketReceived;
	}
	std::vector<sf::Packet> ServerClient::getPackets()
	{
		ServerClient& instance = getInstance();
		std::unique_lock<std::mutex> lock(instance.m_mutex);
		std::vector<sf::Packet> packets = std::move(instance.m_received);
		instance.m_received.clear();
		instance.m_received.reserve(10);
		instance.m_hasPacketReceived = false;
		return packets;
	}


	void ServerClient::handleClient()
	{
		ServerClient& instance = getInstance();
		sf::TcpSocket& socket = instance.m_socket;
		std::vector<sf::Packet>& sending = instance.m_sending;
		std::vector<sf::Packet>& received = instance.m_received;
		Log::LogObject& logger = instance.m_logger;
		std::atomic<bool>& hasPacketReceived = instance.m_hasPacketReceived;
		std::atomic<bool>& hasPacketToSend = instance.m_hasPacketToSend;
		std::atomic<bool>& threadRunning = instance.m_threadRunning;
		while (threadRunning)
		{
			sf::Packet packet;
			sf::Socket::Status status = socket.receive(packet);
			switch (status)
			{
				case sf::Socket::Status::Done:
				{
					{
						std::unique_lock<std::mutex> lock(instance.m_mutex);
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
					std::unique_lock<std::mutex> lock(instance.m_mutex);
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