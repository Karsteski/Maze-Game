#include "Player.h"

Player::Player() {};

Player::Player(std::pair <float, float> playerPosition, float playerAngle, float playerFOV)
{
	position = playerPosition;
	angle = playerAngle;
	FOV = playerFOV;
}

float Player::Angle()
{
	return angle;
}

std::pair <float, float> Player::Position()
{
	return position;
}

void Player::Move(std::pair <float, float> playerPosition)
{
	position = playerPosition;
}

void Player::Move(float playerAngle)
{
	angle = playerAngle;
}

void Player::ResetPosition(std::pair <float, float> playerPosition)
{
	position = { 0.0f,0.0f };
}


