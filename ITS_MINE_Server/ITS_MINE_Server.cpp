#define _WINSOCK_DEPRECATED_NO_WARNINGS		// inet_addr errror 
#pragma comment(lib, "ws2_32")

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "Global.h"
#include <iostream>


// ����ȭ�� ���� �̺�Ʈ ��ü 
HANDLE hUpdateInfoEvt[2];		// �����͸� �����Ͽ� ��� �� Info �����ϴ� �̺�Ʈ ��ü (���� RecvEvt ��ü)
HANDLE hSendEvt; // <- �̰� �Ⱦ��� �����ұ��??

// ���� ������ ����� ����(���� ���� ����)
SInfo info;

// ��Ŷ ����ü ����
CtoSPacket cTsPacket[MAX_PLAYERS];
StoCPacket sTcPacket;

// �� Ŭ�� ���� ���ŵ� ���� �������� ����
SPlayer tempPlayers[MAX_PLAYERS];
SItemObj tempItems[MAX_ITEMS];

// �� Ŭ���̾�Ʈ ���� ���� �迭
SOCKET clientSocks[2];

// Listening Socket
SOCKET listen_sock;

// �ð� ���� ����
DWORD game_startTime = 0;
DWORD game_PrevTime = 0;
DWORD item_PrevTime = 0;
//DWORD server_PrevTime = 0; // server�� �ð��̶� �ǹ̷� s_��......... .... PrevTime ��ġ�淡.....��
//DWORD send_PrevTime = 0;
DWORD tempTime = 0;

// ������ ���� Ŭ���̾�ƮID
short leaveID = -1;
// ������ �����ִ� Ŭ���̾�Ʈ ID
short leftID = -1;

int itemIndex = 0;

short restartP = 0;

//CRITICAL_SECTION cs;

void err_quit(const char *msg);
void err_display(const char *msg);
int recvn(SOCKET s, char *buf, int len, int flags);

void Init();

int RecvFromClient(SOCKET client_sock, short PlayerID);
void SendToClient();
void UpdatePosition(short playerID);

void P_I_CollisionCheck(short playerID);	//Player, Items
void P_B_CollisionCheck(short playerID);// Player, Bullets
void P_P_CollisionCheck();
void P_W_Collision();
void B_W_CollisionAndUpdate(); // �Ѿ� �̵�, �Ѿ�  �� �浹ó��.

bool GameEndCheck();

DWORD WINAPI RecvAndUpdateInfo(LPVOID arg);
DWORD WINAPI UpdatePackAndSend(LPVOID arg);

int main(int argc, char *argv[])
{
	int retval;		// ��� return value�� ���� ����

	// ����ü �ʱ�ȭ
	Init();

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// SO_REUSEADDR ���� �ɼ� ����
	BOOL optval = TRUE;
	retval = setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR)	err_quit("setsockopt()");


	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// ������ ��ſ� ����� ����
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrlen;

	HANDLE hRecvAndUpdateInfo[2];	// Ŭ���̾�Ʈ ó�� ������ �ڵ�
	HANDLE hUpdatePackAndSend;	// ���� ������ �ڵ�

	hUpdatePackAndSend = CreateThread(NULL, 0, UpdatePackAndSend, NULL, 0, NULL); // UpdatePackAndSend ���� (���� hCalculateThread)

	hSendEvt = CreateEvent(NULL, FALSE, TRUE, NULL);	// ��ȣ ����(�ٷ� ����x)

	// �׽�ƮƮƮƮƮƮƮ
	printf("���� ��Ŷ ������ : %d\n", SIZE_SToCPACKET);

	while (1)
	{
		while (info.connectedP < MAX_PLAYERS)		// �� �� ������ �ٽ� ȸ�� �� ����
		{
			// accept()
			addrlen = sizeof(clientAddr);
			clientSock = accept(listen_sock, (SOCKADDR *)&clientAddr, &addrlen);
			if (clientSock == INVALID_SOCKET) {
				err_display("accept()");
				goto EXIT;
			}


			// hRecvAndUpdateInfo ���� (���� hProccessClient)
			hRecvAndUpdateInfo[info.connectedP] = CreateThread(NULL, 0, RecvAndUpdateInfo, (LPVOID)clientSock, 0, NULL);
			// �� ���� �ڵ� ���� �̺�Ʈ ��ü ����(������ ���� �̺�Ʈ, ���� �̺�Ʈ)
			hUpdateInfoEvt[info.connectedP] = CreateEvent(NULL, FALSE, FALSE, NULL);	// ���ȣ ����(�ٷ� ����o)
			if (hRecvAndUpdateInfo[info.connectedP] == NULL) {
				closesocket(clientSock);
			}
			else {
				CloseHandle(hRecvAndUpdateInfo[info.connectedP]);		//�� ������ �ڵ鰪�� �ٷ� ���������� ���� �ʿ�
			}
		}
	}


EXIT:
	// ���� ����
	WSACleanup();
	printf("WSACleanup()\n");
	printf("���� ����\n");
	return 0;
}

// ���� �Լ� ���� ��� �� ����
void err_quit(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// ���� �Լ� ���� ���
void err_display(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

//�� ����� ���� ������ ���� �Լ�(�������� ��ŭ �ݺ� ����)
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

// �ʱ�ȭ �Լ�
void Init() {
	// info ����ü �ʱ�ȭ(�ð�)
	info.gameTime = 0;

	// info ����ü �ʱ�ȭ(info�� �÷��̾� ����ü �� �ӽ��÷��̾� ����ü ����)
	for (int i = 0; i < MAX_PLAYERS; ++i) {

		info.players[i] = { LobbyState, {INIT_POS, INIT_POS}, {false, false, false, false}, INIT_LIFE };
		tempPlayers[i] = { LobbyState, {INIT_POS, INIT_POS}, {false, false, false, false}, INIT_LIFE };
	}

	// info ����ü �ʱ�ȭ(info�� ������ ����ü �� �ӽþ����� ����ü ����)
	for (int i = 0; i < MAX_ITEMS; ++i) {
		float randX = (float)(rand() % PLAY_WIDTH) - PLAY_X;
		float randY = (float)(rand() % PLAY_WIDTH) - PLAY_Y;

		info.items[i] = { {randX, randY}, {0.f, 0.f}, 0.f, nullPlayer, false };

		tempItems[i] = { {randX, randY}, {0.f, 0.f}, 0.f, nullPlayer, false };
	}

	// C -> S Packet ����ü �ʱ�ȭ
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		cTsPacket[i] = { {false, false, false, false} };
	}

	// S -> C Packet ����ü �ʱ�ȭ
	sTcPacket.gameState = LobbyState;
	sTcPacket.time = 0;

	sTcPacket.p1Pos = { INIT_POS, INIT_POS };
	sTcPacket.p2Pos = { INIT_POS, INIT_POS };

	sTcPacket.life = INIT_LIFE;

	for (int i = 0; i < MAX_ITEMS; i++) {
		sTcPacket.itemPos[i] = info.items[i].pos;
		sTcPacket.playerID[i] = nullPlayer;
		sTcPacket.isVisible[i] = false;
	}

	// ��Ŷ ������ Ȯ��
	if (sizeof(cTsPacket[0]) != SIZE_CToSPACKET) {
		printf("define�� c -> s ��Ŷ�� ũ�Ⱑ �ٸ��ϴ�!\n");
		printf("cTsPacket�� ũ�� : %zd\t SIZE_CToSPACKET�� ũ�� : %d\n", sizeof(cTsPacket), SIZE_CToSPACKET);
		return;
	}
	if (sizeof(sTcPacket) != SIZE_SToCPACKET) {
		printf("define�� s -> c ��Ŷ�� ũ�Ⱑ �ٸ��ϴ�!\n");
		printf("sTcPacket�� ũ�� : %zd\t SIZE_SToCPACKET�� ũ�� : %d\n", sizeof(sTcPacket), SIZE_SToCPACKET);
	}
}

// ����� ���� ������ ���� �Լ�(Ŭ���̾�Ʈ�κ��� �Է¹��� ������ ����)
//�� �Լ� ���� �ٲ�. ���� ���� �� ������ �Լ����� ���ϰ� �ް� break�Ͽ� closesocket()
int RecvFromClient(SOCKET client_sock, short PlayerID)
{
	int retVal;

	SOCKET sock = client_sock;
	short playerID = PlayerID;

	SOCKADDR_IN clientAddr;
	int addrLen;

	// Ŭ���̾�Ʈ ���� �ޱ�
	addrLen = sizeof(clientAddr);
	getpeername(sock, (SOCKADDR *)&clientAddr, &addrLen);

	// ������ ��ſ� ����� ����
	char buf[SIZE_CToSPACKET + 1];
	ZeroMemory(buf, SIZE_CToSPACKET);

	// ������ �ޱ�
	retVal = recvn(sock, buf, SIZE_CToSPACKET, 0);
	if (retVal == SOCKET_ERROR) {
		err_display("������ recv()");
		return retVal;
	}

	buf[retVal] = '\0';

	// �ش��ϴ� Ŭ���̾�Ʈ ��Ŷ���ۿ� ���� ������(buf)�� ����
	memcpy(&cTsPacket[playerID], buf, SIZE_CToSPACKET);

	return 0;
}

// ����� ���� ������ �۽� �Լ�(Ŭ���̾�Ʈ���� ���ŵ� ������ �۽�)
void SendToClient()
{
	int retVal;
	char buf[SIZE_SToCPACKET];


	// ��� ���ۿ� ��Ŷ �޸� ����
	ZeroMemory(buf, SIZE_SToCPACKET);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (clientSocks[i] != 0)		// ���� �߰��� ���� �÷��̾ �ƴϸ�
		{
			sTcPacket.gameState = info.players[i].gameState;
			sTcPacket.life = info.players[i].life;

			memcpy(buf, &sTcPacket, SIZE_SToCPACKET);

			retVal = send(clientSocks[i], buf, SIZE_SToCPACKET, 0);
			if (retVal == SOCKET_ERROR)
			{
				closesocket(clientSocks[i]);
				clientSocks[i] = 0;
				printf("%d�� Ŭ�� ", i);
				err_display("send()");
				continue;
			}
		}
	}
}


void UpdatePosition(short playerID) {

	short pID = playerID;

	// ������ 1�ʸ��� ���������ִ� �κ�
	if (item_PrevTime == 0)	// g_PrevTime�� 0�̰� currTime�� ���ۺ��� �ð��� ��� �ֱ⶧���� ó�� elapsedTime�� ���� �� ���̰� �ʹ� ���� ������ �� �ִ�.
	{
		item_PrevTime = GetTickCount();
		return;
	}
	DWORD item_currTime = GetTickCount();
	DWORD item_elapsedTime = item_currTime - item_PrevTime;

	// �� �κ��� 1�ʸ��� �������� Visible�� true�� ������ִ� �κ��Դϴ�
	// ������ ����ð� - �����ð��� >= 1000ms(1��) ���� ũ��, itemIndex�� �ƽ��� ���� ������ �ε����� ���� true�� ������ִ°ſ���!
	if (item_elapsedTime >= 1000 && itemIndex < MAX_ITEMS) {
		item_PrevTime = item_currTime;
		tempItems[itemIndex].isVisible = true;
		itemIndex++;
	}

	// �� ����
	float forceX = 0.f;
	float forceY = 0.f;
	float amount = 5.f;

	float Accel_x = 0.f, Accel_y = 0.f;
	float Vel_x = 0.f, Vel_y = 0.f;


	if (cTsPacket[pID].keyDown[W])
	{
		tempPlayers[pID].pos.y = tempPlayers[pID].pos.y + amount;
	}
	if (cTsPacket[pID].keyDown[A])
	{
		tempPlayers[pID].pos.x = tempPlayers[pID].pos.x - amount;
	}
	if (cTsPacket[pID].keyDown[S])
	{
		tempPlayers[pID].pos.y = tempPlayers[pID].pos.y - amount;
	}
	if (cTsPacket[pID].keyDown[D])
	{
		tempPlayers[pID].pos.x = tempPlayers[pID].pos.x + amount;
	}
}

void P_I_CollisionCheck(short playerID)   //Player, Items
{
	float player_x = 0.f, player_y = 0.f;
	float x = 0.f, y = 0.f;
	float length = 0.f;
	float Plength = 0.f;

	for (int i = 0; i < MAX_ITEMS; i++)
	{
		length = 100.f;
		if (tempItems[i].isVisible == true && tempItems[i].playerID == nullPlayer)
		{
			x = tempPlayers[playerID].pos.x - tempItems[i].pos.x;
			y = tempPlayers[playerID].pos.y - tempItems[i].pos.y;

			length = sqrtf(x * x + y * y);
			if (length < (PLAYER_SIZE + ITEM_SIZE) / 2.f)
			{
				if (playerID == player1)
				{
					player_x = tempPlayers[player2].pos.x - tempPlayers[player1].pos.x;
					player_y = tempPlayers[player2].pos.y - tempPlayers[player1].pos.y;
				}
				else if (playerID == player2)
				{
					player_x = tempPlayers[player1].pos.x - tempPlayers[player2].pos.x;
					player_y = tempPlayers[player1].pos.y - tempPlayers[player2].pos.y;
				}
				Plength = sqrtf(player_x * player_x + player_y * player_y);

				tempItems[i].playerID = playerID;
				tempItems[i].velocity = 4.f;
				tempItems[i].direction = { player_x / Plength, player_y / Plength };
			}
		}
	}
}

void P_B_CollisionCheck(short playerID)// Player, Bullets
{
	float x = 0, y = 0;
	for (int i = 0; i < 99; i++)
	{
		if (tempItems[i].playerID != nullPlayer && playerID != tempItems[i].playerID)
		{
			x = tempItems[i].pos.x - tempPlayers[playerID].pos.x;
			y = tempItems[i].pos.y - tempPlayers[playerID].pos.y;
			if (sqrtf(x * x + y * y) < (ITEM_SIZE + PLAYER_SIZE) / 2.f)
			{
				tempPlayers[playerID].life -= 1;
				tempItems[i].direction = { 0.f, 0.f };
				tempItems[i].isVisible = false;
				tempItems[i].playerID = nullPlayer;
				tempItems[i].velocity = 0.f;
			}
		}
	}
}

void P_P_CollisionCheck()
{
	float x = 0, y = 0;
	float length = 0.f;
	x = tempPlayers[player1].pos.x - tempPlayers[player2].pos.x;
	y = tempPlayers[player1].pos.y - tempPlayers[player2].pos.y;
	length = sqrtf(x * x + y * y);
	if (length < PLAYER_SIZE)
	{
		x *= PLAYER_SIZE / 1800.f;
		y *= PLAYER_SIZE / 1800.f;
		tempPlayers[player1].pos.x += x;
		tempPlayers[player1].pos.y += y;
		tempPlayers[player2].pos.x -= x;
		tempPlayers[player2].pos.y -= y;
	}
}

void P_W_Collision()
{
	if (tempPlayers[player1].pos.x > 290.f)
		tempPlayers[player1].pos.x = 290.f;
	if (tempPlayers[player1].pos.x < -390.f)
		tempPlayers[player1].pos.x = -390.f;
	if (tempPlayers[player1].pos.y > 340.f)
		tempPlayers[player1].pos.y = 340.f;
	if (tempPlayers[player1].pos.y < -340.f)
		tempPlayers[player1].pos.y = -340.f;

	if (tempPlayers[player2].pos.x > 290.f)
		tempPlayers[player2].pos.x = 290.f;
	if (tempPlayers[player2].pos.x < -390.f)
		tempPlayers[player2].pos.x = -390.f;
	if (tempPlayers[player2].pos.y > 340.f)
		tempPlayers[player2].pos.y = 340.f;
	if (tempPlayers[player2].pos.y < -340.f)
		tempPlayers[player2].pos.y = -340.f;
}

void B_W_CollisionAndUpdate()	// �Ѿ� �̵�, �Ѿ� �� �浹ó��.
{
	for (int i = 0; i < MAX_ITEMS; i++)
	{
		if (tempItems[i].playerID != nullPlayer)
		{
			tempItems[i].pos.x += tempItems[i].direction.x * tempItems[i].velocity;
			tempItems[i].pos.y += tempItems[i].direction.y * tempItems[i].velocity;
			if (tempItems[i].pos.x > 300.f)
			{
				tempItems[i].direction = { 0.f, 0.f };
				tempItems[i].isVisible = false;
				tempItems[i].playerID = nullPlayer;
				tempItems[i].velocity = 0.f;
			}
			else if (tempItems[i].pos.x < -400.f)
			{
				tempItems[i].direction = { 0.f, 0.f };
				tempItems[i].isVisible = false;
				tempItems[i].playerID = nullPlayer;
				tempItems[i].velocity = 0.f;
			}
			else if (tempItems[i].pos.y > 350.f)
			{
				tempItems[i].direction = { 0.f, 0.f };
				tempItems[i].isVisible = false;
				tempItems[i].playerID = nullPlayer;
				tempItems[i].velocity = 0.f;
			}
			else if (tempItems[i].pos.y < -350.f)
			{
				tempItems[i].direction = { 0.f, 0.f };
				tempItems[i].isVisible = false;
				tempItems[i].playerID = nullPlayer;
				tempItems[i].velocity = 0.f;
			}

		}
	}
}


bool GameEndCheck()
{
	// ���� ����: true ��ȯ
	// ���� ������: false ��ȯ

	// �÷��̾ ���߿� �������� üũ
	if (leftID != -1)	// �����ִ� �÷��̾� ID�� �⺻��(-1)�� �ƴϸ�(= ������ ���� ���߿� �����ٸ�)
	{
		tempPlayers[leftID].gameState = WinState;
		return true;
	}

	// LifeCheck �ۼ�
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (tempPlayers[i].life == 0) {
			tempPlayers[i].gameState = LoseState;	

			for (int j = 0; j < MAX_PLAYERS; j++)
			{
				if (j != i)
					tempPlayers[j].gameState = WinState;
			}
			return true;
		}
		else
			continue;
	}

	// TimeCheck �ۼ�
	DWORD currTime = GetTickCount();		// current time in millisec
	tempTime = currTime - game_startTime;
	if (tempTime >= GAMEOVER_TIME)
	{
		// 0�� �÷��̾� �¸�
		if (tempPlayers[0].life > tempPlayers[1].life) {
			tempPlayers[0].gameState = WinState;
			tempPlayers[1].gameState = LoseState;
		}
		// 1�� �÷��̾� �¸�
		else if (tempPlayers[1].life > tempPlayers[0].life) {
			tempPlayers[0].gameState = LoseState;
			tempPlayers[1].gameState = WinState;
		}
		// ���º�
		else if (tempPlayers[0].life == tempPlayers[1].life) {
			tempPlayers[0].gameState = drawState;
			tempPlayers[1].gameState = drawState;
		}

		return true;
	}

	return false;
}


// �����͸� �����Ͽ� ��� �� Info �����ϴ� ������ (���� ProccessClient)
DWORD WINAPI RecvAndUpdateInfo(LPVOID arg)
{
	//�� ���� �� ���(�׽�Ʈ��)
	printf("RecvAndUpdateInfo ���� (���� ProccessClient)\n");

	int retval;

	// �÷��̾�ID �ο�
	short playerID;
	// �� 2���� �÷��̾� �� �� ����� ������, ���ο� ����� ������ �� leaveID�� �� �÷��̾��� playerID�� �־���.
	if (info.connectedP == 1 && leaveID != -1)		// leaveID�� �ʱⰪ�� -1. 
	{
		playerID = leaveID;
		info.connectedP++;
	}
	else
		playerID = info.connectedP++;

	if (info.connectedP == MAX_PLAYERS)
	{
		if (leftID != -1)
			leftID = -1;

		tempPlayers[playerID] = { LobbyState, {INIT_POS, INIT_POS}, {false, false, false, false}, INIT_LIFE };
		cTsPacket[playerID] = { {false, false, false, false} };

		// info ����ü �ʱ�ȭ(info�� ������ ����ü �� �ӽþ����� ����ü ����)
		for (int i = 0; i < MAX_ITEMS; ++i) {
			float randX = (float)(rand() % PLAY_WIDTH) - PLAY_X;
			float randY = (float)(rand() % PLAY_WIDTH) - PLAY_Y;

			tempItems[i] = { {randX, randY}, {0.f, 0.f}, 0.f, nullPlayer, false };
		}

		tempTime = 0;
		game_PrevTime = 0;
		item_PrevTime = 0;
		restartP = 0;
		itemIndex = 0;
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			tempPlayers[i].gameState = GamePlayState;
		}
		game_startTime = GetTickCount();
	}

	printf("[TCP ����] Ŭ���̾�Ʈ %d ����\n", playerID);

	// �������
	clientSocks[playerID] = (SOCKET)arg;

	// Ŭ�󿡰� ��� �÷��̾����� ������ ����
	send(clientSocks[playerID], (char*)&playerID, sizeof(short), 0);

	while (1) {

		// ������ �޾ƿ���
		retval = RecvFromClient(clientSocks[playerID], playerID);
		if (retval == SOCKET_ERROR)
			break;

		switch (tempPlayers[playerID].gameState)
		{
		case GamePlayState:
			// ���� �����͸� �̿��� ���
			// �浹�� ���� ������ ���� �÷��̾�ID, ������ ǥ�� ���� ���
			P_I_CollisionCheck(playerID);
			P_W_Collision();
			B_W_CollisionAndUpdate();
			P_B_CollisionCheck(playerID);
			P_P_CollisionCheck();

			// ������ ����� ����� ���� �����͸� ���� ������ ����Ǿ����� üũ
			if (!GameEndCheck())	// ������ ������ �ʾ����� update
				UpdatePosition(playerID);
			break;

		case WinState:
		case LoseState:
		case drawState:
			// ����� ����
			if (cTsPacket[playerID].keyDown[W] && cTsPacket[playerID].keyDown[A] &&
				cTsPacket[playerID].keyDown[S] && cTsPacket[playerID].keyDown[D])
			{
				restartP++;

				tempPlayers[playerID] = { LobbyState, {INIT_POS, INIT_POS}, {false, false, false, false}, INIT_LIFE };
				cTsPacket[playerID] = { {false, false, false, false} };				

				if (restartP == MAX_PLAYERS)
				{
					tempTime = 0;
					game_PrevTime = 0;
					item_PrevTime = 0;
					restartP = 0;
					itemIndex = 0;

					// info ����ü �ʱ�ȭ(info�� ������ ����ü �� �ӽþ����� ����ü ����)
					for (int i = 0; i < MAX_ITEMS; ++i) {
						float randX = (float)(rand() % PLAY_WIDTH) - PLAY_X;
						float randY = (float)(rand() % PLAY_WIDTH) - PLAY_Y;

						tempItems[i] = { {randX, randY}, {0.f, 0.f}, 0.f, nullPlayer, false };
					}

					for (int i = 0; i < MAX_PLAYERS; i++)
					{
						tempPlayers[i].gameState = GamePlayState;
					}
					game_startTime = GetTickCount();
				}

				break;
			}
			break;
		}


		// ����� �� info�� �ֱ� ��, hSendEvt �̺�Ʈ ��ȣ ���
		WaitForSingleObject(hSendEvt, 20);

		// ��� ���ŵ� ���� info�� ����
		info.players[playerID].pos.x = tempPlayers[playerID].pos.x;
		info.players[playerID].pos.y = tempPlayers[playerID].pos.y;

		info.players[playerID].gameState = tempPlayers[playerID].gameState;

		info.players[playerID].life = tempPlayers[playerID].life;

		info.gameTime = tempTime;

		for (int i = 0; i < MAX_ITEMS; i++)
			info.items[i] = tempItems[i];

		// �� ���� hUpdateInfoEvt �̺�Ʈ ��ȣ ���ֱ�
		SetEvent(hUpdateInfoEvt);
	}

	// closesocket()
	if (clientSocks[playerID] != 0)
	{
		closesocket(clientSocks[playerID]);
		clientSocks[playerID] = 0;
	}

	//�� �Ҹ� �� ���(�׽�Ʈ��)
	printf("[TCP ����] Ŭ���̾�Ʈ %d ����\n", playerID);

	// ������ �� ����
	info.connectedP--;
	printf("����� �÷��̾� �� : %d \n", info.connectedP);

	if (info.connectedP == 1)
	{
		leaveID = playerID;
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			if (i != leaveID)
				leftID = i;
		}
	}
	else if(leaveID != -1 && info.connectedP == 0)
	{
		// closesocket()
		closesocket(listen_sock);
		printf("closesocket()\n");
	}

	return 0;
}


// sTcPacket�� �����ϰ� �����͸� �����ϴ� ������ (���� CalculateThread)
DWORD WINAPI UpdatePackAndSend(LPVOID arg)
{
	// �� ���� �� ���(�׽�Ʈ��)
	printf("UpdatePackAndSend ���� (���� CalculateThread)\n");

	while (1)
	{
		// sTcPacket�� ���ŵ� ���� �ֱ� ��, hUpdateInfoEvt �̺�Ʈ ��ȣ ���
		WaitForMultipleObjects(MAX_PLAYERS, hUpdateInfoEvt, TRUE, 20);

		// ��Ŷ�� ���ŵ� info ������ ����
		sTcPacket.time = info.gameTime;

		sTcPacket.p1Pos = info.players[0].pos;
		sTcPacket.p2Pos = info.players[1].pos;

		for (int i = 0; i < MAX_ITEMS; ++i) {
			sTcPacket.itemPos[i] = info.items[i].pos;
			sTcPacket.playerID[i] = info.items[i].playerID;
			sTcPacket.isVisible[i] = info.items[i].isVisible;
		}

		if (tempPlayers[0].gameState != LobbyState || tempPlayers[1].gameState != LobbyState)
		{
			SendToClient();
		}

		SetEvent(hSendEvt);
	};

	return 0;
}

