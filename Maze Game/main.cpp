#include <iostream>
#include <Windows.h>
#include <chrono>
#include <vector>
#include <utility>
#include <algorithm>
#include <random>

#include "Player.h"
#include "Map.h"

using namespace std;

//screen dimensions
int nScreenWidth = 120;
int nScreenHeight = 40;

auto startPosition = make_pair(14.0f, 1.0f); //x,y
float startAngle = 0.0f;
const float playerFOV = 3.14159f / 4.0f;
const float playerRotationSpeed = 0.8f;
const float playerMovementSpeed = 5.0f;

float fPlayerX = 14.0f;
float fPlayerY = 1.0f;
float fPlayerAngle = 0.0f;
float fPlayerFOV = 3.14159f / 4.0f;
float fDepth = 16.0f; //Map size is 16 so depth shouldn't exeed 16

int nMapWidth = 16;
int nMapHeight = 16;

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

int main()
{
	//create screen buffer
	wchar_t *screen = new wchar_t[nScreenWidth * nScreenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	//Time points for game ticks
	auto timePoint1 = chrono::system_clock::now();
	auto timePoint2 = chrono::system_clock::now();

	int nPointTracker = 0;

	Player MainPlayer(startPosition, startAngle, playerFOV);

	while (true)
	{
		bool bResetGame = false;
		wstring map = Map::selectMap();

		while (!bResetGame)
		{
			//Time elapsed per game loop
			timePoint2 = chrono::system_clock::now();
			chrono::duration<float> elapsedTime = timePoint2 - timePoint1;
			timePoint1 = timePoint2;
			float fElapsedTime = elapsedTime.count();

			//Multiplying the movement speed of the player angle by a game loop tick gives a smoother movement regardless of what computer is doing
			if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
				MainPlayer.Rotate(MainPlayer.GetAngle() - (playerRotationSpeed * fElapsedTime));
				
			if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
				MainPlayer.Rotate(MainPlayer.GetAngle() + (playerRotationSpeed * fElapsedTime));
			
			pair<float, float> changeCoordinates;
			
			if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
			{
				changeCoordinates.first = sinf(MainPlayer.GetAngle()) * playerMovementSpeed * fElapsedTime;
				changeCoordinates.second = cosf(MainPlayer.GetAngle()) * playerMovementSpeed * fElapsedTime;
				MainPlayer.Move(MainPlayer.GetPosition() + changeCoordinates);

				//Collision detection
				//Here the player coordinates are converted to integer space and tested on the map. If the cell contains a '#', a wall was hit.
				//When a wall is hit, simply reverse the movement, so the player ends up back in the same place.
				if (map[(int)MainPlayer.GetPosition().second * nMapWidth + (int)MainPlayer.GetPosition().first] == '#')
				{
					changeCoordinates.first = sinf(MainPlayer.GetAngle()) * playerMovementSpeed * fElapsedTime;
					changeCoordinates.second = cosf(MainPlayer.GetAngle()) * playerMovementSpeed * fElapsedTime;
					MainPlayer.Move(MainPlayer.GetPosition() - changeCoordinates);
				}
			}

			if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
			{
				changeCoordinates.first = sinf(MainPlayer.GetAngle()) * playerMovementSpeed * fElapsedTime;
				changeCoordinates.second = cosf(MainPlayer.GetAngle()) * playerMovementSpeed * fElapsedTime;
				MainPlayer.Move((MainPlayer.GetPosition() - changeCoordinates));

				if (map[(int)MainPlayer.GetPosition().second * nMapWidth + (int)MainPlayer.GetPosition().first] == '#')
				{
					changeCoordinates.first = sinf(MainPlayer.GetAngle()) * playerMovementSpeed * fElapsedTime;
					changeCoordinates.second = cosf(MainPlayer.GetAngle()) * playerMovementSpeed * fElapsedTime;
					MainPlayer.Move(MainPlayer.GetPosition() + changeCoordinates);
				}
			}

			//randomly generate traps
			//every time a trap is generated, the player gains a point
			static auto trapTiming = chrono::steady_clock::now() + chrono::seconds(3);
			static auto trapRemovalTiming = chrono::steady_clock::now() + chrono::seconds(3);
			static int nPreviousRandomNumber = 0;

			if (chrono::steady_clock::now() > trapTiming)
			{
				nPointTracker += 1;
				random_device randomDevice;
				mt19937 RNG(randomDevice());
				uniform_int_distribution <mt19937::result_type> randomDistribution(0, nMapWidth * nMapHeight);
				int nRandomNumber = randomDistribution(RNG);

				//trap can't be generated on player position nor player position +/- 1 in either direction
				if (map[nRandomNumber] == '.' && map[nRandomNumber] != (
					MainPlayer.GetPosition().first * MainPlayer.GetPosition().second || 
					MainPlayer.GetPosition().first * MainPlayer.GetPosition().second + 1 || 
					MainPlayer.GetPosition().first * MainPlayer.GetPosition().second - 1))
				{
					if (map[nPreviousRandomNumber] == '#')	map[nPreviousRandomNumber] = '#';
					else map[nPreviousRandomNumber] = '.';

					map[nRandomNumber] = 'x';
					trapTiming = chrono::steady_clock::now() + chrono::seconds(3);
					nPreviousRandomNumber = nRandomNumber;
				}
			}

			//kill player and reset game if a trap 'x' is hit
			if (map[(int)MainPlayer.GetPosition().second * nMapWidth + (int)MainPlayer.GetPosition().first] == 'x')
			{
				bResetGame = true;
				nPointTracker = 0;
				MainPlayer.ResetPosition(startPosition);
			}

			for (int x = 0; x < nScreenWidth; x++)
			{
				//for each column of screen width, calculate the projected ray angle into the player space: "centre of player view" + "segments of the view"
				float fRayAngle = (MainPlayer.GetAngle() - (MainPlayer.GetFOV() / 2.0f)) + ((float)x / ((float)nScreenWidth) * MainPlayer.GetFOV());

				float fDistanceToWall = 0.0f;
				bool bHitWall = false;					//sets when a ray hits a wall block
				bool bBoundary = false;					//sets when a ray hits a boundary between two wall blocks

				//unit vector for ray in player space
				float fViewX = sinf(fRayAngle);
				float fViewY = cosf(fRayAngle);

				while (!bHitWall && fDistanceToWall < fDepth)
				{
					fDistanceToWall += 0.1f;

					//incrementally tests to calculate the approximate distance to a wall. Cast to int because we only care about the int portion of this value.
					int nTestX = (int)(MainPlayer.GetPosition().first + fViewX * fDistanceToWall);
					int nTestY = (int)(MainPlayer.GetPosition().second + fViewY * fDistanceToWall);

					//test if ray is out of bounds
					if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
					{
						bHitWall = true;
						fDistanceToWall = fDepth; //sets distance to max depth, as wall was hit.
					}
					else
					{
						//ray is within boundary so test if the ray cell is a wall block, by mapping the 2D space to 1D to check the cells individually 
						if (map[nTestY * nMapWidth + nTestX] == '#')
						{
							bHitWall = true;

							// distance to the corner of a cell, and the dot product, i.e. the angle between the two vectors
							vector<pair<float, float>> pairTest;

							//Four corners to test, so two loops to test X coordinates and Y coordinates
							for (int tx = 0; tx < 2; tx++)
								for (int ty = 0; ty < 2; ty++)
								{
									//Vector from each of the exact corners of the cells, to the player
									float vy = (float)nTestY + ty - fPlayerY;
									float vx = (float)nTestX + tx - fPlayerX;
									//magnitude of unit vector i.e. distance
									float distance = sqrt(vx * vx + vy * vy); 
									// dot product, giving the angle between the ray cast from a corner to the player, and the player view
									float dotProduct = (fViewX * vx / distance) + (fViewY * vy / distance); 
									pairTest.push_back(make_pair(distance, dotProduct));
								}
							//Need closer distance
							sort(pairTest.begin(), pairTest.end(), [](const pair<float, float> &left, const pair<float, float> &right) {return left.first < right.first;});

							float fBound = 0.01f;

							//inverse cos(dot product) gives the angle between the two rays
							if (acos(pairTest.at(0).second) < fBound)	bBoundary = true;
							if (acos(pairTest.at(1).second) < fBound)	bBoundary = true;
						}
					}
				}

				//calculate distance to ceiling and floor
				//For walls further away, there is more ceiling and floor, and vice-versa
				//Therefore as distance to wall increases, take the midpoint of the screen height and subtract a portion of it that gets smaller as the distance away gets larger
				//Floor is mirror of the ceiling
				int nCeiling = ((float)nScreenHeight / 2.0f) - ((float)nScreenHeight / (float)fDistanceToWall);
				int nFloor = nScreenHeight - nCeiling;

				//Shading depending on distance to wall.
				short nShade = ' ';
				if (fDistanceToWall <= fDepth / 4.0f)		nShade = 0x2588;   //very close
				else if (fDistanceToWall < fDepth / 3.0f)	nShade = 0x2593;
				else if (fDistanceToWall < fDepth / 2.0f)	nShade = 0x2592;
				else if (fDistanceToWall < fDepth)			nShade = 0x2591;
				else										nShade = ' ';     //too far away
				
				//black out the boundary
				if (bBoundary)	nShade = ' '; 

				for (int y = 0; y < nScreenHeight; y++)
				{
					if (y < nCeiling)
						screen[y * nScreenWidth + x] = ' ';
					else if (y > nCeiling && y <= nFloor)
						screen[y * nScreenWidth + x] = nShade;
					else
					{
						float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
						short nShadeFloor = ' ';

						if (b < 0.25)		nShadeFloor = '#';
						else if (b < 0.50)	nShadeFloor = '=';
						else if (b < 0.75)	nShadeFloor = '-';
						else				nShadeFloor = ' ';

						screen[y * nScreenWidth + x] = nShadeFloor;
					}
				}
			}

			//if player passes goal, add points and reset game
			if (map[(int)MainPlayer.GetPosition().second * nMapWidth + (int)MainPlayer.GetPosition().first] == 'G')
			{
				nPointTracker += 100;
				bResetGame = true;
				MainPlayer.ResetPosition(startPosition);
			}

			//print the player stats
			swprintf_s(screen, 80, L"X = %3.2f, Y = %3.2f, FPS = %3.2f, Points = %4d\n", MainPlayer.GetPosition().first, MainPlayer.GetPosition().second, 1.0f / fElapsedTime, nPointTracker);

			//draw map
			for (int nx = 0; nx < nMapWidth; nx++)
			{
				for (int ny = 0; ny < nMapWidth; ny++)
				{
					//nMapWidth - nx - 1 mirrors the map so it lines up with the player movement
					screen[(ny + 1) * nScreenWidth + nx] = map[(ny * nMapWidth + (nMapWidth - nx - 1))];
				}
			}

			//draw player marker
			screen[((int)MainPlayer.GetPosition().second + 1) * nScreenWidth + nMapWidth - (int)MainPlayer.GetPosition().first - 1] = 'P';

			//sets the last character of the screen array to the esc character, which stops outputting the string
			screen[nScreenWidth * nScreenHeight - 1] = '\0';

			//("HANDLE, which allows access to whatever it references", buffer, # of bytes, coordinates to be written to
			WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
		}
	}
	return 1337;
}