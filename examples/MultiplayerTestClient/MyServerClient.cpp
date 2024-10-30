#include "MyServerClient.h"
#include "NetworkObject.h"
#include "Player.h"

namespace Game
{
	MyServerClient* MyServerClient::s_instance = nullptr;

	MyServerClient::MyServerClient(QSFML::Scene* scene)
		: ServerClient()
		, m_scene(scene)
	{
		if (s_instance == nullptr)
			s_instance = this;
		else
			getLogger().logError("MyServerClient instance already exists");

		enableAutoReconnect(true);
	}
	MyServerClient::~MyServerClient()
	{
		s_instance = nullptr;
	}

	void MyServerClient::onUpdate()
	{
		std::vector<sf::Packet> received = getPackets();
		auto& listeners = getListeners();
		for (sf::Packet& packet : received)
		{
			std::string name;
			int command;
			packet >> name >> command;
			sf::Packet subPacket;
			if (ServerClient::extractNextPacket(packet, subPacket))
			{
				if (name == "Client")
				{
					switch (static_cast<Player::Command>(command))
					{
						case Player::Command::getPlayers:
						{
							int count;
							std::string playerName;
							subPacket >> count;
							
							for (int i = 0; i < count; i++)
							{
								subPacket >> playerName;
								auto it = listeners.find(playerName);
								if (it == listeners.end())
								{
									createPlayer(playerName, true);
								}
							}
							if (!m_mainPlayer)
							{
								getLogger().logInfo("No player found, creating a player");
								createPlayer(getUniquePlayerName(), false);
							}
						}
					}
				}
				else
				{
					auto it = listeners.find(name);
					if (it == listeners.end())
					{
						//getLogger().logError("Failed to find listener with name: " + name);
						NetworkObject* player = createPlayer(name, true);
						
						player->handlePacket(command, subPacket);
						continue;
					}
					NetworkObject* listener = it->second;
					listener->handlePacket(command, subPacket);
				}				
			}
		}
	}
	void MyServerClient::onConnect()
	{
		// Read players
		readPlayersFromServer();
	}

	NetworkObject* MyServerClient::createPlayer(const std::string& name, bool isDummy)
	{
		getLogger().logInfo("Creating player: " + name + " " +(isDummy?"Dummy":"MainPlayer"));
		Player* player = new Player(this, name);
		if (isDummy)
		{
			player->setDummy(true);
		}
		else
		{
			m_mainPlayer = player;
			m_mainPlayer->setDummy(false);
		}
		//addListener(player);
		player->requestRotation(0);
		m_scene->addObject(player);
		return player;
	}

	std::string MyServerClient::getUniquePlayerName()
	{
		if(!s_instance)
			return "Player_1";
		return "Player_" + std::to_string(s_instance->getListeners().size() + 1);
	}
	void MyServerClient::readPlayersFromServer()
	{
		getLogger().logInfo("Reading players from server");
		sf::Packet packet;
		packet << "Client" << static_cast<int>(Player::Command::getPlayers);
		send(packet);
	}
}