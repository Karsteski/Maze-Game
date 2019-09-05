#include "Player.h"

Player::Player() : position(0.0f, 0.0f), angle(0.0f), FOV(0.0f) {}

Player::Player(std::pair <float, float> playerPosition, float playerAngle, const float& playerFOV) :
position(playerPosition), angle(playerAngle), FOV(playerFOV) {}


const float Player::Angle()
{
	return angle;
}

const std::pair <float, float> Player::Position()
{
	return position;
}

void Player::Move(std::pair <float, float> playerPosition)
{
	position = playerPosition;
}

void Player::Rotate(float playerAngle)
{
	angle = playerAngle;
}

void Player::ResetPosition(std::pair <float, float> playerPosition)
{
	position = { 0.0f,0.0f };
}


