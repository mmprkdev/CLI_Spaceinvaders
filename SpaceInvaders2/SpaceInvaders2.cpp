// SpaceInvaders2.cpp : Defines the entry point for the console application.
// Shoutout to Javidx9 for his Comand Line Tetris tutorial which inspired me to code this.
// His github: https://www.github.com/onelonecoder
//

// TODO: 
// - create one large enemy row that is displayed as 5 rows (11 X 5)

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <thread>
#include <vector>
#include <intrin.h>    // call __debugbreak() to set a breakpoint in code
using namespace std;


int fieldWidth = 40;          // Playing field size X
int fieldHeight = 25;         // Playing field size Y 
unsigned char *field = nullptr;
int playerX = fieldWidth / 2;
int playerY = fieldHeight - 2;
//wstring enemyRow;
//int enemyRowX = 2;
//int enemyRowY = 2;
//const int enemySpeed = 1;     // NOTE(Matt): changing the enemy speed will mess up the move logic  
//wstring enemyRows[5];
//int activeEnemyRow = 0;
//int enemyRowsX = 2;
//int enemyRowsY = 6;
int fieldXOffset = 20;		  // offset of the field
int screenWidth = 80;         // Console screen size X
int screenHeight = 30;        // Console screen size Y
wchar_t *screen = new wchar_t[screenWidth * screenHeight];

struct projectile
{
	int posX;
	int posY;
	int velocity; // speed at which the projectile moves up

	projectile(int PosX, int PosY, int Speed)
	{
		posX = PosX;
		posY = PosY;
		velocity = Speed;
	}
};

struct enemyRow
{
	int posX;
	int posY;
	int speed;
	wstring enemies;
	int enemyCount = 0;
	bool enemyInversed = false;
	int enemySpeedLimit = 2;

	enemyRow(int PosX, int PosY, int Speed, wstring Enemies)
	{
		posX = PosX;
		posY = PosY;
		speed = Speed;
		enemies = Enemies;
	}
};

void KillEnemy(int posX, int posY) // TODO
{
	int fi = posY * fieldWidth + posX;
	int enemyRowIndex = enemyRowY * fieldWidth + enemyRowX;
	enemyRow[fi - enemyRowIndex] = L'.';
}

bool CanPlayerFit(int posX, int posY)
{
	if (posX >= 0 && posX < fieldWidth)
		if (posY >= 0 && posY < fieldHeight)
			if (screen[posY * screenWidth + (posX + fieldXOffset)] != ' ') return false; //if (field[fieldIndex] != 0) return false;

	return true;
}

bool CanProjectileFit(int posX, int posY)
{
	if (posX >= 0 && posX < fieldWidth)
	{
		if (posY >= 0 && posY < fieldHeight)
		{
			if (screen[posY * screenWidth + (posX + fieldXOffset)] == 'E')
			{
				KillEnemy(posX, posY);
				return false;
			}

			if (screen[posY * screenWidth + (posX + fieldXOffset)] == '#') return false;
			if (screen[posY * screenWidth + (posX + fieldXOffset)] == '|') return false;
		}
	}

	return true;
}

bool CanEnemyFit(int posX, int posY)
{
	for (int ei = 0; ei < int(enemyRow.length()); ei++)
	{
		// Get the index into the field
		int fi = posY * fieldWidth + (posX + ei);

		// In bounds check
		if ((posX + ei) >= 0 && (posX + ei) < fieldWidth)
			if (posY >= 0 && posY < fieldHeight)
			{
				if (enemyRow[ei] != L'.')
					if (field[fi] == 1 || field[fi] == 3) return false; // Check for collision with boundary or player
			}
	}
	return true;
}

void enemyMove(enemyRow er)
{
	//int enemySpeedLimit = 2;
	// Reduce the speed limit after every increase in enemyRowY (each drop down)
	// and make the floor 0.
	er.enemySpeedLimit = -(er.posY * 2) + 12;
	if (er.enemySpeedLimit <= 0) er.enemySpeedLimit = 1;

	// Enemy move logic: Start going right, if you hit the board go down one space then go left, repeat
	er.enemyCount += 1;
	if (er.enemyCount / er.enemySpeedLimit == 1) // Have the enemy rows pause, then move
	{
		er.enemyCount = 0;
		if (!er.enemyInversed)
		{

			if (CanEnemyFit(er.posX + 1, er.posY))
				er.posX += er.speed; // move right
			else
			{
				er.enemyInversed = true;
				if (CanEnemyFit(er.posX, er.posY + 1))
					er.posY += er.speed; // move down
			}
		}
		else
		{
			if (CanEnemyFit(er.posX - 1, er.posY))
				er.posX -= er.speed; // move left
			else
			{
				er.enemyInversed = false;
				if (CanEnemyFit(er.posX, er.posY + 1))
					er.posY += er.speed; // move down
			}
		}
	}
}

int main()
{
	bool key[3];
	bool running = true;
	vector<projectile> projectiles;
	int playerProjCount = 0;
	//bool enemyInversed = false;
	int enemyCount = 0;
	//int enemySpeedLimit = 2;
	vector<projectile> enemyProjectiles;
	int enemyProjCount = 0;
	int enemyProjFireRate = 10;

	// Fill the screen buffer with empty space chars
	for (int i = 0; i < screenWidth * screenHeight; i++) screen[i] = L' ';
	HANDLE console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(console);
	DWORD bytesWritten = 0;

	// Create enemy rows
	//enemyRow er1 = enemyRow(2, 2, 1, L"XXXXXXXXXXX");
	
	enemyRow = L"XXXXXXXXXXX";

	/*enemyRows[0] = L"XXXXXXXXXXX";
	enemyRows[1] = L"XXXXXXXXXXX";
	enemyRows[2] = L"XXXXXXXXXXX";
	enemyRows[3] = L"XXXXXXXXXXX";
	enemyRows[4] = L"XXXXXXXXXXX";*/

	// Create playfield buffer
	field = new unsigned char[fieldWidth * fieldHeight];
	// Assign boarder number code
	// NOTE(Matt): The integer codes are only being used to draw the field and any other 'perminate' additions 
	// to the field. Otherwise we are just drawing the charicters directly to the screen. 
	for (int x = 0; x < fieldWidth; x++)
		for (int y = 0; y < fieldHeight; y++)
			field[y * fieldWidth + x] = (x == 0 || x == fieldWidth - 1 || y == fieldHeight - 1 || y == 0) ? 3 : 0;

	while (running)
	{
		// Timing ========================================================================================
		this_thread::sleep_for(50ms); // Small Step = 1 Game Tick

		// Input =========================================================================================
		for (int i = 0; i < 4; i++)                             // R   L   SB 
			key[i] = (0x8000 & GetAsyncKeyState((unsigned char)("\x27\x25\x20"[i]))) != 0;

		// Game Logic ====================================================================================
		
		// Handle player movement
		if (key[0] && CanPlayerFit(playerX + 1, playerY)) playerX += 1;
		if (key[1] && CanPlayerFit(playerX - 1, playerY)) playerX -= 1;

		// Prevent more than one projectile from being created per spacebar press. Set the ceiling at 2.
		if (playerProjCount < 3)
		{
			if (key[2]) playerProjCount += 1;
			else playerProjCount = 0;
		}
		else
			playerProjCount -= 1;

		if (key[2] && (playerProjCount == 1))
		{
			projectile p = projectile(playerX, playerY, 1);
			projectiles.push_back(p);
		}

		// Update player projectile movement
		for (int i = 0; i < int(projectiles.size()); i++)
		{
			if (CanProjectileFit(projectiles[i].posX, projectiles[i].posY - 1))
			{
				projectiles[i].posY -= projectiles[i].velocity;
			}
			else
				projectiles.erase(projectiles.begin() + i); // destroy the projectile if it collides
		}

		

		// Update enemyRow movement
		
		//// Reduce the speed limit after every increase in enemyRowY (each drop down)
		//// and make the floor 0.
		//
		//enemySpeedLimit = -(enemyRowY * 2) + 12;
		//if (enemySpeedLimit <= 0) enemySpeedLimit = 1;

		//// Enemy move logic: Start going right, if you hit the board go down one space then go left, repeat
		//enemyCount += 1;
		//if (enemyCount / enemySpeedLimit == 1) // Have the enemy rows pause, then move
		//{
		//	enemyCount = 0;
		//	if (!enemyInversed)
		//	{

		//		if (CanEnemyFit(enemyRowX + 1, enemyRowY))
		//			enemyRowX += enemySpeed; // move right
		//		else
		//		{
		//			enemyInversed = true;
		//			if (CanEnemyFit(enemyRowX, enemyRowY + 1))
		//				enemyRowY += enemySpeed; // move down
		//		}
		//	}
		//	else
		//	{
		//		if (CanEnemyFit(enemyRowX - 1, enemyRowY))
		//			enemyRowX -= enemySpeed; // move left
		//		else
		//		{
		//			enemyInversed = false;
		//			if (CanEnemyFit(enemyRowX, enemyRowY + 1))
		//				enemyRowY += enemySpeed; // move down
		//		}
		//	}
		//}


		// Fire a projectile from the pos of each enemy. Only the first enemy row should fire projectiles
		enemyProjCount++;
		if (enemyProjCount / enemyProjFireRate == 1)
		{
			enemyProjCount = 0;
			for (int i = 0; i < int(enemyRow.size()); i++)
			{
				if (enemyRow[i] == 'X' && std::rand() % 2 == 1) // trigger this condition 1/2 of the time
				{
					projectile ep = projectile(enemyRowX + i, enemyRowY, 1);
					enemyProjectiles.push_back(ep);
				}
			}
		}
		
		for (int i = 0; i < int(enemyProjectiles.size()); i++)
		{
			if (CanProjectileFit(enemyProjectiles[i].posX, enemyProjectiles[i].posY + 1))
			{
				enemyProjectiles[i].posY += enemyProjectiles[i].velocity;
			}
			else
			{
				enemyProjectiles.erase(enemyProjectiles.begin() + i);
			}
		}

		// Display =======================================================================================

		// Draw field 
		for (int x = 0; x < fieldWidth; x++)
			for (int y = 0; y < fieldHeight; y++)
				screen[y * screenWidth + (x + fieldXOffset)] = L" @E#|"[field[y * fieldWidth + x]];

		// Draw the player
		screen[playerY * screenWidth + (playerX + fieldXOffset)] = '@';

		// Draw the player projectiles
		for (auto p : projectiles)
			screen[p.posY * screenWidth + (p.posX + fieldXOffset)] = '|';

		// Draw the enemies
		for (int ei = 0; ei < int(enemyRow.length()); ei++)
			if (enemyRow[ei] != '.')
				screen[enemyRowY * screenWidth + (enemyRowX + ei + fieldXOffset)] = 'E';

		// Draw the enemy projectiles
		for (auto p : enemyProjectiles)
			screen[p.posY * screenWidth + (p.posX + fieldXOffset)] = '|';

		// Display frame (Draw everything)
		WriteConsoleOutputCharacter(console, screen, screenWidth * screenHeight, { 0, 0 }, &bytesWritten);
	}

	return 0;
}


