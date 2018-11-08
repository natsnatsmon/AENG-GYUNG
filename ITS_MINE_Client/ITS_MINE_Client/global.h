#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
//#define SERVERIP   "127.0.0.1"
#define SERVERIP   "59.16.87.178"      // 김정현 서버 시
//#define SERVERIP   "182.210.213.139"      // 박하연 서버 시
#define SERVERPORT 8888

#define HERO_ID	0 

#define MAX_OBJECTS	300

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

#define R_PLAYER 40 // 플레이어 반지름
#define R_ITEM 20 // 아이템 반지름

#define INIT_POS -100.f
#define INIT_LIFE 5

#define SIZE_CToSPACKET 14
#define SIZE_StoCPACKET 824

//★ 게임 오버 스테이트를 두개로 나눌지 논의 필요
enum gameState {
	MainState, LobbyState, GamePlayState, GameOverState
};

//★ 플레이어 수는 임시로 4까지 해놓음. 끝날때까지도 2명만 쓰면 define으로 바꿈
enum player {
	player1, player2, player3, player4, nullPlayer = 99
};


typedef struct Vec 
{ float x; float y; };

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

	short life;

	Vec p1Pos;
	Vec p2Pos;

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
struct CInfo {
	short gameState;  // 게임 상태를 나타내는 변수 
	DWORD gameTime;  // 게임 시간 

	short life;		// 플레이어 생명

	Vec playerPos[2];
	CItemObj item[MAX_ITEMS];  // 아이템 구조체
}; 
