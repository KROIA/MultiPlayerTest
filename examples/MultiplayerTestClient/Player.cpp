#include "Player.h"
#include "MyServerClient.h"


namespace Game
{
	int Player::m_objectCount = 0;
	Player::Player(ServerClient *client)
		: NetworkObject(client, "Player_"+std::to_string(m_objectCount++))
	{
		m_painter = new QSFML::Components::RectPainter("PlayerPainter");

		add(m_painter);

		m_painter->setFillColor(sf::Color::Green);
		m_painter->setOutlineColor(sf::Color::Black);
		m_painter->setOutlineThickness(1.0f);
		m_painter->setRect(QSFML::Utilities::AABB(0.0f, 0.0f, 50.0f, 50.0f));
		m_painter->setPosition(-25.0f, -25.0f);	

		m_targetPosition = getPosition();
		m_targetRotation = getRotation();
		m_startPosition = m_targetPosition;
		m_startRotation = m_targetRotation;

	}
	Player::Player(ServerClient *client, const std::string& name)
		: NetworkObject(client, name)
	{
		m_painter = new QSFML::Components::RectPainter("PlayerPainter");

		add(m_painter);

		m_painter->setFillColor(sf::Color::Green);
		m_painter->setOutlineColor(sf::Color::Black);
		m_painter->setOutlineThickness(1.0f);
		m_painter->setRect(QSFML::Utilities::AABB(0.0f, 0.0f, 50.0f, 50.0f));
		m_painter->setPosition(-25.0f, -25.0f);

		m_targetPosition = getPosition();
		m_targetRotation = getRotation();
		m_startPosition = m_targetPosition;
		m_startRotation = m_targetRotation;

	}
	Player::~Player()
	{
		
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
				GameObject::rotate(m_targetRotation);
				m_startRotation = m_targetRotation;
				m_targetRotation = angle;
				m_movementStep = 0;
				break;
			}
			case Command::setPosition:
			{
				sf::Vector2f position;
				packet >> position.x >> position.y;
				GameObject::setPosition(m_targetPosition);
				m_startPosition = m_targetPosition;
				m_targetPosition = position;
				m_hasMoveResponse = true;
				m_movementStep = 0;
				break;
			}
			case Command::setRotation:
			{
				float angle;
				packet >> angle;
				GameObject::setRotation(angle);
				m_hasRotateResponse = true;
				m_movementStep = 0;
				break;
			}
			case Command::setTransform:
			{
				sf::Vector2f position;
				float angle;
				packet >> position.x >> position.y >> angle;
				GameObject::setPosition(position);
				GameObject::setRotation(angle);
				m_movementStep = 1;
				break;
			}
			case Command::setTransformSmoth:
			{
				sf::Vector2f position;
				float angle;
				packet >> position.x >> position.y >> angle;
				GameObject::setPosition(m_targetPosition);
				GameObject::setRotation(m_targetRotation);
				m_startRotation = m_targetRotation;
				m_startPosition = m_targetPosition;
				m_targetPosition = position;
				m_targetRotation = angle;
				m_hasRotateResponse = true;
				m_hasMoveResponse = true;
				//if(m_movementStep > 0)
				//	m_movementStepSize = m_movementStepSize / m_movementStep;

				//logInfo(std::to_string(m_movementStepSize));
				m_movementStep = 0;
				break;
			}
		}
	}

	void Player::updateControlls()
	{
		if (!getDefaultCamera()->isMouseOverWindow())
			return;
		float deltaT = getDeltaT() * 10;
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
			requestMovement(sf::Vector2f(movement) * deltaT);
		if (anngle != 0.0f)
			requestRotation(anngle * deltaT);
	}
	void Player::update()
	{
		
		if (m_isDummy)
		{

		}
		else
		{
			updateControlls();	
		}
		if (m_movementStep > 1)
			return;
		m_movementStep += m_movementStepSize;
		// Interpolate position and rotation to the target position
		sf::Vector2f newPosition = QSFML::VectorMath::lerp(m_startPosition, m_targetPosition, m_movementStep);
		float newRotation = QSFML::VectorMath::lerp(m_startRotation, m_targetRotation, m_movementStep);
		GameObject::setPosition(newPosition);
		GameObject::setRotation(newRotation);
		
	}
}