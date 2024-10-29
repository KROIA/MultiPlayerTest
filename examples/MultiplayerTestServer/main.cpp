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

	bool processPacket(sf::TcpSocket* client, const std::string& name, int command, sf::Packet& packet, sf::Packet& response) override
	{
        switch (static_cast<Game::Player::Command>(command))
        {
			case Game::Player::Command::move:
			{
				sf::Vector2f movement;
				packet >> movement.x >> movement.y;
				//getLogger().logInfo(name + " is moving by " + std::to_string(movement.x) + " " + std::to_string(movement.y));
				response << movement.x << movement.y;
				return true;
			}
			case Game::Player::Command::rotate:
			{
				float angle;
				packet >> angle;
				//getLogger().logInfo(name + " is rotating by " + std::to_string(angle));
				response << angle;
				return true;
			}
        }
		getLogger().logError("Invalid command received from: " + name + " command: "+std::to_string(command));
		return false;
	}
};

int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);
	Log::UI::NativeConsoleView::createStaticInstance();
	Log::UI::NativeConsoleView::getStaticInstance()->show();
	MyServer server;
	server.start(5000);

	QTimer updateTimer;
	QObject::connect(&updateTimer, &QTimer::timeout, [&server]() { server.update(); });
	updateTimer.start(10);

	int ret = app.exec();
	server.stop();
	return ret;
}
