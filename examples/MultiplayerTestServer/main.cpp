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
#include <csignal>
#include "MultiPlayerTest.h"
#include <iostream>
#include <vector>
#include <QTimer>
#include <thread>

#include "KeyStopHandler.h"
#include "MyServer.h"




Game::MyServer* server = nullptr;

int main(int argc, char* argv[])
{
	// Connect signals to our custom signal handle
	ObjectSerializer::Serializer::registerType<Game::PlayerDataSerializable>();
	QCoreApplication app(argc, argv);
	
	Log::UI::NativeConsoleView::createStaticInstance();
	Log::UI::NativeConsoleView::getStaticInstance()->show();
	server = new Game::MyServer();
	if(server->start(5000))
		server->loadFromFile();

	QTimer updateTimer;
	QObject::connect(&updateTimer, &QTimer::timeout, []() 
					 {
						 server->update(); 
						 server->broadcastGameState(); 
					 });
	updateTimer.start(10);

	Game::KeyStopHandler keyHandler(server);

	
	int ret = app.exec();
	return ret;
}

