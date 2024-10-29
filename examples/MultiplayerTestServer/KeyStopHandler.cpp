#include "KeyStopHandler.h"
#include <QSocketNotifier>
#include <QCoreApplication>

#define NOMINMAX
#include <windows.h>
#include <QTimer>

#include <QDebug>



#include "MyServer.h"
namespace Game
{
	KeyStopHandler::KeyStopHandler(MyServer* server, QObject* parent)
		: QObject(parent)
		, server(server)
	{

		// Set up a timer to check for key press periodically
		timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &KeyStopHandler::checkForKeyPress);
		timer->start(100);  // Check every 100 ms

	}


	void KeyStopHandler::checkForKeyPress()
	{
		// Check if "C" key is pressed
		if (GetAsyncKeyState('C') & 0x8000) {  // Key is currently pressed
			qDebug() << "Key 'C' pressed. Closing the application.";
			server->stop();
			server->saveToFile();
			QCoreApplication::quit();  // Trigger the aboutToQuit signal and exit
		}
	}
}