#pragma once
#include <QObject>


namespace Game
{
	class MyServer;
	class KeyStopHandler : public QObject {
		Q_OBJECT

		public:
		KeyStopHandler(MyServer* server, QObject* parent = nullptr);

		public slots:
		void checkForKeyPress();

		private:
		QTimer* timer;
		MyServer* server;
	};
}

