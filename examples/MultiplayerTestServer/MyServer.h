#pragma once


#include "MultiPlayerTest.h"
#include <vector>
#include <unordered_map>
#include "ObjectSerializer.h"


#include "../MultiplayerTestClient/Player.h"

namespace Game
{
	
	class PlayerDataSerializable : public ObjectSerializer::ISerializable
	{
		public:
		float position[2];
		float rotation;
		char name[50];
	};

	class MyServer : public Game::Server
	{
		public:
		MyServer()
			: Game::Server()
		{

		}
		~MyServer()
		{
			saveToFile();
			stop();
		}

		bool processPacket(sf::TcpSocket* client, const std::string& name, int command, sf::Packet& packet, sf::Packet& response, int& responseCommand) override
		{
			switch (static_cast<Game::Player::Command>(command))
			{
				case Game::Player::Command::move:
				{
					PlayerData& player = m_players[name];
					sf::Vector2f movement;
					packet >> movement.x >> movement.y;
					player.position += movement;
					player.hasChanges = true;
					//getLogger().logInfo(name + " is moving by " + std::to_string(movement.x) + " " + std::to_string(movement.y));
					//response << player.position.x << player.position.y;
					//responseCommand = static_cast<int>(Game::Player::Command::setPosition);
					return false;
				}
				case Game::Player::Command::rotate:
				{
					PlayerData& player = m_players[name];
					float angle;
					packet >> angle;
					player.rotation += angle;
					player.hasChanges = true;
					//getLogger().logInfo(name + " is rotating by " + std::to_string(angle));
					//response << player.rotation;
					//responseCommand = static_cast<int>(Game::Player::Command::setRotation);
					return false;
				}
				case Game::Player::Command::getPlayers:
				{
					responseCommand = static_cast<int>(Game::Player::Command::getPlayers);
					// send the names of the players
					response << static_cast<int>(m_players.size());
					std::string playerNames;
					for (auto& pair : m_players)
					{
						playerNames += pair.first + "\n";
						pair.second.hasChanges = true;
						response << pair.first;
					}
					getLogger().logInfo("Sending player names: " + playerNames);
					return true;
				}
			}
			getLogger().logError("Invalid command received from: " + name + " command: " + std::to_string(command));
			return false;
		}

		void broadcastGameState()
		{
			sendTransforms();
		}

		void saveToFile()
		{
			getLogger().logInfo("Saving players to file");
			ObjectSerializer::Serializer serializer;

			std::vector< PlayerDataSerializable* > players;
			players.resize(m_players.size());
			size_t i = 0;
			for (auto& it : m_players)
			{
				auto& player = it.second;
				players[i] = new PlayerDataSerializable();
				//players[i]->position = player.position;
				players[i]->position[0] = player.position.x;
				players[i]->position[1] = player.position.y;
				players[i]->rotation = player.rotation;
				strcpy(players[i]->name, it.first.c_str());
				serializer.addObject(players[i]);
				++i;
			}
			serializer.saveToFile("players.dat");
			for (auto& player : players)
				delete player;
		}
		void loadFromFile()
		{
			ObjectSerializer::Serializer serializer;

			if (serializer.loadFromFile("players.dat"))
			{
				getLogger().logInfo("Loading players from file");
				std::vector<ObjectSerializer::ISerializable*> players = serializer.getObjects();
				for (ObjectSerializer::ISerializable* player : players)
				{
					PlayerDataSerializable* playerData = dynamic_cast<PlayerDataSerializable*>(player);
					if (playerData)
					{
						PlayerData data;
						data.position.x = playerData->position[0];
						data.position.y = playerData->position[1];
						data.rotation = playerData->rotation;
						data.hasChanges = true;
						m_players[playerData->name] = data;
					}
				}

				for (auto& p : players)
				{
					delete p;
				}
			}
		}

		private:
		void sendTransforms()
		{
			for (auto& pair : m_players)
			{
				auto& player = pair.second;
				if (!player.hasChanges)
					continue;
				player.hasChanges = false;
				sf::Packet packet;

				packet << player.position.x << player.position.y << player.rotation;

				for (sf::TcpSocket* client : getClients())
					sendPacket(client, pair.first, static_cast<int>(Game::Player::Command::setTransformSmoth), packet);
			}
		}


		struct PlayerData
		{
			sf::Vector2f position;
			float rotation;
			bool hasChanges = true;
		};

		std::unordered_map<std::string, PlayerData> m_players;
	};
}