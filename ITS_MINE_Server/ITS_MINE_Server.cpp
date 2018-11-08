#define _WINSOCK_DEPRECATED_NO_WARNINGS		// inet_addr errror 
#pragma comment(lib, "ws2_32")

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "Global.h"
#include <iostream>

// ����ȭ�� ���� �̺�Ʈ ��ü 
HANDLE hRecvEvt;		// Ŭ���̾�Ʈ�κ��� ������ ���� �̺�Ʈ ��ü
HANDLE hUpdateEvt;		// ���� �� ���� �̺�Ʈ ��ü

// ���� ������ ����� ����(���� ���� ����)
SInfo info;

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

// �÷��̾�, ������ ����ü
//SPlayer *players[MAX_PLAYERS];
//SItemObj *item[MAX_ITEMS];
CtoSPacket *cTsPacket = new CtoSPacket;
StoCPacket *sTcPacket = new StoCPacket;

void Init() {
	
	// ���� ���� ����ü �ʱ�ȭ
	info.connectedP = 0;
	info.gameTime = 0;

	// ���� ���� ����ü ���� �÷��̾� ����ü ���� �ʱ�ȭ
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		info.p[i] = new SPlayer;
		info.p[i]->gameState = MainState;
		info.p[i]->pos.x = INIT_POS;
		info.p[i]->pos.y = INIT_POS;
		info.p[i]->life = INIT_LIFE;
		for (int j = 0; j < 4; ++j)
			info.p[i]->keyDown[j] = false;
	}

	/*printf("[info �ʱ�ȭ ��]\n");
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		printf("[player %d]\n", i);
		printf("state: %d\n", info.p[i]->gameState);
		printf("life: %d\n", info.p[i]->life);
		printf("pos: %f, %f\n", info.p[i]->pos.x, info.p[i]->pos.y);
		for (int j = 0; j < 4; ++j)
			printf("%d ", info.p[i]->keyDown[j]);
		printf("\n\n");
	}*/

	// ���� ���� ����ü ���� ������ ����ü ���� �ʱ�ȭ
	for (int i = 0; i < MAX_ITEMS; ++i) {
		info.i[i] = new SItemObj;
		// �������� ��ǥ�� 40 ~ 760 ���̷� �����ϰ� ����
		info.i[i]->pos.x = (float)(rand() % 720 + (R_ITEM * 2));
		info.i[i]->pos.y = (float)(rand() % 720 + (R_ITEM * 2));
		info.i[i]->direction.x = 0;
		info.i[i]->direction.y = 0;
		info.i[i]->velocity = 0.f;
		info.i[i]->playerID = nullPlayer;
		info.i[i]->isVisible = false;
	}


	// C -> S Packet ����ü �ʱ�ȭ
	for (int i = 0; i < 4; ++i)
		cTsPacket->keyDown[i] = false;

	// S -> C Packet ����ü �ʱ�ȭ
	sTcPacket->p1Pos.x = INIT_POS;
	sTcPacket->p1Pos.y = INIT_POS;
	sTcPacket->p2Pos.x = INIT_POS;
	sTcPacket->p2Pos.y = INIT_POS;

	// �������� ���� �ʱ�ȭ ����
	sTcPacket->time = 0;
	sTcPacket->life = INIT_LIFE;
	sTcPacket->gameState = MainState;

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
	delete cTsPacket;
	delete sTcPacket;

	for (int i = 0; i < MAX_PLAYERS; ++i) {
		delete info.p[i];
	}

	for (int i = 0; i < MAX_ITEMS; ++i) {
		delete info.i[i];
	}
}

// ����� ���� ������ ���� �Լ�(Ŭ���̾�Ʈ�κ��� �Է¹��� ������ ����)
//�� recvn�� ��ĥ �� �ִ��� ���� �غ���
void RecvFromClient(SOCKET client_sock, short PlayerID)
{
	int retVal;

	SOCKADDR_IN clientAddr;
	int addrLen;

	SOCKET sock = client_sock;

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
			break;
		}
		else if (retVal == 0) { break; }

		buf[retVal] = '\0';

		// ���� buf �� ��ȯ �� ����
		cTsPacket = (CtoSPacket*)buf;

		// ���� ������ ��� (�׽�Ʈ)
		std::cout << "\nw: "<< cTsPacket->keyDown[0] << " " 
			<< "a: " << cTsPacket->keyDown[1] << " "
			<< "s: " << cTsPacket->keyDown[2] << " "
			<< "d: " << cTsPacket->keyDown[3] << std::endl;
		std::cout << "[TCP ����] " << info.connectedP << "�� Ŭ���̾�Ʈ���� ����"
			<< "( IP �ּ� = " << inet_ntoa(clientAddr.sin_addr)
			<< ", ��Ʈ ��ȣ = " << ntohs(clientAddr.sin_port) << " )"
			<< std::endl;
	}
}

// ����� ���� ������ �۽� �Լ�(Ŭ���̾�Ʈ���� ���ŵ� ������ �۽�)
void SendToClient(LPVOID arg, short PlayerID)
{
	// �׽�Ʈ �۽�
	char buf[SIZE_StoCPACKET];


}


// Ŭ���̾�Ʈ�κ��� �����͸� �ְ� �޴� ������ �Լ�
DWORD WINAPI ProccessClient(LPVOID arg)
{
	//�� ���� �� ���(�׽�Ʈ��)
	printf("ProccessClient ����\n");

	// ������ ���� Ȱ���� �÷��̾�ID �ο�
	short playerID = info.connectedP++;

	// Ŭ������ ������ �ޱ�
	SOCKET client_sock = (SOCKET)arg;

	RecvFromClient(client_sock, playerID);

	// �� ���� recv �̺�Ʈ ��ȣ ���ֱ�
	// �ٷ� ���� �ٿ� waitfor() �ۼ�

	switch (info.p[playerID]->gameState) {
	case LobbyState:
		break;

	case GamePlayState:
		break;

	case GameOverState:
		break;

	default:
		printf("state ����!\n");
		return 1;
	}



	// �޴� �̺�Ʈ ��ٸ���.....

	// Ŭ������ ������ ������



	// closesocket()	�� closesocket()�Ϸ��� �츮�� ������ ������ ��.���� �Լ� ���ڷ� arg���� socket �Ѱ��ִ°� ���� ��..
	closesocket(client_sock);
	//�� �Ҹ� �� ���(�׽�Ʈ��)
	printf("[TCP ����] Ŭ���̾�Ʈ %d ����\n", info.connectedP);
	// ������ �� ����
	info.connectedP--;
	return 0;
}

void UpdatePosition() {

	BOOL tempKeyDown[4] = { cTsPacket->keyDown[0], cTsPacket->keyDown[1], cTsPacket->keyDown[2], cTsPacket->keyDown[3] };
	
	// ���


	// �� ��ġ, ���� ��ġ(�ٸ� �����忡�� ��ġ���� �˾ƿ;� ��), ������ ��ġ ���




}

// ����
void CollisionCheck()
{

}

// �Ͽ�
bool GameEndCheck()
{
	// LifeCheck �ۼ�

	// ����� 0�̶�� gameState�� �����ϱ� 
	return false;
}

// ��� ���� �� ���� ������ �Լ�
DWORD WINAPI CalculateThread(LPVOID arg)
{
	// �� ���� �� ���(�׽�Ʈ��)
	printf("CalculateThread ����\n");


	// "������ �� ����" �̺�Ʈ ��ȣ üũ(waitfor())
	//   �� ���� �ۼ��ؾ���.

	// �浹�� ���� �� ����, ������ ���� �÷��̾�ID, ������ ǥ�� ���� ����Ͽ� sTcPacket�� �� �� ����
	CollisionCheck();	// �� ����

	// ������ ����� ����� ���� ������ ����Ǿ����� üũ�Ͽ� 
	if(!GameEndCheck())	// ������ ������ �ʾ����� update�ض� (�� �Լ� ����: �Ͽ�)
		UpdatePosition();

	
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

	HANDLE hProccessClient[2];	// Ŭ���̾�Ʈ ó�� ������ �ڵ�
	HANDLE hCalculateThread;	// ���� ������ �ڵ�

	hCalculateThread = CreateThread(NULL, 0, CalculateThread, NULL, 0, NULL); // hCalculateThread ����

	// �� ���� �ڵ� ���� �̺�Ʈ ��ü ����(������ ���� �̺�Ʈ, ���� �̺�Ʈ)
	hRecvEvt = CreateEvent(NULL, FALSE, FALSE, NULL);	// ���ȣ ����(�ٷ� ����o)
	if (hRecvEvt == NULL) return 1;
	hUpdateEvt = CreateEvent(NULL, FALSE, TRUE, NULL);	// ��ȣ ����(�ٷ� ����x)
	if (hUpdateEvt == NULL) return 1;
	
	// ����ü �ʱ�ȭ
	Init();

	//UpdateValues();

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
	
			// hProccessClient ����
			hProccessClient[info.connectedP] = CreateThread(NULL, 0, ProccessClient, (LPVOID)clientSock, 0, NULL);
			if (hProccessClient[info.connectedP] == NULL) { 
				closesocket(clientSock);
			}
			else { 
				CloseHandle(hProccessClient[info.connectedP]);		//�� ������ �ڵ鰪�� �ٷ� ���������� ���� �ʿ�
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