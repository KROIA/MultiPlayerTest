#pragma once
#include "QSFML_EditorWidget.h"
#include <SFML/Network.hpp>
#include "NetworkObject.h"

namespace Game
{
	class Player : public NetworkObject
	{
	public:
		enum Command
		{
			move = 0,
			rotate = 1
		};

		Player(const std::string& name);
		~Player();

		void requestMovement(const sf::Vector2f& movement);
		void requestRotation(float angle);

		void handlePacket(int command, sf::Packet& packet) override;
	protected:
		void update() override;
	private:

		QSFML::Components::RectPainter *m_painter = nullptr;
		bool m_hasMoveResponse = true;
		bool m_hasRotateResponse = true;
	};
}