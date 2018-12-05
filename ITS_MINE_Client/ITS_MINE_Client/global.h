#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>

#define SERVERPORT 8888

#define SIZE_CToSPACKET 4
#define SIZE_SToCPACKET 1124

#define MAX_OBJECTS	100

#define SHOOT_NONE	-1
#define SHOOT_LEFT	1
#define SHOOT_RIGHT	2
#define SHOOT_UP	3
#define SHOOT_DOWN	4

#define KIND_HERO	0
#define KIND_BULLET	1

#define MAX_PLAYERS 2
#define MAX_ITEMS 100
#define MAX_POS 720
#define MIN_POS 80

#define R_PLAYER 40.f // 플레이어 반지름
#define R_ITEM 20.f // 아이템 반지름

#define INIT_POS -100.f
#define INIT_LIFE 5
#define GAMEOVER_TIME 100

#define W 0
#define A 1
#define S 2
#define D 3


//★ 게임 오버 스테이트를 두개로 나눌지 논의 필요
enum gameState {
	MainState, LobbyState, GamePlayState, WinState, LoseState, drawState
};

//★ 플레이어
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

#pragma pack(1)
// Server -> Client
struct StoCPacket {
	short gameState;
	DWORD time;

	Vec p1Pos;
	Vec p2Pos;
	
	short life;

	Vec itemPos[MAX_ITEMS]; // ★ 논의 필요
	short playerID[MAX_ITEMS];
	bool isVisible[MAX_ITEMS];
};
#pragma pack()

// 아이템 구조체
struct CItemObj {
	Vec pos;   // 아이템 위치 
	bool isVisible;  // 화면 표시 여부

	short playerID;	// 아이템을 먹은 플레이어ID
};

// 게임 정보 구조체
typedef struct CInfo {
	short playerID;	//playerID
	
	short gameState;  // 게임 상태를 나타내는 변수 
	DWORD gameTime;  // 게임 시간 

	short life;		// 플레이어 생명

	Vec playersPos[MAX_PLAYERS];
	CItemObj items[MAX_ITEMS];  // 아이템 구조체
}; 
