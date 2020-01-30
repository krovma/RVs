#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Core/Vertex_PCU.hpp"

class Game;

enum EntityTypes
{
	TypeDefault = -1,
	TypeOne = 0,

	NumEntityTypes
};

class Entity
{
public:
	Entity() = default;
	Entity(Game *theGame);

	virtual void Update(float deltaSeconds);
	virtual void Render() const;

	bool IsDead() const { return m_flagDead; }
	bool IsGarbage() const { return m_flagGarbage; }
	bool IsOffScreen() const;
	Vec2 GetPosition() const { return m_position; }
	float GetRadiusPhysics() const { return m_radiusPhysics; }
	float GetRadiusCosmetic() const { return m_radiusCosmetic; }
	const Vec2 GetVelocity() const { return m_velocity; }
	const Vec2 GetAcceleration() const { return m_acceleration; }
	float GetOrientationDegrees() const { return m_orientationDegrees; }
	float GetAngularVelocity() const { return m_angularVelocity; }
	float GetAngularAcceleration() const { return m_angularAcceleration; }

	void MarkGarbage();

	void SetPosition(const Vec2 &position);
	void SetVelocity(const Vec2 &velocity);
	void SetAcceleration(const Vec2 &acceleraion);
	void SetOrientationDegrees(float orientationDegrees);
	void SetAngularVelocity(float angularVelocity);
	void SetAngularAcceleration(float angularAcceleration);

protected:
	Game * m_theGame = nullptr;

	Vec2 m_position;
	Vec2 m_velocity;
	Vec2 m_acceleration;
	float m_orientationDegrees = 0.f;
	float m_angularVelocity = 0.f;
	float m_angularAcceleration = 0.f;

	float m_radiusPhysics = 0.f;
	float m_radiusCosmetic = 0.f;

	bool m_flagDead = false;
	bool m_flagGarbage = false;
};