#pragma once

#define SERVERPORT 8888

#define SIZE_CToSPACKET 6
#define SIZE_SToCPACKET 1122

#define MAX_PLAYERS 2
#define MAX_ITEMS 100
#define MAX_POS 720

#define HALF_WIDTH 450.f // ȭ�� ������
#define R_PLAYER 40.f // �÷��̾� ������
#define R_ITEM 20.f // ������ ������

#define INIT_POS -100.f
#define INIT_LIFE 5
#define GAMEOVER_TIME 18300
// 183000 = 183�� 3�ʴ� �غ�ð�������

#define W 0
#define A 1
#define S 2
#define D 3

#define MASS 0.07f
#define COEF_FRICT 0.0001f
#define GRAVITY 9.8f


#define PLAYER_SIZE 80.f
#define ITEM_SIZE 40.f

//�� ���� ���� ������Ʈ�� �ΰ��� ������ ���� �ʿ�
enum gameState {
	LobbyState = 1, GamePlayState, GameOverState
};

//�� �÷��̾� ���� �ӽ÷� 4���� �س���. ������������ 2�� ���� define���� �ٲ�
enum player {
	player1, player2, player3, player4, nullPlayer = 99
};

// ��ǥ�� ���� ���� ����ü 
struct Vec {
	float x; float y;
};

// �÷��̾� ����ü
struct SPlayer {
	short gameState; 	// ���� ���¸� ��Ÿ���� ����

	Vec pos;			// �÷��̾� ��ġ
	bool keyDown[4];	// Ŭ���̾�Ʈ Ű �Է� �迭
	short life;			// �÷��̾� ����
};

// ������ ����ü
struct SItemObj {
	Vec pos;			// ������ ��ġ
	Vec direction;		// ������ �߻� ����
	float velocity;		// ������ �ӵ�
	short playerID;		// �������� ���� �÷��̾��� ���̵�
	bool isVisible;		// ȭ�� ǥ�� ����
};

// ���� ���� ����ü
struct SInfo {
	short connectedP;		// ����� �÷��̾��� ��
	DWORD gameTime;			// ���� �ð�

	SPlayer players[MAX_PLAYERS];	// �÷��̾� ����ü
	SItemObj items[MAX_ITEMS];	// ������ ����ü
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

	Vec itemPos[MAX_ITEMS];
	short playerID[MAX_ITEMS];
	bool isVisible[MAX_ITEMS];
};
#pragma pack()
