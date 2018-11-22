//
// [ ���� �˰��� ]
// RecvAndUpdateInfo���� �����͸� �޾Ƽ� �浹üũ �� ������ ���� �� ���(hSendEvt ��ȣ ���)
// -> UpdatePackAndSend���� ���ŵ� �����͵��� ��Ŷ����ü�� ������ �� ��Ŷ ���� �� SetEvent�ϰ� ���(hUpdateInfoEvt ��ȣ ���)
// -> �ٽ� ���ƿ� RecvAndUpdateInfo���� �Ʊ� ������ �����͵��� info ����ü�� �Ѳ����� �־��� ��(��, �� ���� info ������ ������) SetEvent
// -> �� ���� �ݺ�

#define _WINSOCK_DEPRECATED_NO_WARNINGS		// inet_addr errror 
#pragma comment(lib, "ws2_32")

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "Global.h"
#include <iostream>


// ����ȭ�� ���� �̺�Ʈ ��ü 
HANDLE hUpdateInfoEvt;		// �����͸� �����Ͽ� ��� �� Info �����ϴ� �̺�Ʈ ��ü (���� RecvEvt ��ü)
HANDLE hSendEvt;			// sTcPacket�� �����ϰ� �����͸� �����ϴ� �̺�Ʈ ��ü (���� UpdateEvt ��ü)

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
DWORD g_startTime = 0;
DWORD g_PrevTime = 0;

// ������ ���� Ŭ���̾�ƮID
short leaveID = -1;
int itemIndex = 0;

CRITICAL_SECTION cs;

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

// ����ü �ʱ�ȭ �Լ�
void Init() {
	g_startTime = 0;
	g_PrevTime = 0;
	itemIndex = 0;

	// ���� ���� ����ü �ʱ�ȭ
	info.connectedP = 0;
	info.gameTime = 0;

	// ���� ���� ����ü ���� �÷��̾� ����ü ���� �� �ӽ��÷��̾� ����ü ���� �ʱ�ȭ
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		info.players[i].gameState = LobbyState;
		info.players[i].pos.x = INIT_POS;
		info.players[i].pos.y = INIT_POS;
		info.players[i].life = INIT_LIFE;
		for (int j = 0; j < 4; ++j)
			info.players[i].keyDown[j] = false;

		tempPlayers[i].gameState = info.players[i].gameState;
		tempPlayers[i].pos.x = info.players[i].pos.x;
		tempPlayers[i].pos.y = info.players[i].pos.y;
		tempPlayers[i].life = info.players[i].life;
		for (int j = 0; j < 4; ++j)
			tempPlayers[i].keyDown[j] = info.players[i].keyDown[j];
	}

	// ���� ���� ����ü ���� ������ ����ü ���� �� �ӽþ����� ����ü ���� �ʱ�ȭ
	for (int i = 0; i < MAX_ITEMS; ++i) {
		// �������� ��ǥ�� 40 ~ 760 ���̷� �����ϰ� ����
		info.items[i].pos.x = (float)(rand() % 720 + (R_ITEM * 2)) - HALF_WIDTH;
		info.items[i].pos.y = (float)(rand() % 720 + (R_ITEM * 2)) - HALF_WIDTH;
		info.items[i].direction.x = 0;
		info.items[i].direction.y = 0;
		info.items[i].velocity = 0.f;
		info.items[i].playerID = nullPlayer;
		info.items[i].isVisible = false;

		tempItems[i].pos.x = info.items[i].pos.x;
		tempItems[i].pos.y = info.items[i].pos.y;
		tempItems[i].direction.x = info.items[i].direction.x;
		tempItems[i].direction.y = info.items[i].direction.y;
		tempItems[i].velocity = info.items[i].velocity;
		tempItems[i].playerID = info.items[i].playerID;
		tempItems[i].isVisible = info.items[i].isVisible;
	}

	// C -> S Packet ����ü �ʱ�ȭ
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		//cTsPacket[i]= new CtoSPacket;
		cTsPacket[i].life = INIT_LIFE;

		for(int j = 0; j < 4; j++)
			cTsPacket[i].keyDown[j] = false;
	}

	// S -> C Packet ����ü �ʱ�ȭ
	sTcPacket.p1Pos = { INIT_POS, INIT_POS };
	sTcPacket.p2Pos = { INIT_POS, INIT_POS };

	for (int i = 0; i < MAX_ITEMS; ++i) {
		sTcPacket.itemPos[i] = { INIT_POS, INIT_POS };
		sTcPacket.playerID[i] = nullPlayer;
		sTcPacket.isVisible[i] = false;
	}

	// �������� ���� �ʱ�ȭ ����
	sTcPacket.time = 0;
	sTcPacket.gameState = LobbyState;

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
	char buf[SIZE_CToSPACKET+1];
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
	// �׽�Ʈ�� ���
	// std::cout << "SendToClient() ȣ��" << std::endl;
	//Sleep(200);

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
	if (g_PrevTime == 0)	// g_PrevTime�� 0�̰� currTime�� ���ۺ��� �ð��� ��� �ֱ⶧���� ó�� elapsedTime�� ���� �� ���̰� �ʹ� ���� ������ �� �ִ�.
	{
		g_PrevTime = GetTickCount();
		return;
	}
	DWORD currTime = GetTickCount();		// current time in millisec
	DWORD elapsedTime = currTime - g_PrevTime;
	g_PrevTime = currTime;
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

	//std::cout << "elapsed time: " << eTime << std::endl;		// �ð� Ȯ�� ���


	// �� ����
	float forceX = 0.f;
	float forceY = 0.f;
	float amount = 0.2f;

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
	int x = 0, y = 0;
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
			break;
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
		}

	}
}

// �Ͽ�
bool GameEndCheck()
{
	// ���� ����: true ��ȯ
	// ���� ������: false ��ȯ

	// LifeCheck �ۼ�
	for (int playerID = 0; playerID < MAX_PLAYERS; ++playerID) {
		if (info.players[playerID].life <= 0) {
			sTcPacket.gameState = GameOverState;
			return true;
		}
		else 
			continue;
	}

	// TimeCheck �ۼ�
	DWORD currTime = GetTickCount();		// current time in millisec

	if (currTime - g_startTime >= GAMEOVER_TIME) {
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
			g_startTime = GetTickCount();
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
		
		// ������ ����� ����� ���� �����͸� ���� ������ ����Ǿ����� üũ
		if (!GameEndCheck())	// ������ ������ �ʾ����� update�ض� (�� �Լ� ����: �Ͽ�)
			UpdatePosition(playerID);
		
		// �� ����� �� info�� �ֱ� ��, hSendEvt �̺�Ʈ ��ȣ ���
		//WaitForSingleObject(hSendEvt, INFINITE);
		
		// ������ ����� ��� ���ŵ� ���� info�� ����
		// ĳ���� ��ġ
		info.players[playerID].pos.x = tempPlayers[playerID].pos.x;
		info.players[playerID].pos.y = tempPlayers[playerID].pos.y;
		
		// �׽�Ʈ�� ���
		//std::cout << "info�� "<< playerID <<"�� �÷��̾� ��ǥ: " << info.players[playerID].pos.x << ", " << info.players[playerID].pos.y << std::endl;
		
		// �� ���� hUpdateInfoEvt �̺�Ʈ ��ȣ ���ֱ�
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

	while(1)
	{
		// sTcPacket�� ���ŵ� ���� �ֱ� ��, hUpdateInfoEvt �̺�Ʈ ��ȣ ���
		WaitForSingleObject(hUpdateInfoEvt, INFINITE);

		// ��Ŷ�� ���ŵ� info ������ ����
		// sTcPacket.time = info.gameTime;		// �� ���� �ð� �����..?

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

		//// ��Ŷ Ŭ������� �� ���´�!
		//SetEvent(hSendEvt);
	};

	return 0;         
}

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

	// �� ���� �ڵ� ���� �̺�Ʈ ��ü ����(������ ���� �̺�Ʈ, ���� �̺�Ʈ)
	hUpdateInfoEvt = CreateEvent(NULL, FALSE, FALSE, NULL);	// ���ȣ ����(�ٷ� ����o)
	if (hUpdateInfoEvt == NULL) return 1;
	hSendEvt = CreateEvent(NULL, FALSE, TRUE, NULL);	// ��ȣ ����(�ٷ� ����x)
	if (hSendEvt == NULL) return 1;

	// �׽�ƮƮƮƮƮƮƮ
	printf("���� ��Ŷ ������ : %zd\n", SIZE_SToCPACKET);

	while(1)
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