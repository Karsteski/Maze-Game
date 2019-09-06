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

	const std::pair <float, float> GetPosition();
	const float GetAngle();
	const float GetFOV();
	void Move(std::pair <float, float> playerPosition);
	void Rotate(float playerAngle);
	void ResetPosition(std::pair <float, float> playerPosition);

};

//Overloads to make std::pair more flexible
template <typename T, typename U>
std::pair<T, U> operator+(const std::pair<T, U>& leftValue, const std::pair<T, U>& rightValue)
{
	return { leftValue.first + rightValue.first,leftValue.second + rightValue.second };
}

template <typename T, typename U>
std::pair<T, U> operator-(const std::pair<T, U>& leftValue, const std::pair<T, U>& rightValue)
{
	return { leftValue.first - rightValue.first,leftValue.second - rightValue.second };
}