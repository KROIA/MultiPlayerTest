#pragma once
#include "QSFML_EditorWidget.h"
#include <thread>
#include <SFML/Network.hpp>
#include <mutex>
#include <atomic>

namespace Game
{
	class NetworkObject;
	class ServerClient
	{
	public:
		ServerClient();
		virtual ~ServerClient();
		void addListener(NetworkObject* listener);
		void removeListener(NetworkObject* listener);

		void enableAutoReconnect(bool enabled, unsigned int intervalMs = 1000);
		void enableConnectAsync(bool enabled);

		bool connect(const sf::IpAddress& ip, unsigned short port, unsigned int threadUpdateIntervalMs = 10);
		bool reconnect();
		bool isConnected();
		void disconnect();

		void send(const sf::Packet& packet);
		void send(const std::vector<sf::Packet>& packets);
		bool hasPacket();
		std::vector<sf::Packet> getPackets();

		// Function to append packet2 to the end of packet1 with a size prefix
		static void appendPacketWithSize(sf::Packet& packet1, const sf::Packet& packet2);

		// Function to extract a packet from a combined packet
		static bool extractNextPacket(sf::Packet& source, sf::Packet& extracted);

		void update();

		protected:
		Log::LogObject& getLogger() { return m_logger; }
		std::mutex& getMutex() { return m_mutex; }
		std::unordered_map<std::string, NetworkObject*>& getListeners() { return m_listeners; }

	protected:
		virtual void onUpdate();
		virtual void onConnect();
		virtual void onDisconnect();
	private:
		bool connect_intrnal(const sf::IpAddress& ip, unsigned short port, unsigned int threadUpdateIntervalMs);
		bool isConnected_internal();
		void disconnect_internal();

		void handleClient();
		void handleConnectThread();
		

		std::mutex m_mutex;
		std::thread m_thread;
		sf::TcpSocket m_socket;

		std::vector<sf::Packet> m_received;
		std::atomic<bool> m_hasPacketReceived;
		std::vector<sf::Packet> m_sending;
		std::atomic<bool> m_hasPacketToSend;
		std::atomic<bool> m_connected;
		std::atomic<bool> m_disconnectedEvent;
		
		Log::LogObject m_logger;
		unsigned int m_threadDelay;

		sf::IpAddress m_ip;
		unsigned short m_port;

		bool m_autoReconnect;
		unsigned int m_autoReconnectIntervalMs;
		
		sf::Clock m_autoReconnectClock;


		bool m_connectAsync;
		std::mutex m_connectingMutex;
		std::atomic<bool> m_connectedEvent;
		std::atomic<bool> m_connectingThredRunning;
		std::condition_variable m_connectingThreadCondition;
		std::thread *m_connectThread;

		std::unordered_map<std::string, NetworkObject*> m_listeners;
	};
}