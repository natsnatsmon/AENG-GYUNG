#pragma once

#define SERVERPORT 8888
#define BUFSIZE 1024		//�� �ӽ� ���ۻ������̱� ������ ���� �� ���� �ʿ�

#define MAX_PLAYERS 2
#define MAX_ITEMS 100


// ��ǥ�� ���� ���� ����ü 
typedef struct Vec {
	float x; float y;
};

// Client -> Server
typedef struct CtoSPacket {
	Vec pos;
	bool keyDown[4];
	short life;
};

// Server -> Client
typedef struct StoCPacket {
	Vec p1Pos;
	Vec p2Pos;

	Vec itemPos[MAX_ITEMS];
	DWORD time;

	short life;
	short gameState;

};

// �÷��̾� ����ü
typedef struct Player {
	short gameState; 	// ���� ���¸� ��Ÿ���� ����

	Vec pos;			// �÷��̾� ��ġ
	bool keyDown[4];	// Ŭ���̾�Ʈ Ű �Է� �迭
	short life;			// �÷��̾� ����
};

// ������ ����ü
typedef struct ItemObj {
	Vec pos;			// ������ ��ġ
	Vec direction;		// ������ �߻� ����
	float velocity;		// ������ �ӵ�
	short playerID;		// �������� ���� �÷��̾��� ���̵�
	bool isVisible;		// ȭ�� ǥ�� ����
};

// ���� ���� ����ü
typedef struct Info {
	short connectedP;		// ����� �÷��̾��� ��
	DWORD gameTime;			// ���� �ð�

	Player p[MAX_PLAYERS];	// �÷��̾� ����ü
	ItemObj i[MAX_ITEMS];	// ������ ����ü
};