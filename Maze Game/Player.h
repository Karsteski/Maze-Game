#pragma once
#include <utility>
class Player
{
private:
	std::pair <float, float> position = { 0.0f, 0.0f };
	float angle = 0.0f;
	float FOV = 0.0f;

public:

	Player();
	Player(std::pair <float,float> playerPosition, float playerAngle, float playerFOV);

	const std::pair <float, float> Position();
	const float Angle();
	void Move(std::pair <float, float> playerPosition);
	void Move(float playerAngle);
	void ResetPosition(std::pair <float, float> playerPosition);

};

