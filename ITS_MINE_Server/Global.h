#pragma once

#define SERVERPORT 8888
#define BUFSIZE 1024		//�� �ӽ� ���ۻ������̱� ������ ���� �� ���� �ʿ�

#define MAX_PLAYERS 2
#define MAX_ITEMS 100
#define MAX_POS 720

#define R_PLAYER 40 // �÷��̾� ������
#define R_ITEM 20 // ������ ������

#define INIT_POS -100.f
#define INIT_LIFE 5

//#define SIZE_CToSPACKET 14
//#define SIZE_StoCPACKET 1124

//�� ���� ���� ������Ʈ�� �ΰ��� ������ ���� �ʿ�
enum gameState {
	MainState, LobbyState, GamePlayState, GameOverState
};

//�� �÷��̾� ���� �ӽ÷� 4���� �س���. ������������ 2�� ���� define���� �ٲ�
enum player {
	player1, player2, player3, player4, nullPlayer = 99
};

// ��ǥ�� ���� ���� ����ü 
struct Vec {
	float x; float y;
};

// Client -> Server
#pragma pack(1)
struct CtoSPacket {
	Vec pos;
	bool keyDown[4];
	short life;
};
#pragma pack()

#pragma pack(1)
// Server -> Client
struct StoCPacket {
	Vec p1Pos;
	Vec p2Pos;

	Vec itemPos[MAX_ITEMS]; // �� ���� �ʿ�
	short characterID[100];
	bool isVisible[100];

	DWORD time;

	short life;
	short gameState;
};
#pragma pack()

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
struct Info {
	short connectedP;		// ����� �÷��̾��� ��
	DWORD gameTime;			// ���� �ð�

	SPlayer *p[MAX_PLAYERS];	// �÷��̾� ����ü
	SItemObj *i[MAX_ITEMS];	// ������ ����ü
};