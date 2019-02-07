// SpaceInvaders2.cpp : Defines the entry point for the console application.
// Shoutout to Javidx9 for his Comand Line Tetris tutorial which inspired me to code this.
// His github: https://www.github.com/onelonecoder
//

// TODO: Handle spaces in the enemyRow. Enemy rows where the leading edge
// is a space ('.') are phasing through the boundary.

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <thread>
#include <vector>
#include <intrin.h>    // call __debugbreak() in code to set a breakpoint in code
using namespace std;


int fieldWidth = 40;          // Playing field size X
int fieldHeight = 25;         // Playing field size Y 
unsigned char *field = nullptr;
int playerX = fieldWidth / 2;
int playerY = fieldHeight - 2;
wstring enemyRow;
int enemyRowX = 2;
int enemyRowY = 2;
int enemySpeed = 1;

struct projectile
{
	int posX;
	int posY;
	int velocity; // speed at which the projectile moves up (it can only move up)

	projectile(int PosX, int PosY, int Speed)
	{
		posX = PosX;
		posY = PosY;
		velocity = Speed;
	}
};

void KillEnemy(int posX, int posY)
{
	for (int ei = 0; ei < enemyRow.length(); ei++)
	{
		int fi = posY * fieldWidth + posX;
		int enemyRowIndex = enemyRowY * fieldWidth + enemyRowX;
		enemyRow[fi - enemyRowIndex] = L'.';
	}
}

bool CanFit(int posX, int posY)
{
	int fieldIndex = posY * fieldWidth + posX;
	if (posX >= 0 && posX < fieldWidth)
		if (posY >= 0 && posY < fieldHeight)
			if (field[fieldIndex] != 0) return false;

	return true;
}


bool CanProjectileFit(int posX, int posY)
{
	int fieldIndex = posY * fieldWidth + posX;
	if (posX >= 0 && posX < fieldWidth)
		if (posY >= 0 && posY < fieldHeight)
			if (field[fieldIndex] == 2)
			{
				KillEnemy(posX, posY); // get the index of the enemyRow, turn that element into a '.'
				return false;
			}
			else if (field[fieldIndex] != 0) return false;

			return true;
}

bool CanEnemyFit(int posX, int posY)
{
	for (int ei = 0; ei < enemyRow.length(); ei++)
	{
		// Get the index into the field
		int fi = posY * fieldWidth + (posX + ei);

		// In bounds check
		if ((posX + ei) >= 0 && (posX + ei) < fieldWidth)
			if (posY >= 0 && posY < fieldHeight)
				if (enemyRow[ei] != L'.')
					if (field[fi] == 1 || field[fi] == 3) return false; // Check for collision with boundary or player
	}

	return true;
}


void UpdateField(vector<projectile> *projectiles)
{
	for (int x = 0; x < fieldWidth; x++)
		for (int y = 0; y < fieldHeight; y++)
		{
			field[y * fieldWidth + x] = 0; // Set all field numbers back to 0 (blank space)

			if (x == 0 || y == 0 || x == fieldWidth - 1 || y == fieldHeight - 1) // Set the boundary
				field[y * fieldWidth + x] = 3;

			if (x == playerX && y == playerY) // Set the player
				field[y * fieldWidth + x] = 1;

			for (int i = 0; i < enemyRow.length(); i++) // Set the enemy row
			{
				if (x == enemyRowX + i && y == enemyRowY)
				{
					if (enemyRow[i] == L'.')
					{
						field[y * fieldWidth + x] = 0;
					}
					else field[y * fieldWidth + x] = 2;
				}
			}

			if (projectiles->size() > 0) // Set the projectiles
				for each (auto p in *projectiles)
					if (x == p.posX && y == p.posY) field[y * fieldWidth + x] = 4;
		}
}

int main()
{
	bool key[3];
	int screenWidth = 80;         // Console screen size X
	int screenHeight = 30;        // Console screen size Y
	int fieldXOffset = 20;
	bool running = true;
	vector<projectile> projectiles;
	int projCount = 0;
	//wstring enemyRows[5];
	bool enemyInversed = false;
	int enemyCount = 0;

	// Create screen buffer
	wchar_t *screen = new wchar_t[screenWidth * screenHeight];
	for (int i = 0; i < screenWidth * screenHeight; i++) screen[i] = L' ';
	HANDLE console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(console);
	DWORD bytesWritten = 0;

	enemyRow = L"XXXXXXXX.XX";

	// Create playfield buffer
	field = new unsigned char[fieldWidth * fieldHeight];

	while (running)
	{
		// Timing ========================================================================================
		this_thread::sleep_for(50ms); // Small Step = 1 Game Tick

									  // Input ========================================================================================
		for (int i = 0; i < 4; i++)                             // R   L   SB 
			key[i] = (0x8000 & GetAsyncKeyState((unsigned char)("\x27\x25\x20"[i]))) != 0;

		// Game Logic ========================================================================================

		// Handle player movement
		if (key[0] && CanFit(playerX + 1, playerY)) playerX += 1;
		if (key[1] && CanFit(playerX - 1, playerY)) playerX -= 1;

		// Prevent more than one projectile from being created per spacebar press. Set the ceiling at 2.
		if (projCount < 3)
		{
			if (key[2]) projCount += 1;
			else projCount = 0;
		}
		else
			projCount -= 1;

		if (key[2] && (projCount == 1))
		{
			projectile p = projectile(playerX, playerY, 1);
			projectiles.push_back(p);
		}

		// Update projectile movement
		for (int i = 0; i < projectiles.size(); i++)
		{
			if (CanProjectileFit(projectiles[i].posX, projectiles[i].posY - 1))
			{
				projectiles[i].posY -= projectiles[i].velocity;
			}
			else
				projectiles.erase(projectiles.begin() + i); // destroy the projectile if it collides
		}

		// Update enemyRow movement
		// Enemy move logic: Start going right, if you hit the board go down one space then go left, repeat
		enemyCount += 1;
		if (enemyCount / 10 == 1) // Have the enemy rows pause, then move
		{
			enemyCount = 0;
			if (!enemyInversed)
			{

				if (CanEnemyFit(enemyRowX + 1, enemyRowY))
					enemyRowX += enemySpeed; // move right
				else
				{
					enemyInversed = true;
					if (CanEnemyFit(enemyRowX, enemyRowY + 1))
						enemyRowY += enemySpeed; // move down
				}
			}
			else
			{
				if (CanEnemyFit(enemyRowX - 1, enemyRowY))
					enemyRowX -= enemySpeed; // move left
				else
				{
					enemyInversed = false;
					if (CanEnemyFit(enemyRowX, enemyRowY + 1))
						enemyRowY += enemySpeed; // move down
				}
			}
		}

		// Display ========================================================================================

		// Update the field
		UpdateField(&projectiles);

		// Draw field 
		for (int x = 0; x < fieldWidth; x++)
			for (int y = 0; y < fieldHeight; y++)
				screen[y * screenWidth + (x + fieldXOffset)] = L" @E#|"[field[y * fieldWidth + x]];

		// Display frame (Draw everything)
		WriteConsoleOutputCharacter(console, screen, screenWidth * screenHeight, { 0, 0 }, &bytesWritten);
	}

	return 0;
}


