#include "ServerClient.h"
#include "NetworkObject.h"
#include <SFML/System/Time.hpp>

namespace Game
{
	ServerClient::ServerClient()
		: m_logger("ServerClient")
	{
		m_connected = false;
		m_hasPacketToSend = false;
		m_hasPacketReceived = false;

		m_autoReconnect = false;
		m_autoReconnectIntervalMs = 1000;
		m_connectedEvent = false;

		m_ip = sf::IpAddress::None;
		m_port = 0;
		m_connectAsync = true;
		m_connectThread = nullptr;
		enableConnectAsync(true);
	}

	ServerClient::~ServerClient()
	{
		if (isConnected_internal())
			return;
		disconnect_internal();
	}

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

	bool ServerClient::connect(const sf::IpAddress& ip, unsigned short port, unsigned int threadUpdateIntervalMs)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_connectAsync)
		{
			if (!m_connectThread)
			{
				m_connectingThredRunning = true;
				m_connectThread = new std::thread(&ServerClient::handleConnectThread, this);
			}
			m_connectingThreadCondition.notify_one();
		}
		
		return connect_intrnal(ip, port, threadUpdateIntervalMs);
	}
	bool ServerClient::connect_intrnal(const sf::IpAddress& ip, unsigned short port, unsigned int threadUpdateIntervalMs)
	{
		if (isConnected_internal())
		{
			disconnect_internal();
		}
		m_threadDelay = threadUpdateIntervalMs;
		m_ip = ip;
		m_port = port;

		if (m_socket.connect(ip, port, sf::milliseconds(m_autoReconnectIntervalMs/2)) != sf::TcpSocket::Done)
		{
			m_logger.logError("Failed to connect to server IP: " + ip.toString() + " Port: " + std::to_string(port));
			return false;
		}
		else
		{
			m_logger.logInfo("Connected to server IP: " + ip.toString() + " Port: " + std::to_string(port));
			m_socket.setBlocking(false);
			m_connected = true;
			// Create a thread to handle the client
			m_thread = std::thread(&ServerClient::handleClient, this);
			m_connectedEvent = true;
		}
		return true;
	}
	void ServerClient::enableAutoReconnect(bool enabled, unsigned int intervalMs)
	{
		m_autoReconnect = enabled;
		m_autoReconnectIntervalMs = intervalMs;
	}
	void ServerClient::enableConnectAsync(bool enabled)
	{
		m_connectAsync = enabled;
		if (!m_connectAsync)
		{
			m_connectingThredRunning = false;
			if (m_connectThread)
			{
				m_connectThread->join();
				delete m_connectThread;
				m_connectThread = nullptr;
			}
		}
	}
	bool ServerClient::reconnect()
	{
		if(!m_connected) [[likely]]
		{
			return connect(m_ip, m_port, m_threadDelay);
		}
	}
	void ServerClient::update()
	{
		if (m_disconnectedEvent) [[unlikely]]
		{
			m_disconnectedEvent = false;
			m_connected = false;
			m_thread.join();
			m_autoReconnectClock.restart();
			m_socket.setBlocking(true);
			onDisconnect();
		}
		if (m_connectedEvent) [[unlikely]]
		{
			m_connectedEvent = false;
			m_socket.setBlocking(false);
			onConnect();
		}
		if (m_connected) [[likely]]
			onUpdate();
		else
		{
			if (m_autoReconnect)
			{
				if (m_connectAsync)
				{
					m_connectingThreadCondition.notify_one();
				}
				else
				{
					if (m_autoReconnectClock.getElapsedTime().asMilliseconds() >= m_autoReconnectIntervalMs)
					{
						m_autoReconnectClock.restart();
						bool ret = connect(m_ip, m_port, m_threadDelay);
						if (ret)
						{
							getLogger().log("Reconnected to server", Log::Level::info, Log::Colors::green);
						}
						else
						{
							getLogger().log("Failed to reconnect to server", Log::Level::error, Log::Colors::red);
						}
					}
				}
			}
		}
	}
	void ServerClient::onUpdate()
	{
		
	}
	void ServerClient::onConnect()
	{
		getLogger().logInfo("Connected to server");
	}
	void ServerClient::onDisconnect()
	{
		getLogger().logInfo("Disconnected from server");
	}
	bool ServerClient::isConnected()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return isConnected_internal();
	}
	bool ServerClient::isConnected_internal()
	{
		return m_connected;
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
		if (!m_connected)
			return;
		m_socket.disconnect();
		m_logger.logInfo("Disconnected from server");
		m_connected = false;
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

	void ServerClient::appendPacketWithSize(sf::Packet& packet1, const sf::Packet& packet2) {
		std::size_t size = packet2.getDataSize();
		packet1 << static_cast<sf::Uint32>(size); // Prefix with size of packet2
		packet1.append(packet2.getData(), size);   // Append raw data from packet2
	}

	// Function to extract a packet from a combined packet
	bool ServerClient::extractNextPacket(sf::Packet& source, sf::Packet& extracted) {
		sf::Uint32 size;

		// Read the packet size first
		if (!(source >> size)) {
			return false; // Failed to read size
		}

		// Check if the source has enough data for the specified packet size
		if (source.getDataSize() < source.getReadPosition() + size) {
			return false; // Not enough data in source
		}

		// Copy the exact number of bytes into a temporary packet
		const char* data = static_cast<const char*>(source.getData()) + source.getReadPosition();
		extracted.append(data, size);

		// Move the read position forward by reading the extracted data directly
		sf::Packet temp;
		const char* skipData = static_cast<const char*>(source.getData()) + source.getReadPosition() + size;
		temp.append(skipData, source.getDataSize() - (source.getReadPosition() + size));
		source.clear();
		source.append(temp.getData(), temp.getDataSize());

		return true;
	}


	void ServerClient::handleClient()
	{
		
		sf::TcpSocket& socket = m_socket;
		std::vector<sf::Packet>& sending = m_sending;
		std::vector<sf::Packet>& received = m_received;
		Log::LogObject& logger = m_logger;
		std::atomic<bool>& hasPacketReceived = m_hasPacketReceived;
		std::atomic<bool>& hasPacketToSend = m_hasPacketToSend;
		std::atomic<bool>& connected = m_connected;

		m_disconnectedEvent = false;

		const auto delay = std::chrono::milliseconds(m_threadDelay);
		while (connected)
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
					logger.logWarning("Connection lost");
					connected = false;
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
							logger.logWarning("Connection lost");
							connected = false;
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
			std::this_thread::sleep_for(delay);
		}
		logger.logInfo("Client thread stopped");
		m_disconnectedEvent = true;
	}


	void ServerClient::handleConnectThread()
	{
		while (m_connectingThredRunning)
		{		
			if (m_autoReconnect)
			{
				bool ret = connect(m_ip, m_port, m_threadDelay);
				do {
					if (ret)
					{
						getLogger().log("Reconnected to server", Log::Level::info, Log::Colors::green);
					}
					else
					{
						getLogger().log("Failed to reconnect to server", Log::Level::error, Log::Colors::red);
						std::this_thread::sleep_for(std::chrono::milliseconds(m_autoReconnectIntervalMs));
					}
					ret = connect(m_ip, m_port, m_threadDelay);
				} while (!ret);
			}
			else
			{
				connect_intrnal(m_ip, m_port, m_threadDelay);
			}	
			std::unique_lock<std::mutex> lock(m_connectingMutex);
			m_connectingThreadCondition.wait(lock);
			
		}
	}

}