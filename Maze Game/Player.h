#pragma once
#include <utility>
class Player
{
private:
	std::pair <float, float> position;
	float angle;
	const float FOV;

public:

	Player();
	Player(std::pair <float,float> playerPosition, float playerAngle, const float& playerFOV);

	const std::pair <float, float> Position();
	const float Angle();
	void Move(std::pair <float, float> playerPosition);
	void Rotate(float playerAngle);
	void ResetPosition(std::pair <float, float> playerPosition);

};

