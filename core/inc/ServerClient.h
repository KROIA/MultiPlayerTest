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
		

		//static ServerClient& getInstance();
	public:
		ServerClient();
		virtual ~ServerClient();
		void addListener(NetworkObject* listener);
		void removeListener(NetworkObject* listener);
		//static void updateListeners();

		void connect(const sf::IpAddress& ip, unsigned short port);
		bool isConnected();
		void disconnect();

		void send(const sf::Packet& packet);
		void send(const std::vector<sf::Packet>& packets);
		bool hasPacket();
		std::vector<sf::Packet> getPackets();

		// Function to append packet2 to the end of packet1 with a size prefix
		static void appendPacketWithSize(sf::Packet& packet1, const sf::Packet& packet2) {
			std::size_t size = packet2.getDataSize();
			packet1 << static_cast<sf::Uint32>(size); // Prefix with size of packet2
			packet1.append(packet2.getData(), size);   // Append raw data from packet2
		}

		// Function to extract a packet from a combined packet
		static bool extractNextPacket(sf::Packet& source, sf::Packet& extracted) {
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
		
		virtual void update();

		protected:
		Log::LogObject& getLogger() { return m_logger; }
		std::mutex& getMutex() { return m_mutex; }
		std::unordered_map<std::string, NetworkObject*>& getListeners() { return m_listeners; }
	private:
		bool isConnected_internal();
		void disconnect_internal();

		void handleClient();

		

		std::mutex m_mutex;
		std::thread m_thread;
		sf::TcpSocket m_socket;

		std::vector<sf::Packet> m_received;
		std::atomic<bool> m_hasPacketReceived;
		std::vector<sf::Packet> m_sending;
		std::atomic<bool> m_hasPacketToSend;
		std::atomic<bool> m_threadRunning;
		Log::LogObject m_logger;

		std::unordered_map<std::string, NetworkObject*> m_listeners;
	};
}