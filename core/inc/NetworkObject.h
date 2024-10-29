#pragma once
#include "QSFML_EditorWidget.h"
#include <thread>
#include <SFML/Network.hpp>
#include <mutex>
#include <atomic>
#include "ServerClient.h"

namespace Game
{
	class NetworkObject : public QSFML::Objects::GameObject
	{
	public:
		NetworkObject(const std::string &name = "NetworkObject")
			: QSFML::Objects::GameObject(name)
		{
		}
		~NetworkObject() {}

		virtual void handlePacket(int command, sf::Packet& packet) = 0;
	protected:
		
		void sendPacket(int command, const sf::Packet& packet)
		{
			sf::Packet p;
			p << getName() << command;// << packet.getData();
			ServerClient::appendPacketWithSize(p, packet);
			ServerClient::send(p);
		}

	};
}