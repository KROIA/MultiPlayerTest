/*#ifdef QT_ENABLED
#include <QApplication>
#endif
#include <iostream>
#include "MultiPlayerTest.h"

#ifdef QT_WIDGETS_ENABLED
#include <QWidget>
#endif

int main(int argc, char* argv[])
{
#ifdef QT_WIDGETS_ENABLED
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
#ifdef QT_ENABLED
	QApplication app(argc, argv);
#endif
	MultiPlayerTest::Profiler::start();
	MultiPlayerTest::LibraryInfo::printInfo();
#ifdef QT_WIDGETS_ENABLED
	QWidget* widget = MultiPlayerTest::LibraryInfo::createInfoWidget();
	if (widget)
		widget->show();
#endif
	int ret = 0;
#ifdef QT_ENABLED
	ret = app.exec();
#endif
	MultiPlayerTest::Profiler::stop((std::string(MultiPlayerTest::LibraryInfo::name) + ".prof").c_str());
	return ret;
}*/

#include <QCoreApplication>
#include "MultiPlayerTest.h"
#include <iostream>
#include <vector>
#include <thread>

#include "../MultiplayerTestClient/Player.h"

class MyServer : public Game::Server
{
public:
    MyServer()
		: Game::Server() {}
	~MyServer() {}

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
		getLogger().logError("Invalid command received from: " + name + " command: "+std::to_string(command));
		return false;
	}

	void broadcastGameState()
	{
		sendTransforms();
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

int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);
	Log::UI::NativeConsoleView::createStaticInstance();
	Log::UI::NativeConsoleView::getStaticInstance()->show();
	MyServer server;
	server.start(5000);

	QTimer updateTimer;
	QObject::connect(&updateTimer, &QTimer::timeout, [&server]() 
					 {
						 server.update(); 
						 server.broadcastGameState(); 
					 });
	updateTimer.start(10);

	int ret = app.exec();
	server.stop();
	return ret;
}
