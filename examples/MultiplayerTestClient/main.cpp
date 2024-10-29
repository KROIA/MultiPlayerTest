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



#include <QApplication>
#include <QMainWindow>
#include <QTimer>
#include <SFML/Network.hpp>
#include <thread>
#include "MultiPlayerTest.h"
#include "Player.h"


namespace Game
{

	void setupScene(QWidget* widget);

	QSFML::Scene* scene = nullptr;
	Player* player = nullptr;
	QSFML::Objects::GameObjectPtr clientUpdateObj = nullptr;
}

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	QMainWindow widget;
	widget.resize(800, 600);
	Log::UI::NativeConsoleView::createStaticInstance();
	Log::UI::NativeConsoleView::getStaticInstance()->show();

	Game::ServerClient::connect("127.0.0.1", 5000);

	Game::setupScene(&widget);
	QTimer connectionTiler;
	QTimer updateTimer;
	QObject::connect(&connectionTiler, &QTimer::timeout, [&connectionTiler]() {
		if (Game::ServerClient::isConnected())
		{
			connectionTiler.stop();
			if (!Game::clientUpdateObj)
			{
				Game::clientUpdateObj = new QSFML::Objects::GameObject("ClientHandler");
				Game::clientUpdateObj->addUpdateFunction([](QSFML::Objects::GameObject& obj)
					{
						Game::ServerClient::updateListeners();
					});
				if (Game::scene)
				{
					Game::scene->addObject(Game::clientUpdateObj);
					Game::scene->getSceneLogger().logInfo("ClientHandler added to scene");
					Game::scene->start();
				}
			}
		}
		});
	connectionTiler.start(1000);

	int ret = app.exec();
	Game::ServerClient::disconnect();
	Game::scene->stop();
	delete Game::scene;
	return ret;
}

namespace Game
{
	void setupScene(QWidget *widget)
	{
		using namespace QSFML;
		widget->resize(800, 600);

		SceneSettings settings;
		settings.timing.frameTime = 1.0f / 60.0f;
		scene = new Scene(widget, settings);

		scene->addObject(new Objects::DefaultEditor("Editor", sf::Vector2f(1000, 800)));
		player = new Player("Player");
		scene->addObject(player);

		widget->show();
	}
}