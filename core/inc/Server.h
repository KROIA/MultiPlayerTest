#pragma once
#include "QSFML_EditorWidget.h"
#include <thread>
#include <SFML/Network.hpp>
#include <mutex>
#include <atomic>

namespace Game
{

	class Server
	{
	public:
		static Log::LogObject& getLogger();
		Server();
		virtual ~Server();

		void start(unsigned short port);
		void stop();

		virtual void update();
		virtual bool processPacket(sf::TcpSocket* client, const std::string& name, int command, sf::Packet& packet, sf::Packet &response);

		std::vector<std::pair<sf::TcpSocket*, sf::Packet>> getReceivedPackets();
		void setSendingPackets(const std::vector<std::pair<sf::TcpSocket*, sf::Packet>>& packets);


		

	private:
		void handleServer();
		bool receivePackets(sf::TcpSocket& client);

		std::mutex m_mutex;
		std::thread m_thread;
		sf::TcpListener m_listener;
		std::vector<sf::TcpSocket*> m_clients;
		std::atomic<bool> m_threadRunning;

		std::vector<std::pair<sf::TcpSocket*, sf::Packet>> m_received;
		std::atomic<bool> m_hasPacketReceived;
		std::vector<std::pair<sf::TcpSocket*, sf::Packet>> m_sending;
		std::atomic<bool> m_hasPacketToSend;
	};
}