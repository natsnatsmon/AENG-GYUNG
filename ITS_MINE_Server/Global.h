#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>


#define SERVERPORT 8888

#define SIZE_CToSPACKET 4
#define SIZE_SToCPACKET 1124

#define MAX_PLAYERS 2
#define MAX_ITEMS 100
#define MAX_POS 720

#define PLAY_WIDTH 720 // 플레이 장소 크기
#define PLAY_X 410,f //  x값 한계치 - 랜덤 위치 놓을때 사용
#define PLAY_Y 360.f // y값 한계치 - 랜덤 위치 놓을때 사용
#define R_PLAYER 40.f // 플레이어 반지름
#define R_ITEM 20.f // 아이템 반지름

#define INIT_POS -100.f
#define INIT_LIFE 5
#define GAMEOVER_TIME 100000	// 100000 = 100초

#define W 0
#define A 1
#define S 2
#define D 3

#define MASS 0.07f
#define COEF_FRICT 0.0001f
#define GRAVITY 9.8f


#define PLAYER_SIZE 80.f
#define ITEM_SIZE 40.f

enum gameState {
	LobbyState = 1, GamePlayState, WinState, LoseState, drawState
};

enum player {
	player1, player2, nullPlayer = 99
};

// 좌표를 위한 벡터 구조체 
struct Vec {
	float x; float y;
};

// 플레이어 구조체
struct SPlayer {
	short gameState; 	// 게임 상태를 나타내는 변수

	Vec pos;			// 플레이어 위치
	bool keyDown[4];	// 클라이언트 키 입력 배열
	short life;			// 플레이어 생명
};

// 아이템 구조체
struct SItemObj {
	Vec pos;			// 아이템 위치
	Vec direction;		// 아이템 발사 방향
	float velocity;		// 아이템 속도
	short playerID;		// 아이템을 먹은 플레이어의 아이디
	bool isVisible;		// 화면 표시 여부
};

// 게임 정보 구조체
struct SInfo {
	short connectedP;		// 연결된 플레이어의 수
	DWORD gameTime;			// 게임 시간

	SPlayer players[MAX_PLAYERS];	// 플레이어 구조체
	SItemObj items[MAX_ITEMS];	// 아이템 구조체
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

	Vec itemPos[MAX_ITEMS];
	short playerID[MAX_ITEMS];
	bool isVisible[MAX_ITEMS];
};
#pragma pack()
