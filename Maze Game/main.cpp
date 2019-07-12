#include <iostream>
#include <Windows.h>
#include <chrono>
using namespace std;

//screen dimensions
int nScreenWidth = 120;
int nScreenHeight = 40;

//player coordinates, view angle, FOV
//8.0 is midpoint, and therefore player is in the middle of the room
float fPlayerX = 8.0;
float fPlayerY = 8.0;
float fPlayerAngle = 0.0;
float fPlayerFOV = 3.14159 / 4.0;
float fDepth = 16.0f; //Map size is 16 so depth shouldn't exeed 16

int nMapWidth = 16;
int nMapHeight = 16;

int main()
{
	//create screen buffer
	wchar_t *screen = new wchar_t[nScreenWidth * nScreenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	wstring map; 
	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";
	
	//we need two time points from the system to create our game ticks, for X and for Y
	auto timePoint1 = chrono::system_clock::now();
	auto timePoint2 = chrono::system_clock::now();

	//game loop
	while (true)
	{
		//calculate time elapsed per game loop
		timePoint2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = timePoint2 - timePoint1;
		float fElapsedTime = elapsedTime.count();

		//controls
		//this handles rotation
		//multiplying the movement speed of the player angle by a game loop tick gives a smoother movement regardless of what our computer is doing in the background. 
		if (GetAsyncKeyState((unsigned short) 'A') & 0x8000)
			fPlayerAngle -= (0.001f) * fElapsedTime;
		if (GetAsyncKeyState((unsigned short) 'D') & 0x8000)
			fPlayerAngle += (0.001f) * fElapsedTime;

		fElapsedTime = elapsedTime.count();

		for (int x = 0; x < nScreenWidth; x++)
		{
			//for each column of screen width, calculate the projected ray angle into the player space: "centre of player view" + "segments of the view"
			float fRayAngle = (fPlayerAngle - fPlayerFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fPlayerFOV;

			float fDistanceToWall = 0.0;
			bool bHitWall = false;
			//unit vector for ray in player space
			float fViewX = sinf(fRayAngle);
			float fViewY = cosf(fRayAngle);

			while (!bHitWall && fDistanceToWall < fDepth)
			{
				fDistanceToWall += 0.1f;

				//incrementally tests to calculate the approximate distance to a wall. Cast to int because we only care about the int portion of this value. The walls exist as integers between 0 - 15
				int nTestX = (int)(fPlayerX + fViewX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fViewY * fDistanceToWall);

				//test if ray is out of bounds
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
				{
					bHitWall = true;
					fDistanceToWall = fDepth; //sets distance to max depth, as we know we've hit a wall
				}
				else
				{
					//ray is within our boundary so we can now test if the ray cell is a wall block, by mapping the 2D array to	1D to check the cells individually 
					if (map[nTestY * nMapWidth + nTestX] == '#')
					{
						bHitWall = true;
					}
				}
			}

			//calculate distance to ceiling and floor
			//For walls further away, there is more ceiling and floor, and vice-versa
			//Therefore as fDistanceToWall gets bigger, we take the midpoint of the screen height and subtract a portion of it that gets smaller as the distance away gets larger
			//Floor is just a mirror of the ceiling
			int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			for (int y = 0; y < nScreenHeight; y++)
			{
				if (y < nCeiling)
					screen[y * nScreenWidth + x] = ' ';
				else if (y >  nCeiling && y <= nFloor)
					screen[y * nScreenWidth + x] = '#';
				else
					screen[y * nScreenWidth + x] = ' ';
			}
		}

		screen[nScreenWidth * nScreenHeight - 1] = '\0'; //sets the last character of the screen array to the esc character, which stops outputting the string
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten); //("HANDLE, which allows access to whatever it references", buffer, # of bytes, coordinates to be written to, "variable that isnt useful")
		 
	}
	return 1337;
}