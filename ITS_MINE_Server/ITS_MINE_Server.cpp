#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#pragma pack(1)
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "Global.h"
#include <iostream>

// ����ȭ�� ���� �̺�Ʈ ��ü 
HANDLE hRecvEvt;		// Ŭ���̾�Ʈ�κ��� ������ ���� �̺�Ʈ ��ü
HANDLE hUpdateEvt;		// ���� �� ���� �̺�Ʈ ��ü

// ���� ������ ����� ����(���� ���� ����)
Info info;

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
Player *players[MAX_PLAYERS];
ItemObj *item[MAX_ITEMS];
CtoSPacket *cTsPacket = new CtoSPacket;
StoCPacket *sTcPacket = new StoCPacket;
void Init() {

	// �÷��̾� ����ü �ʱ�ȭ
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		players[i] = new Player;
		players[i]->gameState = MainState;
		players[i]->pos.x = INIT_POS;
		players[i]->pos.y = INIT_POS;
		players[i]->life = INIT_LIFE;
		for (int j = 0; j < 4; ++j)
			players[i]->keyDown[j] = false;
	}

	// ������ ����ü �ʱ�ȭ
	for (int i = 0; i < MAX_ITEMS; ++i) {
		item[i] = new ItemObj;
		// �������� ��ǥ�� 40 ~ 760 ���̷� �����ϰ� ����
		item[i]->pos.x = (float)(rand() % 720 + (R_ITEM * 2));
		item[i]->pos.y = (float)(rand() % 720 + (R_ITEM * 2));
		item[i]->direction.x = 0;
		item[i]->direction.y = 0;
		item[i]->velocity = 0.f;
		item[i]->playerID = nullPlayer;
		item[i]->isVisible = false;
	}

	// ���� ���� ����ü �ʱ�ȭ
	info.connectedP = 0;
	info.gameTime = 0;
	for (int i = 0; i < MAX_PLAYERS; ++i)
		info.p[i] = players[i];
	for (int i = 0; i < MAX_ITEMS; ++i)
		info.i[i] = item[i];

	// C -> S Packet ����ü �ʱ�ȭ
	cTsPacket->pos.x = INIT_POS;
	cTsPacket->pos.y = INIT_POS;
	for (int i = 0; i < 4; ++i)
		cTsPacket->keyDown[i] = false;
	cTsPacket->life = INIT_LIFE;


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

// ����� ���� ������ ���� �Լ�(Ŭ���̾�Ʈ�κ��� �Է¹��� ������ ����)
//�� recvn�� ��ĥ �� �ִ��� ���� �غ���
void RecvFromClient(SOCKET client_sock, short PlayerID)
{
	int retVal;

	SOCKADDR_IN clientAddr;
	int addrLen;


	// Ŭ���̾�Ʈ ���� �ޱ�
	addrLen = sizeof(clientAddr);
	getpeername(client_sock, (SOCKADDR *)&clientAddr, &addrLen);

	char buf[SIZE_CToSPACKET + 1];

	// Ŭ���̾�Ʈ�� ������ ���
	while (1) {

		ZeroMemory(buf, BUFSIZE);

		// ������ �ޱ�
		retVal = recvn(client_sock, buf, SIZE_CToSPACKET, 0);
		if (retVal == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}

		else if (retVal == 0) { break; }


		// ���� ������ ���
		buf[retVal] = '\0';
		cTsPacket = (CtoSPacket*)buf;

		printf("\n[TCP ����] Ŭ���̾�Ʈ %d ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			info.connectedP, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

	}

}

// ����� ���� ������ �۽� �Լ�(Ŭ���̾�Ʈ���� ���ŵ� ������ �۽�)
void SendToClient(LPVOID arg, short PlayerID)
{
}


short clientID = 0;
// Ŭ���̾�Ʈ�κ��� �����͸� �ְ� �޴� ������ �Լ�
DWORD WINAPI ProccessClient(LPVOID arg)
{
	//�� ���� �� ���(�׽�Ʈ��)
	printf("ProccessClient ����\n");
	
	// Ŭ������ ������ �ޱ�
	SOCKET client_sock = (SOCKET)arg;
	const short currentThreadNum = clientID++;

	RecvFromClient(client_sock, clientID);

	switch (players[clientID]->gameState) {
	case MainState:
		break;

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


	// ������� ~~~~~ �̺�Ʈ �ѱ�
	// CalculateThread���� ����ϵ��� �Ѱ��ֱ�

	// �޴� �̺�Ʈ ��ٸ���.....

	// Ŭ������ ������ ������



	//// closesocket()	�� closesocket()�Ϸ��� �츮�� ������ ������ ��.���� �Լ� ���ڷ� arg���� socket �Ѱ��ִ°� ���� ��..
	//closesocket(client_sock);
	//info.connectedP--;
	////�� �Ҹ� �� ���(�׽�Ʈ��)
	//printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
	//	inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	return 0;
}

// ��� ���� �� ���� ������ �Լ�
DWORD WINAPI CalculateThread(LPVOID arg)
{
	//�� ���� �� ���(�׽�Ʈ��)
	printf("CalculateThread ����\n");

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
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;

	HANDLE hProccessClient[2];	// Ŭ���̾�Ʈ ó�� ������ �ڵ�
	HANDLE hCalculateThread;	// ���� ������ �ڵ�
	hCalculateThread = CreateThread(NULL, 0, CalculateThread, NULL, 0, NULL); // hCalculateThread ����

	// �� ���� �ڵ� ���� �̺�Ʈ ��ü ����(������ ���� �̺�Ʈ, ���� �̺�Ʈ)
	hRecvEvt = CreateEvent(NULL, FALSE, FALSE, NULL);	// ���ȣ ����
	if (hRecvEvt == NULL) return 1;
	hUpdateEvt = CreateEvent(NULL, FALSE, TRUE, NULL);	// ��ȣ ����
	if (hUpdateEvt == NULL) return 1;
	
	Init();

	// �׽�ƮƮƮƮƮƮƮ
	printf("cTsPacket.life : %d\n", cTsPacket->life);
	printf("��Ŷ ������ : %d\n", sizeof(StoCPacket));

	while(1)
	{
		while (info.connectedP < MAX_PLAYERS)		// �� �� ������ �ٽ� ȸ�� �� ����
		{
			// accept()
			addrlen = sizeof(clientaddr);
			client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
			if (client_sock == INVALID_SOCKET) {
				err_display("accept()");
				break;
			}
	
			// hProccessClient ����
			hProccessClient[info.connectedP] = CreateThread(NULL, 0, ProccessClient, (LPVOID)client_sock, 0, NULL);
			if (hProccessClient[info.connectedP] == NULL) { closesocket(client_sock); }
			else { CloseHandle(hProccessClient[info.connectedP]); }		//�� ������ �ڵ鰪�� �ٷ� ���������� ���� �ʿ�
	
			// ������ �� ����
			info.connectedP++;

			//�� ������ Ŭ���̾�Ʈ ���� ���(�׽�Ʈ��)
			printf("\n[TCP ����] Ŭ���̾�Ʈ %d ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
				info.connectedP, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		}
	}

	// closesocket()
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}