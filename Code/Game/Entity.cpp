#include "Game/Entity.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"

#include "Engine/Math/MathUtils.hpp"
//////////////////////////////////////////////////////////////////////////
Entity::Entity(Game *theGame)
	:m_theGame(theGame)
{
}

void Entity::Update(float deltaSeconds)
{
	m_velocity += m_acceleration * deltaSeconds;
	m_position += m_velocity * deltaSeconds;
}

void Entity::Render() const
{
	//GUARANTEE_OR_DIE(_vert != nullptr, "NOTHING TO RENDER\nin <Entity::render>");
	//_game->getRenderer()->DrawVertexArray(_numVert, _vert);
}

bool Entity::IsOffScreen() const
{
	float screenW, screenH;
	m_theGame->GetScreenSize(&screenW, &screenH);
	
	return
		FloatGt(m_position.x, m_radiusCosmetic + screenW)
		|| FloatGt(m_position.y, m_radiusCosmetic + screenH)
		|| FloatLt(m_position.x, -m_radiusCosmetic)
		|| FloatLt(m_position.y, -m_radiusCosmetic);//not exactly

}

void Entity::MarkGarbage()
{
	m_flagGarbage = true;
	m_flagDead = true;
}

void Entity::SetPosition(const Vec2 &position)
{
	m_position = position;
}

void Entity::SetVelocity(const Vec2 &velocity)
{
	m_velocity = velocity;
}

void Entity::SetAcceleration(const Vec2 &acceleraion)
{
	m_acceleration = acceleraion;
}

void Entity::SetOrientationDegrees(float orientationDegrees)
{
	m_orientationDegrees = orientationDegrees;
}

void Entity::SetAngularVelocity(float angularVelocity)
{
	m_angularVelocity = angularVelocity;
}

void Entity::SetAngularAcceleration(float angularAcceleration)
{
	m_angularAcceleration = angularAcceleration;
}
