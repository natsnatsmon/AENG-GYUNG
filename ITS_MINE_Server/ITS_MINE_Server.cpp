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
CtoSPacket *cTsPacket[MAX_PLAYERS];
StoCPacket *sTcPacket = new StoCPacket;

// �� Ŭ�� ���� ���ŵ� ���� �������� ����
SPlayer *tempPlayers[MAX_PLAYERS];
SItemObj *tempItems[MAX_PLAYERS];

// �� Ŭ���̾�Ʈ ���� ���� �迭
SOCKET clientSocks[2];

// ���� �ð� ���� ����
DWORD g_PrevTime = 0;

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


void Init() {
	// ���� ���� ����ü �ʱ�ȭ
	info.connectedP = 0;
	info.gameTime = 0;

	// ���� ���� ����ü ���� �÷��̾� ����ü ���� �� �ӽ��÷��̾� ����ü ���� �ʱ�ȭ
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		info.players[i] = new SPlayer;
		info.players[i]->gameState = LobbyState;
		info.players[i]->pos.x = INIT_POS;
		info.players[i]->pos.y = INIT_POS;
		info.players[i]->life = INIT_LIFE;
		for (int j = 0; j < 4; ++j)
			info.players[i]->keyDown[j] = false;

		tempPlayers[i] = new SPlayer;
		tempPlayers[i]->gameState = info.players[i]->gameState;
		tempPlayers[i]->pos.x = info.players[i]->pos.x;
		tempPlayers[i]->pos.y = info.players[i]->pos.y;
		tempPlayers[i]->life = info.players[i]->life;
		for (int j = 0; j < 4; ++j)
			tempPlayers[i]->keyDown[j] = info.players[i]->keyDown[j];
	}

	// ���� ���� ����ü ���� ������ ����ü ���� �� �ӽþ����� ����ü ���� �ʱ�ȭ
	for (int i = 0; i < MAX_ITEMS; ++i) {
		info.items[i] = new SItemObj;
		// �������� ��ǥ�� 40 ~ 760 ���̷� �����ϰ� ����
		info.items[i]->pos.x = (float)(rand() % 720 + (R_ITEM * 2));
		info.items[i]->pos.y = (float)(rand() % 720 + (R_ITEM * 2));
		info.items[i]->direction.x = 0;
		info.items[i]->direction.y = 0;
		info.items[i]->velocity = 0.f;
		info.items[i]->playerID = nullPlayer;
		info.items[i]->isVisible = false;

		tempItems[i] = new SItemObj;
		tempItems[i]->pos.x = info.items[i]->pos.x;
		tempItems[i]->pos.y = info.items[i]->pos.y;
		tempItems[i]->direction.x = info.items[i]->direction.x;
		tempItems[i]->direction.y = info.items[i]->direction.y;
		tempItems[i]->velocity = info.items[i]->velocity;
		tempItems[i]->playerID = info.items[i]->playerID;
		tempItems[i]->isVisible = info.items[i]->isVisible;
	}

	// C -> S Packet ����ü �ʱ�ȭ
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		cTsPacket[i]= new CtoSPacket;
		cTsPacket[i]->life = INIT_LIFE;

		for(int j = 0; j < 4; j++)
			cTsPacket[i]->keyDown[j] = false;
	}

	// S -> C Packet ����ü �ʱ�ȭ
	sTcPacket->p1Pos.x = INIT_POS;
	sTcPacket->p1Pos.y = INIT_POS;
	sTcPacket->p2Pos.x = INIT_POS;
	sTcPacket->p2Pos.y = INIT_POS;

	// �������� ���� �ʱ�ȭ ����
	sTcPacket->time = 0;
	sTcPacket->gameState = LobbyState;

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

void DeleteAll()
{
	delete sTcPacket;

	for (int i = 0; i < MAX_PLAYERS; ++i) {
		delete info.players[i];
		delete cTsPacket[i];

		delete tempItems[i];
		delete tempPlayers[i];
	}

	for (int i = 0; i < MAX_ITEMS; ++i) {
		delete info.items[i];
	}
}

// ����� ���� ������ ���� �Լ�(Ŭ���̾�Ʈ�κ��� �Է¹��� ������ ����)
//�� recvn�� ��ĥ �� �ִ��� ���� �غ���
void RecvFromClient(SOCKET client_sock, short PlayerID)
{
	int retVal;

	SOCKET sock = client_sock;
	short playerID = PlayerID;

	SOCKADDR_IN clientAddr;
	int addrLen;

	// Ŭ���̾�Ʈ ���� �ޱ�
	addrLen = sizeof(clientAddr);
	getpeername(sock, (SOCKADDR *)&clientAddr, &addrLen);

	// �׽�Ʈ ����
	char buf[SIZE_CToSPACKET+1];

	while (1) {
		ZeroMemory(buf, sizeof(CtoSPacket));

		// ������ �ޱ�
		retVal = recvn(sock, buf, SIZE_CToSPACKET, 0);
		if (retVal == SOCKET_ERROR) {
			err_display("recv()");
			closesocket(sock);
			//�� �Ҹ� �� ���(�׽�Ʈ��)
			printf("[TCP ����] Ŭ���̾�Ʈ %d ����\n", playerID);
			break;
		}
		else if (retVal == 0) { break; }

		buf[retVal] = '\0';

		// �ش��ϴ� Ŭ���̾�Ʈ ��Ŷ���ۿ� ���� ������(buf)�� ����
		memcpy(cTsPacket[playerID], buf, SIZE_CToSPACKET);


		// ���� ������ ��� (�׽�Ʈ)
		std::cout << "\nw: "<< cTsPacket[playerID]->keyDown[W] << " "
			<< "a: " << cTsPacket[playerID]->keyDown[A] << " "
			<< "s: " << cTsPacket[playerID]->keyDown[S] << " "
			<< "d: " << cTsPacket[playerID]->keyDown[D] << std::endl
			<< "life: " << cTsPacket[playerID]->life << std::endl
			<< "[TCP ����] " << playerID << "�� Ŭ���̾�Ʈ���� ����"
			<< "( IP �ּ� = " << inet_ntoa(clientAddr.sin_addr)
			<< ", ��Ʈ ��ȣ = " << ntohs(clientAddr.sin_port) << " )"
			<< std::endl;
	}
}

// ����� ���� ������ �۽� �Լ�(Ŭ���̾�Ʈ���� ���ŵ� ������ �۽�)
void SendToClient(LPVOID arg, short playerID)
{
	// �׽�Ʈ �۽�
	char buf[SIZE_StoCPACKET];

	
}


void UpdatePosition(short playerID) {

	BOOL tempKeyDown[4] = { 
		cTsPacket[playerID]->keyDown[W], 
		cTsPacket[playerID]->keyDown[A], 
		cTsPacket[playerID]->keyDown[S], 
		cTsPacket[playerID]->keyDown[D] 
	};

	// �� ��ġ ��� �� ����
	// elapsed time ���
	if (g_PrevTime == 0)	// g_PrevTime�� 0�̰� currTime�� ���ۺ��� �ð��� ��� �ֱ⶧���� ó�� elapsedTime�� ���� �� ���̰� �ʹ� ���� ������ �� �ִ�.
	{
		g_PrevTime = GetTickCount();
		return;
	}
	DWORD currTime = GetTickCount();		// current time in millisec
	DWORD elapsedTime = currTime - g_PrevTime;
	g_PrevTime = currTime;
	float eTime = (float)elapsedTime / 1000.f;		// ms to s

	// �� ����
	float forceX = 0.f;
	float forceY = 0.f;
	float amount = 4.f;

	float Accel_x = 0.f, Accel_y = 0.f;
	float Vel_x = 0.f, Vel_y = 0.f;
	

	if (tempKeyDown[W])
	{
		forceY += amount;
	}	
	if (tempKeyDown[A])
	{
		forceX -= amount;
	}
	if (tempKeyDown[S])
	{
		forceY -= amount;
	}
	if (tempKeyDown[D])
	{
		forceX += amount;
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
	tempPlayers[playerID]->pos.x = tempPlayers[playerID]->pos.x + Vel_x * eTime;
	tempPlayers[playerID]->pos.y = tempPlayers[playerID]->pos.y + Vel_y * eTime;

	
	// ������ ��ġ ��� �� ����(����)
	


}

// ����
void CollisionCheck(short playerID)
{
	// playerID�� �̿��� ��� �� tempItems[playerID]�� ���� ��!

	// ������ ���� �÷��̾�ID

	// ������ ǥ�� ����
}

// �Ͽ�
bool GameEndCheck()
{
	// ���� ����: true ��ȯ
	// ���� ������: false ��ȯ

	// LifeCheck �ۼ�
	for (int playerID = 0; playerID < MAX_PLAYERS; ++playerID) {
		if (info.players[playerID]->life <= 0) {
			sTcPacket->gameState = GameOverState;
			return true;
		}
		else 
			continue;
	}

	// TimeCheck �ۼ�
	

	return false;
}


// �����͸� �����Ͽ� ��� �� Info �����ϴ� ������ (���� ProccessClient)
DWORD WINAPI RecvAndUpdateInfo(LPVOID arg)
{
	//�� ���� �� ���(�׽�Ʈ��)
	printf("RecvAndUpdateInfo ���� (���� ProccessClient)\n");

	// ������ ���� Ȱ���� �÷��̾�ID �ο�
	short playerID = info.connectedP++;

	// �������
	SOCKET client_sock = (SOCKET)arg;
	
	// ��������� ���� �������� �迭�� ����
	clientSocks[playerID] = client_sock;

	// ������ �޾ƿ���
	RecvFromClient(client_sock, playerID);

	// ���� �����͸� �̿��� ���
	// �浹�� ���� ������ ���� �÷��̾�ID, ������ ǥ�� ���� ���
	CollisionCheck(playerID);	// �� ����

	// ������ ����� ����� ���� �����͸� ���� ������ ����Ǿ����� üũ
	if (!GameEndCheck())	// ������ ������ �ʾ����� update�ض� (�� �Լ� ����: �Ͽ�)
		UpdatePosition(playerID);
	
	// ����� �� info�� �ֱ� ��, hSendEvt �̺�Ʈ ��ȣ ���
	WaitForSingleObject(hSendEvt, INFINITE);

	// ������ ����� ��� ���ŵ� ���� info�� ����
	// ĳ���� ��ġ
	info.players[playerID]->pos.x = tempPlayers[playerID]->pos.x;
	info.players[playerID]->pos.y = tempPlayers[playerID]->pos.y;

	// ������Ʈ�� ���� switch��(�̰� ���� �����ϳ�..?)
	/*
	switch (info.players[playerID]->gameState) {
	case LobbyState:
		// �κ񿡼��� gamestate�� ����Ǿ������� gamestate ������ ������ �ȴ�
		break;

	case GamePlayState:
		// �̶��� ��Ŷ���� �� �ʿ��ϰ�
		break;

	case GameOverState:
		// �̶����� �� �޾ƾߴ�??
		break;

	default:
		printf("state ����!\n");
		return 1;
	}
	*/


	// �� ���� hUpdateInfoEvt �̺�Ʈ ��ȣ ���ֱ�
	SetEvent(hUpdateInfoEvt);


	// closesocket()
	closesocket(client_sock);
	//�� �Ҹ� �� ���(�׽�Ʈ��)
	printf("[TCP ����] Ŭ���̾�Ʈ %d ����\n", playerID);
	// ������ �� ����
	info.connectedP--;
	return 0;
}


// sTcPacket�� �����ϰ� �����͸� �����ϴ� ������ (���� CalculateThread)
DWORD WINAPI UpdatePackAndSend(LPVOID arg)
{
	// �� ���� �� ���(�׽�Ʈ��)
	printf("UpdatePackAndSend ���� (���� CalculateThread)\n");

	// sTcPacket�� ���ŵ� ���� �ֱ� ��, hUpdateInfoEvt �̺�Ʈ ��ȣ ���
	WaitForSingleObject(hUpdateInfoEvt, INFINITE);

	// ��Ŷ�� ���ŵ� info ������ ����

	
	// SendToClient() �ۼ�


	// ��Ŷ Ŭ������� �� ���´�!
	SetEvent(hSendEvt);
	
	return 0;         
}

int main(int argc, char *argv[])
{
	int retval;		// ��� return value�� ���� ����

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

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
	
	// ����ü �ʱ�ȭ
	Init();

	// �׽�ƮƮƮƮƮƮƮ
	printf("���� ��Ŷ ������ : %zd\n", sizeof(StoCPacket));

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
			if (hRecvAndUpdateInfo[info.connectedP] == NULL) {
				closesocket(clientSock);
			}
			else { 
				CloseHandle(hRecvAndUpdateInfo[info.connectedP]);		//�� ������ �ڵ鰪�� �ٷ� ���������� ���� �ʿ�
			}
	
			//�� ������ Ŭ���̾�Ʈ ���� ���(�׽�Ʈ��)
			printf("\n[TCP ����] Ŭ���̾�Ʈ %d ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
				info.connectedP, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
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