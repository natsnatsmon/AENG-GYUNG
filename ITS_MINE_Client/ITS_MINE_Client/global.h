#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
//#define SERVERIP   "127.0.0.1"
#define SERVERIP   "59.16.87.178"
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

#define R_PLAYER 40 // �÷��̾� ������
#define R_ITEM 20 // ������ ������

#define INIT_POS -100.f
#define INIT_LIFE 5

#define SIZE_CToSPACKET 14
#define SIZE_StoCPACKET 824

//�� ���� ���� ������Ʈ�� �ΰ��� ������ ���� �ʿ�
enum gameState {
	MainState, LobbyState, GamePlayState, GameOverState
};

//�� �÷��̾� ���� �ӽ÷� 4���� �س���. ������������ 2�� ���� define���� �ٲ�
enum player {
	player1, player2, player3, player4, nullPlayer = 99
};

#pragma pack(1)
typedef struct Vec 
{ float x; float y; };


// Client -> Server
struct CtoSPacket
{   
	Vec pos;     
	bool keyDown[4] = { 0, 0, 0, 0 };//������ ����ȭ ���ʺ��� �ð����
	short life = 5;  
}; 

// Server -> Client 
struct StoCPacket 
{  
	DWORD time;
	Vec p1Pos = { 0, 0 };
	Vec p2Pos = { 0, 0 };

	Vec itemPos[100];  
	

	short life;  
	short gameState;
};

struct ItemObj {
	Vec pos;   // ������ ��ġ 
	bool isVisible;  // ȭ�� ǥ�� ����

	short playerID;
};

struct Info {
	short gameState;  // ���� ���¸� ��Ÿ���� ���� 
	DWORD gameTime;  // ���� �ð� 

	Vec player[2];
	short life;
	ItemObj item[100];  // ������ ����ü
}; 

#pragma pack(4)

