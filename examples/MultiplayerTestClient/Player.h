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
			rotate = 1,
			setPosition = 2,
			setRotation = 3,
			setTransform = 4,
			setTransformSmoth = 5,
			getPlayers = 6
		};

		Player(ServerClient* client);
		Player(ServerClient* client,const std::string &name);
		~Player();

		void setDummy(bool isDummy)
		{
			m_isDummy = isDummy;
		}

		void requestMovement(const sf::Vector2f& movement);
		void requestRotation(float angle);

		void handlePacket(int command, sf::Packet& packet) override;

		void updateControlls();
	protected:
		void update() override;
	private:

		QSFML::Components::RectPainter *m_painter = nullptr;
		bool m_hasMoveResponse = true;
		bool m_hasRotateResponse = true;

		float m_movementStepSize = 0.25;
		float m_movementStep = 0;
		sf::Vector2f m_startPosition;
		sf::Vector2f m_targetPosition;
		float m_startRotation = 0;
		float m_targetRotation = 0;
		static int m_objectCount;

		bool m_isDummy = false;
	};
}