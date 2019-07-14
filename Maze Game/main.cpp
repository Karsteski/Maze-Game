#include <iostream>
#include <Windows.h>
#include <chrono>
#include <vector>
#include <utility>
#include <algorithm>
#include <stdio.h>

using namespace std;

//screen dimensions
int nScreenWidth = 120;
int nScreenHeight = 40;

//player coordinates, view angle, FOV
//8.0 is midpoint, and therefore player is in the middle of the room
float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerAngle = 0.0f;
float fPlayerFOV = 3.14159f / 4.0f;
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
	map += L"#...........####";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#......#.......#";
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
		timePoint1 = timePoint2;
		float fElapsedTime = elapsedTime.count();

		//controls
		//this handles rotation
		//multiplying the movement speed of the player angle by a game loop tick gives a smoother movement regardless of what our computer is doing in the background. 
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			fPlayerAngle -= (0.8f) * fElapsedTime;
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			fPlayerAngle += (0.8f) * fElapsedTime;

		//this handles forward and backward movement
		// 5.0f just multiplies the unit vector by 5 to give it a magnitude
		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			fPlayerX += sinf(fPlayerAngle) * 5.0f * fElapsedTime;
			fPlayerY += cosf(fPlayerAngle) * 5.0f * fElapsedTime;

			//collision detection
			//here the player coordinates are converted to integer space and tested on the map. If the cell contains a '#', then we know we've hit a wall.
			//when we hit a wall, we simply reverse the movement, so the player ends up back in the same place.
			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX -= sinf(fPlayerAngle) * 5.0f * fElapsedTime;
				fPlayerY -= cosf(fPlayerAngle) * 5.0f * fElapsedTime;
			}
		}																				

		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			fPlayerX -= sinf(fPlayerAngle) * 5.0f * fElapsedTime;
			fPlayerY -= cosf(fPlayerAngle) * 5.0f * fElapsedTime;

			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX += sinf(fPlayerAngle) * 5.0f * fElapsedTime;
				fPlayerY += cosf(fPlayerAngle) * 5.0f * fElapsedTime;
			}
		}

		for (int x = 0; x < nScreenWidth; x++)
		{
			//for each column of screen width, calculate the projected ray angle into the player space: "centre of player view" + "segments of the view"
			float fRayAngle = (fPlayerAngle - (fPlayerFOV / 2.0f)) + ((float)x / ((float)nScreenWidth) * fPlayerFOV);

			float fDistanceToWall = 0.0f;
			bool bHitWall = false;					//sets when a ray hits a wall block
			bool bBoundary = false;					//sets when a ray hits a boundary between two wall blocks

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
					//ray is within our boundary so we can now test if the ray cell is a wall block, by mapping the 2D space to	1D to check the cells individually 
					if (map[nTestY * nMapWidth + nTestX] == '#')
					{
						bHitWall = true;

						 // distance to the corner of a cell, and the dot product, i.e. the angle between the two vectors
						vector<pair<float, float>> p;

						//we have four corners to test, so we create two loops to test X coordiinates and Y coordinates
						for (int tx = 0; tx < 2; tx++)
							for (int ty = 0; ty < 2; ty++)
							{
								//this gives us a vector from each of the exact corners of the cells, to the player
								float vy = (float)nTestY + ty - fPlayerY;
								float vx = (float)nTestX + tx - fPlayerX;
								float distance = sqrt(vx * vx + vy * vy); //magnitude of unit vector i.e. distance
								float dot = (fViewX * vx / distance) + (fViewY * vy / distance); // dot product, giving the angle between the ray cast from a corner to the player, and the player view
								p.push_back(make_pair(distance, dot));
							}
						//we need to sort our points - lambda function takes two pairs as arguments, does a simple boolean test to see if one pair is smaller than the other based on the first element on the pair, i.e. distance
						sort(p.begin(), p.end(), [](const pair<float, float> &left, const pair<float, float> &right) {return left.first < right.first; });

						float fBound = 0.01f;

						//inverse cos(dot product) gives the angle between the two rays
						if (acos(p.at(0).second) < fBound)	bBoundary = true;
						if (acos(p.at(1).second) < fBound)	bBoundary = true;
						//if (acos(p.at(3).second) < fBound)	bBoundary = true;
						//you can add another corner test but it does occasionally draw a corner through a cell...


					}
				}
			}

			//calculate distance to ceiling and floor
			//For walls further away, there is more ceiling and floor, and vice-versa
			//Therefore as fDistanceToWall gets bigger, we take the midpoint of the screen height and subtract a portion of it that gets smaller as the distance away gets larger
			//Floor is just a mirror of the ceiling
			int nCeiling = ((float)nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			//shading in UNICODE that makes the walls appear distant/close depending on fDistanceToWall
			short nShade = ' ';
			if (fDistanceToWall <= fDepth / 4.0f)		nShade = 0x2588;   //very close
			else if (fDistanceToWall < fDepth / 3.0f)	nShade = 0x2593;
			else if (fDistanceToWall < fDepth / 2.0f)	nShade = 0x2592;
			else if (fDistanceToWall < fDepth)			nShade = 0x2591;
			else										nShade = ' ';     //too far away

			if (bBoundary)	nShade = ' '; //black out the boundary

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

					if (b < 0.25)
						nShadeFloor = '#';
					else if (b < 0.50)
						nShadeFloor = 'x';
					else if (b < 0.75)
						nShadeFloor = '-';
					else
						nShadeFloor = ' ';
					screen[y * nScreenWidth + x] = nShadeFloor;
				}
			}
		}
		//print the player stats - X/Y coordinates, player angle, frame rate = 1/time per frame, synced per game loop
		swprintf_s(screen, 60, L" X = %3.2f, Y = %3.2f, Angle = %3.2f, FPS = %3.2f ", fPlayerX, fPlayerY, fPlayerAngle, 1.0f / fElapsedTime);

		//draws a map
		for (int nx = 0; nx < nMapWidth; nx++)
		{
			for (int ny = 0; ny < nMapWidth; ny++)
			{
				screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx]; //nMapWidth - nx - 1 mirrors the map so it lines up with the player movement
			}
		}

		//draws the player marker on the map
		screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX ] = 'P';

		screen[nScreenWidth * nScreenHeight - 1] = '\0'; //sets the last character of the screen array to the esc character, which stops outputting the string
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten); //("HANDLE, which allows access to whatever it references", buffer, # of bytes, coordinates to be written to, "variable that isnt useful")
		 
	}
	return 1337;
}