#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>

#define SERVERPORT 8888

#define SIZE_CToSPACKET 4
#define SIZE_SToCPACKET 1124

#define MAX_OBJECTS	100

#define MAX_PLAYERS 2
#define MAX_ITEMS 100
#define MAX_POS 720
#define MIN_POS 80

#define SIZE_PLAYER 80.f
#define SIZE_ITEM 40.f

#define INIT_POS 0.f
#define INIT_LIFE 5
#define GAMEOVER_TIME 100

#define W 0
#define A 1
#define S 2
#define D 3

enum gameState {
	MainState, LobbyState, GamePlayState, WinState, LoseState, drawState
};

enum player {
	player1, player2, nullPlayer = 99
};


struct Vec { 
	float x; float y; 
};

// Client -> Server
#pragma pack(1)
struct CtoSPacket {
	bool keyDown[4];
};
#pragma pack()

// Server -> Client
#pragma pack(1)
struct StoCPacket {
	short gameState;
	DWORD time;

	Vec p1Pos;
	Vec p2Pos;
	
	short life;

	Vec itemPos[MAX_ITEMS];
	short playerID[MAX_ITEMS];
	bool isVisible[MAX_ITEMS];
};
#pragma pack()

// 아이템 구조체
struct CItemObj {
	Vec pos;
	bool isVisible;

	short playerID;
};

// 게임 정보 구조체
typedef struct CInfo {
	short playerID;	
	
	short gameState;
	DWORD gameTime;

	short life;

	Vec playersPos[MAX_PLAYERS];
	CItemObj items[MAX_ITEMS];
}; 
