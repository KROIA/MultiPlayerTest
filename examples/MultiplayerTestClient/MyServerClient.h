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

		

		NetworkObject* createPlayer(const std::string& name, bool isDummy);
		static std::string getUniquePlayerName();

		void readPlayersFromServer();

		private:
			void onUpdate() override;

		QSFML::Scene* m_scene = nullptr;
		Player* m_mainPlayer = nullptr;
		
		static MyServerClient* s_instance;
		
	};
}