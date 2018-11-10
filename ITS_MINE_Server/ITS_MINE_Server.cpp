//
// [ 서버 알고리즘 ]
// RecvAndUpdateInfo에서 데이터를 받아서 충돌체크 및 포지션 갱신 후 대기(hSendEvt 신호 대기)
// -> UpdatePackAndSend에서 갱신된 데이터들을 패킷구조체에 저장한 후 패킷 전송 후 SetEvent하고 대기(hUpdateInfoEvt 신호 대기)
// -> 다시 돌아와 RecvAndUpdateInfo에서 아까 갱신한 데이터들을 info 구조체에 한꺼번에 넣어준 후(즉, 이 때만 info 정보에 접근함) SetEvent
// -> 위 과정 반복

#define _WINSOCK_DEPRECATED_NO_WARNINGS		// inet_addr errror 
#pragma comment(lib, "ws2_32")

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "Global.h"
#include <iostream>

// 동기화를 위한 이벤트 객체 
HANDLE hUpdateInfoEvt;		// 데이터를 수신하여 계산 및 Info 갱신하는 이벤트 객체 (이전 RecvEvt 객체)
HANDLE hSendEvt;			// sTcPacket을 갱신하고 데이터를 전송하는 이벤트 객체 (이전 UpdateEvt 객체)

// 서버 구동에 사용할 변수(게임 관련 정보)
SInfo info;

// 패킷 구조체 선언
CtoSPacket *cTsPacket[MAX_PLAYERS];
StoCPacket *sTcPacket = new StoCPacket;

// 각 클라에 대해 갱신된 값을 갖고있을 변수
SPlayer *tempPlayers[MAX_PLAYERS];
SItemObj *tempItems[MAX_PLAYERS];

// 각 클라이언트 전용 소켓 배열
SOCKET clientSocks[2];

// 이전 시간 저장 변수
DWORD g_PrevTime = 0;

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


void Init() {
	// 게임 정보 구조체 초기화
	info.connectedP = 0;
	info.gameTime = 0;

	// 게임 정보 구조체 내의 플레이어 구조체 정보 및 임시플레이어 구조체 정보 초기화
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

	// 게임 정보 구조체 내의 아이템 구조체 정보 및 임시아이템 구조체 정보 초기화
	for (int i = 0; i < MAX_ITEMS; ++i) {
		info.items[i] = new SItemObj;
		// 아이템의 좌표를 40 ~ 760 사이로 랜덤하게 놓기
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

	// C -> S Packet 구조체 초기화
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		cTsPacket[i]= new CtoSPacket;
		cTsPacket[i]->life = INIT_LIFE;

		for(int j = 0; j < 4; j++)
			cTsPacket[i]->keyDown[j] = false;
	}

	// S -> C Packet 구조체 초기화
	sTcPacket->p1Pos.x = INIT_POS;
	sTcPacket->p1Pos.y = INIT_POS;
	sTcPacket->p2Pos.x = INIT_POS;
	sTcPacket->p2Pos.y = INIT_POS;

	// 아이템은 아직 초기화 안함
	sTcPacket->time = 0;
	sTcPacket->gameState = LobbyState;

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

// 사용자 정의 데이터 수신 함수(클라이언트로부터 입력받은 데이터 수신)
//★ recvn과 합칠 수 있는지 논의 해보기
void RecvFromClient(SOCKET client_sock, short PlayerID)
{
	int retVal;

	SOCKET sock = client_sock;
	short playerID = PlayerID;

	SOCKADDR_IN clientAddr;
	int addrLen;

	// 클라이언트 정보 받기
	addrLen = sizeof(clientAddr);
	getpeername(sock, (SOCKADDR *)&clientAddr, &addrLen);

	// 테스트 수신
	char buf[SIZE_CToSPACKET+1];

	while (1) {
		ZeroMemory(buf, sizeof(CtoSPacket));

		// 데이터 받기
		retVal = recvn(sock, buf, SIZE_CToSPACKET, 0);
		if (retVal == SOCKET_ERROR) {
			err_display("recv()");
			closesocket(sock);
			//★ 소멸 시 출력(테스트용)
			printf("[TCP 서버] 클라이언트 %d 종료\n", playerID);
			break;
		}
		else if (retVal == 0) { break; }

		buf[retVal] = '\0';

		// 해당하는 클라이언트 패킷버퍼에 받은 데이터(buf)를 복사
		memcpy(cTsPacket[playerID], buf, SIZE_CToSPACKET);


		// 받은 데이터 출력 (테스트)
		std::cout << "\nw: "<< cTsPacket[playerID]->keyDown[W] << " "
			<< "a: " << cTsPacket[playerID]->keyDown[A] << " "
			<< "s: " << cTsPacket[playerID]->keyDown[S] << " "
			<< "d: " << cTsPacket[playerID]->keyDown[D] << std::endl
			<< "life: " << cTsPacket[playerID]->life << std::endl
			<< "[TCP 서버] " << playerID << "번 클라이언트에서 받음"
			<< "( IP 주소 = " << inet_ntoa(clientAddr.sin_addr)
			<< ", 포트 번호 = " << ntohs(clientAddr.sin_port) << " )"
			<< std::endl;
	}
}

// 사용자 정의 데이터 송신 함수(클라이언트에게 갱신된 데이터 송신)
void SendToClient(LPVOID arg, short playerID)
{
	// 테스트 송신
	char buf[SIZE_StoCPACKET];

	
}


void UpdatePosition(short playerID) {

	BOOL tempKeyDown[4] = { 
		cTsPacket[playerID]->keyDown[W], 
		cTsPacket[playerID]->keyDown[A], 
		cTsPacket[playerID]->keyDown[S], 
		cTsPacket[playerID]->keyDown[D] 
	};

	// 내 위치 계산 및 대입
	// elapsed time 계산
	if (g_PrevTime == 0)	// g_PrevTime은 0이고 currTime은 시작부터 시간을 재고 있기때문에 처음 elapsedTime을 구할 때 차이가 너무 많아 나버릴 수 있다.
	{
		g_PrevTime = GetTickCount();
		return;
	}
	DWORD currTime = GetTickCount();		// current time in millisec
	DWORD elapsedTime = currTime - g_PrevTime;
	g_PrevTime = currTime;
	float eTime = (float)elapsedTime / 1000.f;		// ms to s

	// 힘 적용
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

	// 0.f으로 set 해주지않으면 계속 빨라진다!
	Accel_x = 0.f;
	Accel_y = 0.f;

	// calc friction(마찰력) 계산
	// 정규화 과정 (벡터 방향을 그대로 두지만 크기가 1인 단위벡터로 바꾸는 과정)
	float magVel = sqrtf(Vel_x * Vel_x + Vel_y * Vel_y);
	float velX = Vel_x / magVel;
	float velY = Vel_y / magVel;

	// 마찰력이 작용하는 방향도 구함(힘의 반대방향이므로)
	float fricX = -velX;
	float fricY = -velY;

	// 마찰력의 크기
	float friction = COEF_FRICT * MASS * GRAVITY; // 마찰력 = 마찰계수 * m(질량) * g(중력)

	// 마찰력 방향 * 마찰력 크기
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

	// calc pos(최종 계산된 위치값을 갖고있음)
	tempPlayers[playerID]->pos.x = tempPlayers[playerID]->pos.x + Vel_x * eTime;
	tempPlayers[playerID]->pos.y = tempPlayers[playerID]->pos.y + Vel_y * eTime;

	
	// 아이템 위치 계산 및 대입(종원)
	


}

// 종원
void CollisionCheck(short playerID)
{
	// playerID를 이용해 계산 후 tempItems[playerID]에 넣을 것!

	// 아이템 먹은 플레이어ID

	// 아이템 표시 여부
}

// 하연
bool GameEndCheck()
{
	// 게임 종료: true 반환
	// 게임 미종료: false 반환

	// LifeCheck 작성
	for (int playerID = 0; playerID < MAX_PLAYERS; ++playerID) {
		if (info.players[playerID]->life <= 0) {
			sTcPacket->gameState = GameOverState;
			return true;
		}
		else 
			continue;
	}

	// TimeCheck 작성
	

	return false;
}


// 데이터를 수신하여 계산 및 Info 갱신하는 스레드 (이전 ProccessClient)
DWORD WINAPI RecvAndUpdateInfo(LPVOID arg)
{
	//★ 생성 시 출력(테스트용)
	printf("RecvAndUpdateInfo 생성 (이전 ProccessClient)\n");

	// 접속자 수를 활용해 플레이어ID 부여
	short playerID = info.connectedP++;

	// 전용소켓
	SOCKET client_sock = (SOCKET)arg;
	
	// 전용소켓을 갖는 전역변수 배열에 대입
	clientSocks[playerID] = client_sock;

	// 데이터 받아오기
	RecvFromClient(client_sock, playerID);

	// 받은 데이터를 이용해 계산
	// 충돌로 인한 아이템 먹은 플레이어ID, 아이템 표시 여부 계산
	CollisionCheck(playerID);	// ★ 종원

	// 위에서 계산한 결과와 받은 데이터를 토대로 게임이 종료되었는지 체크
	if (!GameEndCheck())	// 게임이 끝나지 않았으면 update해라 (★ 함수 내부: 하연)
		UpdatePosition(playerID);
	
	// 계산한 값 info에 넣기 전, hSendEvt 이벤트 신호 대기
	WaitForSingleObject(hSendEvt, INFINITE);

	// 위에서 계산한 모든 갱신된 값을 info에 대입
	// 캐릭터 위치
	info.players[playerID]->pos.x = tempPlayers[playerID]->pos.x;
	info.players[playerID]->pos.y = tempPlayers[playerID]->pos.y;

	// 스테이트에 따른 switch문(이건 어디로 가야하나..?)
	/*
	switch (info.players[playerID]->gameState) {
	case LobbyState:
		// 로비에서는 gamestate가 변경되었을때만 gamestate 정보를 보내면 된당
		break;

	case GamePlayState:
		// 이때는 패킷내용 다 필요하고
		break;

	case GameOverState:
		// 이때에는 뭐 받아야댐??
		break;

	default:
		printf("state 오류!\n");
		return 1;
	}
	*/


	// 이 곳에 hUpdateInfoEvt 이벤트 신호 해주기
	SetEvent(hUpdateInfoEvt);


	// closesocket()
	closesocket(client_sock);
	//★ 소멸 시 출력(테스트용)
	printf("[TCP 서버] 클라이언트 %d 종료\n", playerID);
	// 접속자 수 감소
	info.connectedP--;
	return 0;
}


// sTcPacket을 갱신하고 데이터를 전송하는 스레드 (이전 CalculateThread)
DWORD WINAPI UpdatePackAndSend(LPVOID arg)
{
	// ★ 생성 시 출력(테스트용)
	printf("UpdatePackAndSend 생성 (이전 CalculateThread)\n");

	// sTcPacket에 갱신된 정보 넣기 전, hUpdateInfoEvt 이벤트 신호 대기
	WaitForSingleObject(hUpdateInfoEvt, INFINITE);

	// 패킷에 갱신된 info 값들을 대입

	
	// SendToClient() 작성


	// 패킷 클라들한테 다 보냈다!
	SetEvent(hSendEvt);
	
	return 0;         
}

int main(int argc, char *argv[])
{
	int retval;		// 모든 return value를 받을 변수

	// 윈속 초기화
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

	// 데이터 통신에 사용할 변수
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrlen;

	HANDLE hRecvAndUpdateInfo[2];	// 클라이언트 처리 스레드 핸들
	HANDLE hUpdatePackAndSend;	// 연산 스레드 핸들

	hUpdatePackAndSend = CreateThread(NULL, 0, UpdatePackAndSend, NULL, 0, NULL); // UpdatePackAndSend 생성 (이전 hCalculateThread)

	// 두 개의 자동 리셋 이벤트 객체 생성(데이터 수신 이벤트, 갱신 이벤트)
	hUpdateInfoEvt = CreateEvent(NULL, FALSE, FALSE, NULL);	// 비신호 상태(바로 시작o)
	if (hUpdateInfoEvt == NULL) return 1;
	hSendEvt = CreateEvent(NULL, FALSE, TRUE, NULL);	// 신호 상태(바로 시작x)
	if (hSendEvt == NULL) return 1;
	
	// 구조체 초기화
	Init();

	// 테스트트트트트트트
	printf("보낼 패킷 사이즈 : %zd\n", sizeof(StoCPacket));

	while(1)
	{
		while (info.connectedP < MAX_PLAYERS)		// ★ 이 조건은 다시 회의 후 결정
		{
			// accept()
			addrlen = sizeof(clientAddr);
			clientSock = accept(listen_sock, (SOCKADDR *)&clientAddr, &addrlen);
			if (clientSock == INVALID_SOCKET) {
				err_display("accept()");
				break;
			}
	
			// hRecvAndUpdateInfo 생성 (이전 hProccessClient)
			hRecvAndUpdateInfo[info.connectedP] = CreateThread(NULL, 0, RecvAndUpdateInfo, (LPVOID)clientSock, 0, NULL);
			if (hRecvAndUpdateInfo[info.connectedP] == NULL) {
				closesocket(clientSock);
			}
			else { 
				CloseHandle(hRecvAndUpdateInfo[info.connectedP]);		//★ 스레드 핸들값을 바로 삭제할지도 논의 필요
			}
	
			//★ 접속한 클라이언트 정보 출력(테스트용)
			printf("\n[TCP 서버] 클라이언트 %d 접속: IP 주소=%s, 포트 번호=%d\n",
				info.connectedP, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
		}
	}

	// delete()
	DeleteAll();

	// closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}