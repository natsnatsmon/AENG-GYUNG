#pragma once

#define SERVERPORT 8888
#define BUFSIZE 1024		//★ 임시 버퍼사이즈이기 때문에 논의 및 수정 필요

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

// 좌표를 위한 벡터 구조체 
typedef struct Vec {
	float x; float y;
};

// Client -> Server
struct CtoSPacket {
	Vec pos;
	bool keyDown[4];
	short life;
};

// Server -> Client
struct StoCPacket {
	Vec p1Pos;
	Vec p2Pos;

	Vec itemPos[MAX_ITEMS]; // ★ 논의 필요
	DWORD time;

	short life;
	short gameState;

};

// 플레이어 구조체
struct Player {
	short gameState; 	// 게임 상태를 나타내는 변수

	Vec pos;			// 플레이어 위치
	bool keyDown[4];	// 클라이언트 키 입력 배열
	short life;			// 플레이어 생명
};

// 아이템 구조체
struct ItemObj {
	Vec pos;			// 아이템 위치
	Vec direction;		// 아이템 발사 방향
	float velocity;		// 아이템 속도
	short playerID;		// 아이템을 먹은 플레이어의 아이디
	bool isVisible;		// 화면 표시 여부
};

// 게임 정보 구조체
struct Info {
	short connectedP;		// 연결된 플레이어의 수
	DWORD gameTime;			// 게임 시간

	Player *p[MAX_PLAYERS];	// 플레이어 구조체
	ItemObj *i[MAX_ITEMS];	// 아이템 구조체
};