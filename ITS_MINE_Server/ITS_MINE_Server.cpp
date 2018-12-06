#define _WINSOCK_DEPRECATED_NO_WARNINGS		// inet_addr errror 
#pragma comment(lib, "ws2_32")

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "Global.h"
#include <iostream>


// 동기화를 위한 이벤트 객체 
HANDLE hUpdateInfoEvt[2];		// 데이터를 수신하여 계산 및 Info 갱신하는 이벤트 객체 (이전 RecvEvt 객체)
HANDLE hSendEvt; // <- 이거 안쓰면 삭제할까요??

// 서버 구동에 사용할 변수(게임 관련 정보)
SInfo info;

// 패킷 구조체 선언
CtoSPacket cTsPacket[MAX_PLAYERS];
StoCPacket sTcPacket;

// 각 클라에 대해 갱신된 값을 갖고있을 변수
SPlayer tempPlayers[MAX_PLAYERS];
SItemObj tempItems[MAX_ITEMS];

// 각 클라이언트 전용 소켓 배열
SOCKET clientSocks[2];

// Listening Socket
SOCKET listen_sock;

// 시간 저장 변수
DWORD game_startTime = 0;
DWORD game_PrevTime = 0;
DWORD item_PrevTime = 0;
//DWORD server_PrevTime = 0; // server의 시간이란 의미로 s_를......... .... PrevTime 겹치길래.....ㅜ
//DWORD send_PrevTime = 0;
DWORD tempTime = 0;

// 서버를 떠난 클라이언트ID
short leaveID = -1;
// 서버에 남아있는 클라이언트 ID
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
void B_W_CollisionAndUpdate(); // 총알 이동, 총알  벽 충돌처리.

bool GameEndCheck();

DWORD WINAPI RecvAndUpdateInfo(LPVOID arg);
DWORD WINAPI UpdatePackAndSend(LPVOID arg);

int main(int argc, char *argv[])
{
	int retval;		// 모든 return value를 받을 변수

	// 구조체 초기화
	Init();

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// SO_REUSEADDR 소켓 옵션 설정
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

	// 데이터 통신에 사용할 변수
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrlen;

	HANDLE hRecvAndUpdateInfo[2];	// 클라이언트 처리 스레드 핸들
	HANDLE hUpdatePackAndSend;	// 연산 스레드 핸들

	hUpdatePackAndSend = CreateThread(NULL, 0, UpdatePackAndSend, NULL, 0, NULL); // UpdatePackAndSend 생성 (이전 hCalculateThread)

	hSendEvt = CreateEvent(NULL, FALSE, TRUE, NULL);	// 신호 상태(바로 시작x)

	// 테스트트트트트트트
	printf("보낼 패킷 사이즈 : %d\n", SIZE_SToCPACKET);

	while (1)
	{
		while (info.connectedP < MAX_PLAYERS)		// ★ 이 조건은 다시 회의 후 결정
		{
			// accept()
			addrlen = sizeof(clientAddr);
			clientSock = accept(listen_sock, (SOCKADDR *)&clientAddr, &addrlen);
			if (clientSock == INVALID_SOCKET) {
				err_display("accept()");
				goto EXIT;
			}


			// hRecvAndUpdateInfo 생성 (이전 hProccessClient)
			hRecvAndUpdateInfo[info.connectedP] = CreateThread(NULL, 0, RecvAndUpdateInfo, (LPVOID)clientSock, 0, NULL);
			// 두 개의 자동 리셋 이벤트 객체 생성(데이터 수신 이벤트, 갱신 이벤트)
			hUpdateInfoEvt[info.connectedP] = CreateEvent(NULL, FALSE, FALSE, NULL);	// 비신호 상태(바로 시작o)
			if (hRecvAndUpdateInfo[info.connectedP] == NULL) {
				closesocket(clientSock);
			}
			else {
				CloseHandle(hRecvAndUpdateInfo[info.connectedP]);		//★ 스레드 핸들값을 바로 삭제할지도 논의 필요
			}
		}
	}


EXIT:
	// 윈속 종료
	WSACleanup();
	printf("WSACleanup()\n");
	printf("서버 종료\n");
	return 0;
}

// 소켓 함수 오류 출력 후 종료
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

// 소켓 함수 오류 출력
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

//★ 사용자 정의 데이터 수신 함수(고정길이 만큼 반복 수신)
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

// 초기화 함수
void Init() {
	// info 구조체 초기화(시간)
	info.gameTime = 0;

	// info 구조체 초기화(info내 플레이어 구조체 및 임시플레이어 구조체 정보)
	for (int i = 0; i < MAX_PLAYERS; ++i) {

		info.players[i] = { LobbyState, {INIT_POS, INIT_POS}, {false, false, false, false}, INIT_LIFE };
		tempPlayers[i] = { LobbyState, {INIT_POS, INIT_POS}, {false, false, false, false}, INIT_LIFE };
	}

	// info 구조체 초기화(info내 아이템 구조체 및 임시아이템 구조체 정보)
	for (int i = 0; i < MAX_ITEMS; ++i) {
		float randX = (float)(rand() % PLAY_WIDTH) - PLAY_X;
		float randY = (float)(rand() % PLAY_WIDTH) - PLAY_Y;

		info.items[i] = { {randX, randY}, {0.f, 0.f}, 0.f, nullPlayer, false };

		tempItems[i] = { {randX, randY}, {0.f, 0.f}, 0.f, nullPlayer, false };
	}

	// C -> S Packet 구조체 초기화
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		cTsPacket[i] = { {false, false, false, false} };
	}

	// S -> C Packet 구조체 초기화
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

	// 패킷 사이즈 확인
	if (sizeof(cTsPacket[0]) != SIZE_CToSPACKET) {
		printf("define된 c -> s 패킷의 크기가 다릅니다!\n");
		printf("cTsPacket의 크기 : %zd\t SIZE_CToSPACKET의 크기 : %d\n", sizeof(cTsPacket), SIZE_CToSPACKET);
		return;
	}
	if (sizeof(sTcPacket) != SIZE_SToCPACKET) {
		printf("define된 s -> c 패킷의 크기가 다릅니다!\n");
		printf("sTcPacket의 크기 : %zd\t SIZE_SToCPACKET의 크기 : %d\n", sizeof(sTcPacket), SIZE_SToCPACKET);
	}
}

// 사용자 정의 데이터 수신 함수(클라이언트로부터 입력받은 데이터 수신)
//★ 함수 형태 바꿈. 오류 있을 시 스레드 함수에서 리턴값 받고 break하여 closesocket()
int RecvFromClient(SOCKET client_sock, short PlayerID)
{
	int retVal;

	SOCKET sock = client_sock;
	short playerID = PlayerID;

	SOCKADDR_IN clientAddr;
	int addrLen;

	// 클라이언트 정보 받기
	addrLen = sizeof(clientAddr);
	getpeername(sock, (SOCKADDR *)&clientAddr, &addrLen);

	// 데이터 통신에 사용할 변수
	char buf[SIZE_CToSPACKET + 1];
	ZeroMemory(buf, SIZE_CToSPACKET);

	// 데이터 받기
	retVal = recvn(sock, buf, SIZE_CToSPACKET, 0);
	if (retVal == SOCKET_ERROR) {
		err_display("데이터 recv()");
		return retVal;
	}

	buf[retVal] = '\0';

	// 해당하는 클라이언트 패킷버퍼에 받은 데이터(buf)를 복사
	memcpy(&cTsPacket[playerID], buf, SIZE_CToSPACKET);

	return 0;
}

// 사용자 정의 데이터 송신 함수(클라이언트에게 갱신된 데이터 송신)
void SendToClient()
{
	int retVal;
	char buf[SIZE_SToCPACKET];


	// 통신 버퍼에 패킷 메모리 복사
	ZeroMemory(buf, SIZE_SToCPACKET);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (clientSocks[i] != 0)		// 게임 중간에 나간 플레이어가 아니면
		{
			sTcPacket.gameState = info.players[i].gameState;
			sTcPacket.life = info.players[i].life;

			memcpy(buf, &sTcPacket, SIZE_SToCPACKET);

			retVal = send(clientSocks[i], buf, SIZE_SToCPACKET, 0);
			if (retVal == SOCKET_ERROR)
			{
				closesocket(clientSocks[i]);
				clientSocks[i] = 0;
				printf("%d번 클라 ", i);
				err_display("send()");
				continue;
			}
		}
	}
}


void UpdatePosition(short playerID) {

	short pID = playerID;

	// 아이템 1초마다 스폰시켜주는 부분
	if (item_PrevTime == 0)	// g_PrevTime은 0이고 currTime은 시작부터 시간을 재고 있기때문에 처음 elapsedTime을 구할 때 차이가 너무 많아 나버릴 수 있다.
	{
		item_PrevTime = GetTickCount();
		return;
	}
	DWORD item_currTime = GetTickCount();
	DWORD item_elapsedTime = item_currTime - item_PrevTime;

	// 이 부분은 1초마다 아이템의 Visible을 true로 만들어주는 부분입니다
	// 공식은 현재시간 - 이전시간이 >= 1000ms(1초) 보다 크고, itemIndex가 맥스를 넘지 않을때 인덱스의 값을 true로 만들어주는거에요!
	if (item_elapsedTime >= 1000 && itemIndex < MAX_ITEMS) {
		item_PrevTime = item_currTime;
		tempItems[itemIndex].isVisible = true;
		itemIndex++;
	}

	// 힘 적용
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

void B_W_CollisionAndUpdate()	// 총알 이동, 총알 벽 충돌처리.
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
	// 게임 종료: true 반환
	// 게임 미종료: false 반환

	// 플레이어가 도중에 나갔는지 체크
	if (leftID != -1)	// 남아있는 플레이어 ID가 기본값(-1)이 아니면(= 누군가 게임 도중에 나갔다면)
	{
		tempPlayers[leftID].gameState = WinState;
		return true;
	}

	// LifeCheck 작성
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

	// TimeCheck 작성
	DWORD currTime = GetTickCount();		// current time in millisec
	tempTime = currTime - game_startTime;
	if (tempTime >= GAMEOVER_TIME)
	{
		// 0번 플레이어 승리
		if (tempPlayers[0].life > tempPlayers[1].life) {
			tempPlayers[0].gameState = WinState;
			tempPlayers[1].gameState = LoseState;
		}
		// 1번 플레이어 승리
		else if (tempPlayers[1].life > tempPlayers[0].life) {
			tempPlayers[0].gameState = LoseState;
			tempPlayers[1].gameState = WinState;
		}
		// 무승부
		else if (tempPlayers[0].life == tempPlayers[1].life) {
			tempPlayers[0].gameState = drawState;
			tempPlayers[1].gameState = drawState;
		}

		return true;
	}

	return false;
}


// 데이터를 수신하여 계산 및 Info 갱신하는 스레드 (이전 ProccessClient)
DWORD WINAPI RecvAndUpdateInfo(LPVOID arg)
{
	//★ 생성 시 출력(테스트용)
	printf("RecvAndUpdateInfo 생성 (이전 ProccessClient)\n");

	int retval;

	// 플레이어ID 부여
	short playerID;
	// 총 2명의 플레이어 중 한 사람이 나가고, 새로운 사람이 들어왔을 때 leaveID를 새 플레이어의 playerID로 넣어줌.
	if (info.connectedP == 1 && leaveID != -1)		// leaveID의 초기값은 -1. 
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

		// info 구조체 초기화(info내 아이템 구조체 및 임시아이템 구조체 정보)
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

	printf("[TCP 서버] 클라이언트 %d 접속\n", playerID);

	// 전용소켓
	clientSocks[playerID] = (SOCKET)arg;

	// 클라에게 몇번 플레이어인지 데이터 전송
	send(clientSocks[playerID], (char*)&playerID, sizeof(short), 0);

	while (1) {

		// 데이터 받아오기
		retval = RecvFromClient(clientSocks[playerID], playerID);
		if (retval == SOCKET_ERROR)
			break;

		switch (tempPlayers[playerID].gameState)
		{
		case GamePlayState:
			// 받은 데이터를 이용해 계산
			// 충돌로 인한 아이템 먹은 플레이어ID, 아이템 표시 여부 계산
			P_I_CollisionCheck(playerID);
			P_W_Collision();
			B_W_CollisionAndUpdate();
			P_B_CollisionCheck(playerID);
			P_P_CollisionCheck();

			// 위에서 계산한 결과와 받은 데이터를 토대로 게임이 종료되었는지 체크
			if (!GameEndCheck())	// 게임이 끝나지 않았으면 update
				UpdatePosition(playerID);
			break;

		case WinState:
		case LoseState:
		case drawState:
			// 재시작 선택
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

					// info 구조체 초기화(info내 아이템 구조체 및 임시아이템 구조체 정보)
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


		// 계산한 값 info에 넣기 전, hSendEvt 이벤트 신호 대기
		WaitForSingleObject(hSendEvt, 20);

		// 모든 갱신된 값을 info에 대입
		info.players[playerID].pos.x = tempPlayers[playerID].pos.x;
		info.players[playerID].pos.y = tempPlayers[playerID].pos.y;

		info.players[playerID].gameState = tempPlayers[playerID].gameState;

		info.players[playerID].life = tempPlayers[playerID].life;

		info.gameTime = tempTime;

		for (int i = 0; i < MAX_ITEMS; i++)
			info.items[i] = tempItems[i];

		// 이 곳에 hUpdateInfoEvt 이벤트 신호 해주기
		SetEvent(hUpdateInfoEvt);
	}

	// closesocket()
	if (clientSocks[playerID] != 0)
	{
		closesocket(clientSocks[playerID]);
		clientSocks[playerID] = 0;
	}

	//★ 소멸 시 출력(테스트용)
	printf("[TCP 서버] 클라이언트 %d 종료\n", playerID);

	// 접속자 수 감소
	info.connectedP--;
	printf("연결된 플레이어 수 : %d \n", info.connectedP);

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


// sTcPacket을 갱신하고 데이터를 전송하는 스레드 (이전 CalculateThread)
DWORD WINAPI UpdatePackAndSend(LPVOID arg)
{
	// ★ 생성 시 출력(테스트용)
	printf("UpdatePackAndSend 생성 (이전 CalculateThread)\n");

	while (1)
	{
		// sTcPacket에 갱신된 정보 넣기 전, hUpdateInfoEvt 이벤트 신호 대기
		WaitForMultipleObjects(MAX_PLAYERS, hUpdateInfoEvt, TRUE, 20);

		// 패킷에 갱신된 info 값들을 대입
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

