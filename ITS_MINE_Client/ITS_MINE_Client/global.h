#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#define SERVERIP   "127.0.0.1"		 // ������(���� �۾� ��)
//#define SERVERIP   "59.16.87.178"      // ������ ���� ��
//#define SERVERIP   "182.210.213.139"   // ���Ͽ� ���� ��
//#define SERVERIP "10.30.1.3"			 // ���ǽ� ���Ͽ� ���� ��
//#define SERVERIP "10.30.1.4"			 // ���ǽ� ������ ���� ��

#define SERVERPORT 8888

#define SIZE_CToSPACKET 6
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

#define R_PLAYER 40.f // �÷��̾� ������
#define R_ITEM 20.f // ������ ������

#define INIT_POS -100.f
#define INIT_LIFE 5
#define GAMEOVER_TIME 100

#define W 0
#define A 1
#define S 2
#define D 3


//�� ���� ���� ������Ʈ�� �ΰ��� ������ ���� �ʿ�
enum gameState {
	MainState, LobbyState, GamePlayState, WinState, LoseState
};

//�� �÷��̾�
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
	short life;
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

	Vec itemPos[MAX_ITEMS]; // �� ���� �ʿ�
	short playerID[MAX_ITEMS];
	bool isVisible[MAX_ITEMS];
};
#pragma pack()

// ������ ����ü
struct CItemObj {
	Vec pos;   // ������ ��ġ 
	bool isVisible;  // ȭ�� ǥ�� ����

	short playerID;	// �������� ���� �÷��̾�ID
};

// ���� ���� ����ü
typedef struct CInfo {
	short gameState;  // ���� ���¸� ��Ÿ���� ���� 
	DWORD gameTime;  // ���� �ð� 

	short life;		// �÷��̾� ����

	Vec playersPos[MAX_PLAYERS];
	CItemObj items[MAX_ITEMS];  // ������ ����ü
}; 
