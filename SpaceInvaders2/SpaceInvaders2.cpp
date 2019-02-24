// SpaceInvaders2.cpp : Defines the entry point for the console application.
// Shoutout to Javidx9 for his Comand Line Tetris tutorial which inspired me to code this.
// His github: https://www.github.com/onelonecoder
//

// TODO: 
// - maybe replace all wstrings with strings
// - maybe limit firing rate
// - maybe add explosion effect
// - maybe add multiple enemy rows
// - maybe add barriers
// - maybe restructure code further

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <thread>
#include <vector>
#include <intrin.h>    // call __debugbreak() to set a breakpoint in code
#include <iostream>
#include <string>

int fieldWidth = 40;          // Playing field size X
int fieldHeight = 25;         // Playing field size Y 
unsigned char *field = nullptr;
int playerX = fieldWidth / 2;
int playerY = fieldHeight - 2;
int fieldXOffset = 20;		  // offset of the field
int screenWidth = 80;         // Console screen size X
int screenHeight = 30;        // Console screen size Y
wchar_t *screen = new wchar_t[screenWidth * screenHeight];
int score = 0;
bool key[3];
bool running = true;
int playerProjCount = 0;
int enemyCount = 0;
int enemyProjCount = 0;
int enemyProjFireRate = 10;
std::string gameOverCondition;

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
	int speed = 1;  // NOTE(Matt): changing the speed from 1 will mess up the move logic
	std::wstring enemies;
	int enemyCount = 0;
	bool enemyInversed = false;
	int enemySpeedLimit = 2;

	enemyRow(int PosX, int PosY, std::wstring Enemies)
	{
		posX = PosX;
		posY = PosY;
		enemies = Enemies;
	}
};

std::vector<projectile> projectiles;
std::vector<projectile> enemyProjectiles;

void KillEnemy(int posX, int posY, enemyRow *er) 
{
	int fi = posY * fieldWidth + posX;
	int erIndex = er->posY * fieldWidth + er->posX;
	er->enemies[fi - erIndex] = L'.';

	if (er->posY < 3 ) score += 100; // give more points if the enemies are killed sooner 
	else if (er->posY > 2 && er->posY < 5) score += 50;
	else score += 10;
}

bool CanPlayerFit(int posX, int posY)
{
	if (posX >= 0 && posX < fieldWidth)
		if (posY >= 0 && posY < fieldHeight)
			if (screen[posY * screenWidth + (posX + fieldXOffset)] != ' ') return false; //if (field[fieldIndex] != 0) return false;

	return true;
}

bool CanProjectileFit(int posX, int posY, std::vector<enemyRow *> *enemyRowPointers)
{
	if (posX >= 0 && posX < fieldWidth)
	{
		if (posY >= 0 && posY < fieldHeight)
		{
			if (screen[posY * screenWidth + (posX + fieldXOffset)] == 'E')
			{
				for (auto enemyRow : *enemyRowPointers)
					if (posY == enemyRow->posY) KillEnemy(posX, posY, enemyRow);
				
				return false;
			}

			if (screen[posY * screenWidth + (posX + fieldXOffset)] == '#') return false;
			if (screen[posY * screenWidth + (posX + fieldXOffset)] == '|') return false;
			if (screen[posY * screenWidth + (posX + fieldXOffset)] == '@')
			{
				running = false;
				gameOverCondition = "Player destroyed.";
				return false;
			}
		}
	}

	return true;
}

bool CanEnemyFit(int posX, int posY, enemyRow *er)
{
	for (int ei = 0; ei < int(er->enemies.length()); ei++)
	{
		// Get the index into the field
		int fi = posY * fieldWidth + (posX + ei);

		// In bounds check
		if ((posX + ei) >= 0 && (posX + ei) < fieldWidth)
			if (posY >= 0 && posY < fieldHeight)
			{
				if (er->enemies[ei] != L'.')
				{
					if (field[fi] == 1 || field[fi] == 3) return false; // Check for collision with boundary or player
				}
			}
	}
	return true;
}

void EnemyMove(enemyRow *er)
{
	//int enemySpeedLimit = 2;
	// Reduce the speed limit after every increase in enemyRowY (each drop down)
	// and make the floor 0.
	er->enemySpeedLimit = -(er->posY * 2) + 12;
	if (er->enemySpeedLimit <= 0) er->enemySpeedLimit = 1;

	// Enemy move logic: Start going right, if you hit the board go down one space then go left, repeat
	er->enemyCount += 1;
	if (er->enemyCount / er->enemySpeedLimit == 1) // Have the enemy rows pause, then move
	{
		er->enemyCount = 0;
		if (!er->enemyInversed)
		{

			if (CanEnemyFit(er->posX + 1, er->posY, er))
				er->posX += er->speed; // move right
			else
			{
				er->enemyInversed = true;
				if (CanEnemyFit(er->posX, er->posY + 1, er))
					er->posY += er->speed; // move down
			}
		}
		else
		{
			if (CanEnemyFit(er->posX - 1, er->posY, er))
				er->posX -= er->speed; // move left
			else
			{
				er->enemyInversed = false;
				if (CanEnemyFit(er->posX, er->posY + 1, er))
					er->posY += er->speed; // move down
			}
		}
	}
}

int main()
{
	// Fill the screen buffer with empty space chars
	for (int i = 0; i < screenWidth * screenHeight; i++) screen[i] = L' ';
	HANDLE console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(console);
	DWORD bytesWritten = 0;

	// Create enemy rows
	enemyRow er1 = enemyRow(2, 2, L"XXXXXXXXXXX");
	std::vector<enemyRow *> enemyRowPointers = { &er1 };

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
		std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Small Step = 1 Game Tick
		// NOTE(Matt): if you include the std namespace you can just call sleep_for like this:
		// this_thread::sleep_for(50ms)

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
			if (CanProjectileFit(projectiles[i].posX, projectiles[i].posY - 1, &enemyRowPointers))
			{
				projectiles[i].posY -= projectiles[i].velocity;
			}
			else
				projectiles.erase(projectiles.begin() + i); // destroy the projectile if it collides
		}

		

		// Update enemyRow movement
		EnemyMove(&er1);
		
		// Fire a projectile from the pos of each enemy. Only the first enemy row should fire projectiles
		enemyProjCount++;
		if (enemyProjCount / enemyProjFireRate == 1)
		{
			enemyProjCount = 0;
			for (int i = 0; i < int(er1.enemies.length()); i++)
			{
				if (er1.enemies[i] == 'X' && std::rand() % 2 == 1) // trigger this condition 1/2 of the time
				{
					projectile ep = projectile(er1.posX + i, er1.posY, 1);
					enemyProjectiles.push_back(ep);
				}
			}
		}
		
		for (int i = 0; i < int(enemyProjectiles.size()); i++)
		{
			if (CanProjectileFit(enemyProjectiles[i].posX, enemyProjectiles[i].posY + 1, &enemyRowPointers))
			{
				enemyProjectiles[i].posY += enemyProjectiles[i].velocity;
			}
			else
			{
				enemyProjectiles.erase(enemyProjectiles.begin() + i);
			}
		}

		// Endgame condition checks
		int enemyCounter = 0;
		for (auto enemyRow : enemyRowPointers)
		{
			if (enemyRow->posY == 23)
			{
				running = false; // if any enemy row hits the bottom of the field, end the game
				gameOverCondition = "Invaders have landed.";
			}
			for (auto e : enemyRow->enemies)
			{
				if (e == 'X') enemyCounter += 1;
			}
		}
			
		if (enemyCounter == 0)
		{
			running = false;  // if there are no more enemies left, end the game
			gameOverCondition = "Invaders destroyed.";
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
		for (int ei = 0; ei < int(er1.enemies.length()); ei++)
			if (er1.enemies[ei] != '.')
				screen[er1.posY * screenWidth + (er1.posX + ei + fieldXOffset)] = 'E';

		// Draw the enemy projectiles
		for (auto p : enemyProjectiles)
			screen[p.posY * screenWidth + (p.posX + fieldXOffset)] = '|';

		// Draw the score
		swprintf_s(&screen[2 * screenWidth + ((screenWidth / 2) + (fieldWidth / 2) + 2)], 16, L"SCORE: %8d", score);

		// Display frame (Draw everything)
		WriteConsoleOutputCharacter(console, screen, screenWidth * screenHeight, { 0, 0 }, &bytesWritten);
	}

	// Game End =======================================================================================

	CloseHandle(console);
	std::cout << "GAME OVER! " << gameOverCondition << std::endl;
	std::cout << "SCORE: " << score << std::endl;
	system("pause");
	while (true); // to prevent the user from accidentally closing the window before they can view their score 

	return 0;
}


