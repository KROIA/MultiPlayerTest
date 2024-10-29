#include "Player.h"



namespace Game
{
	Player::Player(const std::string& name)
		: NetworkObject(name)
	{
		m_painter = new QSFML::Components::RectPainter("PlayerPainter");

		add(m_painter);

		m_painter->setFillColor(sf::Color::Green);
		m_painter->setOutlineColor(sf::Color::Black);
		m_painter->setOutlineThickness(1.0f);
		m_painter->setRect(QSFML::Utilities::AABB(0.0f, 0.0f, 50.0f, 50.0f));
		m_painter->setPosition(-25.0f, -25.0f);	

		Game::ServerClient::addListener(this);

	}
	Player::~Player()
	{
		Game::ServerClient::removeListener(this);
	}

	void Player::requestMovement(const sf::Vector2f& movement)
	{
		if (!m_hasMoveResponse)
			return;
		sf::Packet packet;
		packet << movement.x << movement.y;
		//logInfo("Requesting movement: " + std::to_string(movement.x) + ", " + std::to_string(movement.y));
		NetworkObject::sendPacket(static_cast<int>(Command::move), packet);
		m_hasMoveResponse = false;
	}
	void Player::requestRotation(float angle)
	{
		if(!m_hasRotateResponse)
			return;
		sf::Packet packet;
		packet << angle;
		//logInfo("Requesting rotation: " + std::to_string(angle));
		NetworkObject::sendPacket(static_cast<int>(Command::rotate), packet);
		m_hasRotateResponse = false;
	}

	void Player::handlePacket(int command, sf::Packet& packet)
	{
		switch (static_cast<Command>(command))
		{
			case Command::move:
			{
				m_hasMoveResponse = true;
				sf::Vector2f movement;
				packet >> movement.x >> movement.y;
				GameObject::move(movement);
				break;
			}
			case Command::rotate:
			{
				m_hasRotateResponse = true;
				float angle;
				packet >> angle;
				GameObject::rotate(angle);
				break;
			}
		}
	}


	void Player::update()
	{
		float deltaT = getDeltaT()*10;

		sf::Vector2i movement(0, 0);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
			movement.y -= 100;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
			movement.y += 100;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
			movement.x -= 100;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
			movement.x += 100;

		float anngle = 0.0f;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
			anngle -= 100.0f;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::E))
			anngle += 100.0f;

		if (movement.x != 0 || movement.y != 0)
			requestMovement(sf::Vector2f(movement)* deltaT);
		if (anngle != 0.0f)
			requestRotation(anngle * deltaT);
	}
}