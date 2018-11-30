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
SItemObj tempItems[MAX_PLAYERS];

// �� Ŭ���̾�Ʈ ���� ���� �迭
SOCKET clientSocks[2];

// �ð� ���� ����
DWORD game_startTime = 0;
DWORD game_PrevTime = 0;
DWORD server_PrevTime = 0; // server�� �ð��̶� �ǹ̷� s_��......... .... PrevTime ��ġ�淡.....��
DWORD item_PrevTime = 0;
DWORD send_PrevTime = 0;

// ������ ���� Ŭ���̾�ƮID
short leaveID = -1;
int itemIndex = 0;

CRITICAL_SECTION cs;

void err_quit(const char *msg);
void err_display(const char *msg);
int recvn(SOCKET s, char *buf, int len, int flags);

void Init();
void DeleteAll();

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
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
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

	//hSendEvt[0] = CreateEvent(NULL, FALSE, TRUE, NULL);	// ��ȣ ����(�ٷ� ����x)
	//hSendEvt[1] = CreateEvent(NULL, FALSE, TRUE, NULL);	// ��ȣ ����(�ٷ� ����x)
	//hUpdateInfoEvt = CreateEvent(NULL, FALSE, FALSE, NULL);	// ���ȣ ����(�ٷ� ����o)

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
				break;
			}

			// hRecvAndUpdateInfo ���� (���� hProccessClient)
			hRecvAndUpdateInfo[info.connectedP] = CreateThread(NULL, 0, RecvAndUpdateInfo, (LPVOID)clientSock, 0, NULL);
			// �� ���� �ڵ� ���� �̺�Ʈ ��ü ����(������ ���� �̺�Ʈ, ���� �̺�Ʈ)
			hUpdateInfoEvt[info.connectedP] = CreateEvent(NULL, FALSE, FALSE, NULL);	// ���ȣ ����(�ٷ� ����o)
			//if (hUpdateInfoEvt == NULL) return 1;
			//hUpdatePackAndSend = CreateThread(NULL, 0, UpdatePackAndSend, (LPVOID)clientSock, 0, NULL);
			if (hRecvAndUpdateInfo[info.connectedP] == NULL) {
				closesocket(clientSock);
			}
			else {
				CloseHandle(hRecvAndUpdateInfo[info.connectedP]);		//�� ������ �ڵ鰪�� �ٷ� ���������� ���� �ʿ�
			}
		}
	}


	// delete()
	DeleteAll();

	// closesocket()
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
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
	game_startTime = 0;
	game_PrevTime = 0;
	server_PrevTime = 0;
	item_PrevTime = 0;
	send_PrevTime = 0;

	itemIndex = 0;

	// ���� ���� ����ü �ʱ�ȭ
	info.connectedP = 0;
	info.gameTime = 0;

	// ���� ���� ����ü ���� �÷��̾� ����ü ���� �� �ӽ��÷��̾� ����ü ���� �ʱ�ȭ
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		info.players[i] = { LobbyState, {INIT_POS, INIT_POS}, {false, false, false, false}, INIT_LIFE};

		tempPlayers[i] = { LobbyState, {INIT_POS, INIT_POS}, {false, false, false, false}, INIT_LIFE };
	}

	// ���� ���� ����ü ���� ������ ����ü ���� �� �ӽþ����� ����ü ���� �ʱ�ȭ
	for (int i = 0; i < MAX_ITEMS; ++i) {
		float randX = (float)(rand() % PLAY_WIDTH) - PLAY_X;
		float randY = (float)(rand() % PLAY_WIDTH) - PLAY_Y;

		info.items[i] = { {randX, randY}, {0.f, 0.f}, 0.f, nullPlayer, false };

		tempItems[i] = { {randX, randY}, {0.f, 0.f}, 0.f, nullPlayer, false };
	}

	// C -> S Packet ����ü �ʱ�ȭ
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		cTsPacket[i] = { {false, false, false, false}, INIT_LIFE };
	}

	// S -> C Packet ����ü �ʱ�ȭ
	sTcPacket.gameState = LobbyState;
	sTcPacket.time = 0;

	sTcPacket.p1Pos = { INIT_POS, INIT_POS };
	sTcPacket.p2Pos = { INIT_POS, INIT_POS };

	for (int i = 0; i < MAX_ITEMS; ++i) {
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


	// �׽�Ʈ ����Դϴ�.
	for (int i = 0; i < 10; ++i) {
		printf("item[%d].x = %f, y = %f\n", i, sTcPacket.itemPos[i].x, sTcPacket.itemPos[i].y);
	}
}

void DeleteAll() {



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

	// ���� ������ ��� (�׽�Ʈ)
	//std::cout << std::endl
	//	<< "w: " << cTsPacket[playerID]->keyDown[W] << " "
	//	<< "a: " << cTsPacket[playerID]->keyDown[A] << " "
	//	<< "s: " << cTsPacket[playerID]->keyDown[S] << " "
	//	<< "d: " << cTsPacket[playerID]->keyDown[D] << std::endl
	//	<< "[TCP ����] " << playerID << "�� Ŭ���̾�Ʈ���� ����"
	//	<< ", ��Ʈ ��ȣ = " << ntohs(clientAddr.sin_port) << " )"
	//	<< std::endl;
	return 0;
}

// ����� ���� ������ �۽� �Լ�(Ŭ���̾�Ʈ���� ���ŵ� ������ �۽�)
void SendToClient()
{
	int retVal;
	char buf[SIZE_SToCPACKET];

	// �׽�Ʈ�� ���
	//std::cout << "��Ŷ�� �÷��̾� 1 ��ǥ: " << sTcPacket->p1Pos.x << ", " << sTcPacket->p1Pos.y << std::endl;

	// ��� ���ۿ� ��Ŷ �޸� ����
	ZeroMemory(buf, SIZE_SToCPACKET);
	memcpy(buf, &sTcPacket, SIZE_SToCPACKET);

	for (int i = 0; i < info.connectedP; i++)
	{
		retVal = send(clientSocks[i], buf, SIZE_SToCPACKET, 0);
		if (retVal == SOCKET_ERROR)
		{
			std::cout << i << "�� Ŭ�� ";
			err_display("send()");
			continue;
			//exit(1);
		}
	}
}


void UpdatePosition(short playerID) {

	BOOL tempKeyDown[4] = {
		cTsPacket[playerID].keyDown[W],
		cTsPacket[playerID].keyDown[A],
		cTsPacket[playerID].keyDown[S],
		cTsPacket[playerID].keyDown[D]
	};

	// �� ��ġ ��� �� ����
	// �� elapsed time ���
	if (game_PrevTime == 0)	// g_PrevTime�� 0�̰� currTime�� ���ۺ��� �ð��� ��� �ֱ⶧���� ó�� elapsedTime�� ���� �� ���̰� �ʹ� ���� ������ �� �ִ�.
	{
		game_PrevTime = GetTickCount();
		return;
	}
	DWORD currTime = GetTickCount();		// current time in millisec
	DWORD elapsedTime = currTime - game_PrevTime;
	game_PrevTime = currTime;
	float eTime = (float)elapsedTime / 1000.f;		// ms to s

	// �� �κ��� 1�ʸ��� �������� Visible�� true�� ������ִ� �κ��Դϴ�
	// ���� eTime������ �ذ�Ǹ� ��ġ�� ������ �� �ֽ��ϴ�!
	// �ð��� �����ϰ� ��� ���ŵ� �� �ִ� ���� �־��ֽø� �˴ϴ�~~
	// ������ ����ð� - �����ð��� >= 1000ms(1��) ���� ũ��, itemIndex�� �ƽ��� ���� ������ �ε����� ���� true�� ������ִ°ſ���!
	if (elapsedTime >= 1000 && itemIndex < MAX_ITEMS) {
		EnterCriticalSection(&cs);
		info.items[itemIndex].isVisible = true;
		itemIndex++;
		LeaveCriticalSection(&cs);
	}

	//printf("elapsed time: %f", eTime);		// �ð� Ȯ�� ���


	// �� ����
	float forceX = 0.f;
	float forceY = 0.f;
	float amount = 5.f;

	float Accel_x = 0.f, Accel_y = 0.f;
	float Vel_x = 0.f, Vel_y = 0.f;


	if (tempKeyDown[W])
	{
		//forceY += amount;
		tempPlayers[playerID].pos.y = tempPlayers[playerID].pos.y + amount;
	}
	if (tempKeyDown[A])
	{
		//forceX -= amount;
		tempPlayers[playerID].pos.x = tempPlayers[playerID].pos.x - amount;
	}
	if (tempKeyDown[S])
	{
		//forceY -= amount;
		tempPlayers[playerID].pos.y = tempPlayers[playerID].pos.y - amount;
	}
	if (tempKeyDown[D])
	{
		//forceX += amount;
		tempPlayers[playerID].pos.x = tempPlayers[playerID].pos.x + amount;
	}

	// calc accel
	Accel_x = forceX / MASS;
	Accel_y = forceY / MASS;

	// calc vel
	Vel_x = Vel_x + Accel_x * eTime;
	Vel_y = Vel_y + Accel_y * eTime;

	// 0.f���� set ������������ ��� ��������!
	Accel_x = 0.f;
	Accel_y = 0.f;

	// calc friction(������) ���
	// ����ȭ ���� (���� ������ �״�� ������ ũ�Ⱑ 1�� �������ͷ� �ٲٴ� ����)
	float magVel = sqrtf(Vel_x * Vel_x + Vel_y * Vel_y);
	float velX = Vel_x / magVel;
	float velY = Vel_y / magVel;

	// �������� �ۿ��ϴ� ���⵵ ����(���� �ݴ�����̹Ƿ�)
	float fricX = -velX;
	float fricY = -velY;

	// �������� ũ��
	float friction = COEF_FRICT * MASS * GRAVITY; // ������ = ������� * m(����) * g(�߷�)

	// ������ ���� * ������ ũ��
	fricX *= friction;
	fricY *= friction;

	if (magVel < FLT_EPSILON)
	{
		Vel_x = 0.f;
		Vel_y = 0.f;
	}
	else
	{
		float accX = fricX / MASS;
		float accY = fricY / MASS;

		float afterVelX = Vel_x + accX * eTime;
		float afterVelY = Vel_y + accY * eTime;

		if (afterVelX * Vel_x < 0.f)
			Vel_x = 0.f;
		else
			Vel_x = afterVelX;

		if (afterVelY * Vel_y < 0.f)
			Vel_y = 0.f;
		else
			Vel_y = afterVelY;
	}

	//calc velocity
	Vel_x = Vel_x + Accel_x * eTime;
	Vel_y = Vel_y + Accel_y * eTime;

	// calc pos(���� ���� ��ġ���� ��������)
	tempPlayers[playerID].pos.x = tempPlayers[playerID].pos.x + Vel_x * eTime;
	tempPlayers[playerID].pos.y = tempPlayers[playerID].pos.y + Vel_y * eTime;

	//std::cout << "tempPlayers ��ǥ ��� ��: "<< tempPlayers[playerID]->pos.x << ", " << tempPlayers[playerID]->pos.y << std::endl;

	// ������ ��ġ ��� �� ����(����)

}

// ����
void P_I_CollisionCheck(short playerID)	//Player, Items
{
	float x = 0, y = 0;
	for (int i = 0; i < 99; i++)
	{
		if (playerID == player1)
		{
			x = tempPlayers[player2].pos.x - tempPlayers[player1].pos.x;
			y = tempPlayers[player2].pos.y - tempPlayers[player1].pos.y;
		}
		else if (playerID == player2)
		{
			x = tempPlayers[player1].pos.x - tempPlayers[player2].pos.x;
			y = tempPlayers[player1].pos.y - tempPlayers[player2].pos.y;
		}


		if (!tempItems[i].isVisible)
			continue;
		else if (tempItems[i].velocity == 0.f)
		{
			if (sqrtf(x * x + y * y) < (PLAYER_SIZE + ITEM_SIZE) / 2.f)
			{
				printf("�÷��̾� %d, %d��° �Ѿ� �浹!", playerID, i);
				tempItems[i].playerID = playerID;
				tempItems[i].velocity = 3.f;
				tempItems[i].direction = { x / sqrtf(x *x + y * y), y / sqrtf(x * x + y * y) };
			}
		}
	}
	// playerID�� �̿��� ��� �� tempItems[playerID]�� ���� ��!

	// ������ ���� �÷��̾�ID

	// ������ ǥ�� ����
}

void P_B_CollisionCheck(short playerID)// Player, Bullets
{
	float x = 0, y = 0;
	for (int i = 0; i < 99; i++)
	{
		if (tempItems[i].playerID != nullPlayer)
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
	x = tempPlayers[player1].pos.x - tempPlayers[player2].pos.x;
	y = tempPlayers[player1].pos.y - tempPlayers[player2].pos.y;
	if (sqrtf(x * x + y * y) < PLAYER_SIZE)
	{
		printf("Player - Player Collision!\n");
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

void B_W_CollisionAndUpdate()// �Ѿ� �̵�, �Ѿ�  �� �浹ó��.
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


// �Ͽ�
bool GameEndCheck()
{
	// ���� ����: true ��ȯ
	// ���� ������: false ��ȯ

	// LifeCheck �ۼ�
	for (int playerID = 0; playerID < MAX_PLAYERS; playerID++) {
		if (info.players[playerID].life <= 0) {
			sTcPacket.gameState = GameOverState;
			return true;
		}
		else
			continue;
	}

	// TimeCheck �ۼ�
	DWORD currTime = GetTickCount();		// current time in millisec
	info.gameTime = currTime - game_startTime;
	if (info.gameTime >= GAMEOVER_TIME) {
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
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			info.players[i].gameState = GamePlayState;
			game_startTime = GetTickCount();
		}
	}

	std::cout << "[TCP ����] Ŭ���̾�Ʈ " << playerID << "����" << std::endl;

	// �������
	SOCKET client_sock = (SOCKET)arg;
	// ��������� ���� �������� �迭�� ����
	clientSocks[playerID] = client_sock;

	while (1) {

		// ������ �޾ƿ���
		retval = RecvFromClient(client_sock, playerID);
		if (retval == SOCKET_ERROR)
			break;

		// ���� �����͸� �̿��� ���
		// �浹�� ���� ������ ���� �÷��̾�ID, ������ ǥ�� ���� ���
		P_I_CollisionCheck(playerID);	// �� ����
		P_W_Collision();

		// ������ ����� ����� ���� �����͸� ���� ������ ����Ǿ����� üũ
		if (!GameEndCheck())	// ������ ������ �ʾ����� update�ض� (�� �Լ� ����: �Ͽ�)
			UpdatePosition(playerID);

		// �� ����� �� info�� �ֱ� ��, hSendEvt �̺�Ʈ ��ȣ ���
		WaitForSingleObject(hSendEvt, 20);
		//WaitForMultipleObjects(MAX_PLAYERS, hSendEvt, TRUE, 30);

		// ������ ����� ��� ���ŵ� ���� info�� ����
		// ĳ���� ��ġ
		info.players[playerID].pos.x = tempPlayers[playerID].pos.x;
		info.players[playerID].pos.y = tempPlayers[playerID].pos.y;


		// �׽�Ʈ�� ���
		//printf("info�� %d�� �÷��̾� ��ǥ: %f, %f\n", playerID, info.players[playerID].pos.x, info.players[playerID].pos.y);
		printf("%d �� �÷��̾� ��� ��!\n", playerID);

		// �� ���� hUpdateInfoEvt �̺�Ʈ ��ȣ ���ֱ�
		//SetEvent(hUpdateInfoEvt[playerID]);
		SetEvent(hUpdateInfoEvt);


	}


	// closesocket()		�� ���� RecvFromClient()���� recv() �������� Ŭ�����ϰ� �Ǿ����� ���� �ʿ�
	closesocket(client_sock);

	//�� �Ҹ� �� ���(�׽�Ʈ��)
	printf("[TCP ����] Ŭ���̾�Ʈ %d ����\n", playerID);

	// ������ �� ����
	info.connectedP--;
	std::cout << "����� �÷��̾� �� : " << info.connectedP << std::endl;

	if (info.connectedP == 0)
		leaveID = -1;
	else
		leaveID = playerID;

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
		//WaitForSingleObject(hUpdateInfoEvt, 30);

		// ��Ŷ�� ���ŵ� info ������ ����
		sTcPacket.time = info.gameTime;		// �� ���� �ð� �����..?

		sTcPacket.gameState = info.players[0].gameState;		// �� 0�� Ŭ���̾�Ʈ�� ���¸� �����Ѵ�.(���� �ʿ��� ��)

		sTcPacket.p1Pos = info.players[0].pos;
		sTcPacket.p2Pos = info.players[1].pos;

		for (int i = 0; i < MAX_ITEMS; ++i) {
			sTcPacket.itemPos[i] = info.items[i].pos;
			sTcPacket.playerID[i] = info.items[i].playerID;
			sTcPacket.isVisible[i] = info.items[i].isVisible;
		}

		// SendToClient() �ۼ�
		SendToClient();

		if (info.connectedP != 0)
			printf("������ ���´�.\n");

		SetEvent(hSendEvt);

	};

	return 0;
}

