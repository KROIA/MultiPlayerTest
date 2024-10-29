#pragma once
#include "ServerClient.h"

namespace Game
{
	class Player;
	class MyServerClient : public ServerClient
	{
		public:
		MyServerClient(QSFML::Scene* scene);
		~MyServerClient();

		void update() override;

		NetworkObject* createPlayer(const std::string& name, bool isDummy);
		static std::string getUniquePlayerName();

		void readPlayersFromServer();

		private:

		QSFML::Scene* m_scene = nullptr;
		Player* m_mainPlayer = nullptr;
		
		static MyServerClient* s_instance;
		
	};
}