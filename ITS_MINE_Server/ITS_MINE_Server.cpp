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
CtoSPacket cTsPacket[MAX_PLAYERS];
StoCPacket sTcPacket;

// 각 클라에 대해 갱신된 값을 갖고있을 변수
SPlayer tempPlayers[MAX_PLAYERS];
SItemObj tempItems[MAX_PLAYERS];

// 각 클라이언트 전용 소켓 배열
SOCKET clientSocks[2];

// 시간 저장 변수
DWORD g_startTime = 0;
DWORD g_PrevTime = 0;

// 서버를 떠난 클라이언트ID
short leaveID = -1;
int itemIndex = 0;

CRITICAL_SECTION cs;

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

// 구조체 초기화 함수
void Init() {
	g_startTime = 0;
	g_PrevTime = 0;
	itemIndex = 0;

	// 게임 정보 구조체 초기화
	info.connectedP = 0;
	info.gameTime = 0;

	// 게임 정보 구조체 내의 플레이어 구조체 정보 및 임시플레이어 구조체 정보 초기화
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

	// 게임 정보 구조체 내의 아이템 구조체 정보 및 임시아이템 구조체 정보 초기화
	for (int i = 0; i < MAX_ITEMS; ++i) {
		// 아이템의 좌표를 40 ~ 760 사이로 랜덤하게 놓기
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

	// C -> S Packet 구조체 초기화
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		//cTsPacket[i]= new CtoSPacket;
		cTsPacket[i].life = INIT_LIFE;

		for(int j = 0; j < 4; j++)
			cTsPacket[i].keyDown[j] = false;
	}

	// S -> C Packet 구조체 초기화
	sTcPacket.p1Pos = { INIT_POS, INIT_POS };
	sTcPacket.p2Pos = { INIT_POS, INIT_POS };

	for (int i = 0; i < MAX_ITEMS; ++i) {
		sTcPacket.itemPos[i] = { INIT_POS, INIT_POS };
		sTcPacket.playerID[i] = nullPlayer;
		sTcPacket.isVisible[i] = false;
	}

	// 아이템은 아직 초기화 안함
	sTcPacket.time = 0;
	sTcPacket.gameState = LobbyState;

}

void DeleteAll() {



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
	char buf[SIZE_CToSPACKET+1];
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

	// 받은 데이터 출력 (테스트)
	//std::cout << std::endl
	//	<< "w: " << cTsPacket[playerID]->keyDown[W] << " "
	//	<< "a: " << cTsPacket[playerID]->keyDown[A] << " "
	//	<< "s: " << cTsPacket[playerID]->keyDown[S] << " "
	//	<< "d: " << cTsPacket[playerID]->keyDown[D] << std::endl
	//	<< "[TCP 서버] " << playerID << "번 클라이언트에서 받음"
	//	<< ", 포트 번호 = " << ntohs(clientAddr.sin_port) << " )"
	//	<< std::endl;
	return 0;
}

// 사용자 정의 데이터 송신 함수(클라이언트에게 갱신된 데이터 송신)
void SendToClient()
{
	// 테스트용 출력
	// std::cout << "SendToClient() 호출" << std::endl;
	//Sleep(200);

	int retVal;
	char buf[SIZE_SToCPACKET];

	// 테스트용 출력
	//std::cout << "패킷내 플레이어 1 좌표: " << sTcPacket->p1Pos.x << ", " << sTcPacket->p1Pos.y << std::endl;

	// 통신 버퍼에 패킷 메모리 복사
	ZeroMemory(buf, SIZE_SToCPACKET);
	memcpy(buf, &sTcPacket, SIZE_SToCPACKET);
	
	for (int i = 0; i < info.connectedP; i++)
	{
		retVal = send(clientSocks[i], buf, SIZE_SToCPACKET, 0);
		if (retVal == SOCKET_ERROR)
		{
			std::cout << i << "번 클라 ";
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

	// 내 위치 계산 및 대입
	// ★ elapsed time 계산
	if (g_PrevTime == 0)	// g_PrevTime은 0이고 currTime은 시작부터 시간을 재고 있기때문에 처음 elapsedTime을 구할 때 차이가 너무 많아 나버릴 수 있다.
	{
		g_PrevTime = GetTickCount();
		return;
	}
	DWORD currTime = GetTickCount();		// current time in millisec
	DWORD elapsedTime = currTime - g_PrevTime;
	g_PrevTime = currTime;
	float eTime = (float)elapsedTime / 1000.f;		// ms to s

	// 이 부분은 1초마다 아이템의 Visible을 true로 만들어주는 부분입니다
	// 지금 eTime문제가 해결되면 위치가 변동될 수 있습니다!
	// 시간이 일정하게 계속 갱신될 수 있는 곳에 넣어주시면 됩니다~~
	// 공식은 현재시간 - 이전시간이 >= 1000ms(1초) 보다 크고, itemIndex가 맥스를 넘지 않을때 인덱스의 값을 true로 만들어주는거에요!
	if (elapsedTime >= 1000 && itemIndex < MAX_ITEMS) {
		EnterCriticalSection(&cs);
		info.items[itemIndex].isVisible = true;
		itemIndex++;
		LeaveCriticalSection(&cs);
	}

	//std::cout << "elapsed time: " << eTime << std::endl;		// 시간 확인 출력


	// 힘 적용
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
	tempPlayers[playerID].pos.x = tempPlayers[playerID].pos.x + Vel_x * eTime;
	tempPlayers[playerID].pos.y = tempPlayers[playerID].pos.y + Vel_y * eTime;

	//std::cout << "tempPlayers 좌표 계산 후: "<< tempPlayers[playerID]->pos.x << ", " << tempPlayers[playerID]->pos.y << std::endl;

	// 아이템 위치 계산 및 대입(종원)
	
}

// 종원
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
				printf("플레이어 %d, %d번째 총알 충돌!", playerID, i);
				tempItems[i].playerID = playerID;
				tempItems[i].velocity = 3.f;
				tempItems[i].direction = { x / sqrtf(x *x + y * y), y / sqrtf(x * x + y * y) };
			}
		}
	}
	// playerID를 이용해 계산 후 tempItems[playerID]에 넣을 것!

	// 아이템 먹은 플레이어ID

	// 아이템 표시 여부
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

// 하연
bool GameEndCheck()
{
	// 게임 종료: true 반환
	// 게임 미종료: false 반환

	// LifeCheck 작성
	for (int playerID = 0; playerID < MAX_PLAYERS; ++playerID) {
		if (info.players[playerID].life <= 0) {
			sTcPacket.gameState = GameOverState;
			return true;
		}
		else 
			continue;
	}

	// TimeCheck 작성
	DWORD currTime = GetTickCount();		// current time in millisec

	if (currTime - g_startTime >= GAMEOVER_TIME) {
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
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			info.players[i].gameState = GamePlayState;
			g_startTime = GetTickCount();
		}
	}
		
	std::cout << "[TCP 서버] 클라이언트 " << playerID << "접속" << std::endl;

	// 전용소켓
	SOCKET client_sock = (SOCKET)arg;
	// 전용소켓을 갖는 전역변수 배열에 대입
	clientSocks[playerID] = client_sock;

	while (1) {

		// 데이터 받아오기
		retval = RecvFromClient(client_sock, playerID);
		if (retval == SOCKET_ERROR)
			break;

		// 받은 데이터를 이용해 계산
		// 충돌로 인한 아이템 먹은 플레이어ID, 아이템 표시 여부 계산
		P_I_CollisionCheck(playerID);	// ★ 종원
		
		// 위에서 계산한 결과와 받은 데이터를 토대로 게임이 종료되었는지 체크
		if (!GameEndCheck())	// 게임이 끝나지 않았으면 update해라 (★ 함수 내부: 하연)
			UpdatePosition(playerID);
		
		// ★ 계산한 값 info에 넣기 전, hSendEvt 이벤트 신호 대기
		//WaitForSingleObject(hSendEvt, INFINITE);
		
		// 위에서 계산한 모든 갱신된 값을 info에 대입
		// 캐릭터 위치
		info.players[playerID].pos.x = tempPlayers[playerID].pos.x;
		info.players[playerID].pos.y = tempPlayers[playerID].pos.y;
		
		// 테스트용 출력
		//std::cout << "info내 "<< playerID <<"번 플레이어 좌표: " << info.players[playerID].pos.x << ", " << info.players[playerID].pos.y << std::endl;
		
		// 이 곳에 hUpdateInfoEvt 이벤트 신호 해주기
		SetEvent(hUpdateInfoEvt);
	}


	// closesocket()		★ 현재 RecvFromClient()에서 recv() 오류나면 클로즈하게 되어있음 수정 필요
	closesocket(client_sock);
	
	//★ 소멸 시 출력(테스트용)
	printf("[TCP 서버] 클라이언트 %d 종료\n", playerID);

	// 접속자 수 감소
	info.connectedP--;
	std::cout << "연결된 플레이어 수 : " << info.connectedP << std::endl;

	if (info.connectedP == 0)
		leaveID = -1;
	else
		leaveID = playerID;

	return 0;
}


// sTcPacket을 갱신하고 데이터를 전송하는 스레드 (이전 CalculateThread)
DWORD WINAPI UpdatePackAndSend(LPVOID arg)
{	
	// ★ 생성 시 출력(테스트용)
	printf("UpdatePackAndSend 생성 (이전 CalculateThread)\n");

	while(1)
	{
		// sTcPacket에 갱신된 정보 넣기 전, hUpdateInfoEvt 이벤트 신호 대기
		WaitForSingleObject(hUpdateInfoEvt, INFINITE);

		// 패킷에 갱신된 info 값들을 대입
		// sTcPacket.time = info.gameTime;		// ★ 게임 시간 계산은..?

		sTcPacket.gameState = info.players[0].gameState;		// ★ 0번 클라이언트의 상태를 전송한다.(수정 필요할 듯)
		
		sTcPacket.p1Pos = info.players[0].pos;
		sTcPacket.p2Pos = info.players[1].pos;
		
		for (int i = 0; i < MAX_ITEMS; ++i) {
			sTcPacket.itemPos[i] = info.items[i].pos;
			sTcPacket.playerID[i] = info.items[i].playerID;
			sTcPacket.isVisible[i] = info.items[i].isVisible;
		}

		// SendToClient() 작성
		SendToClient();

		//// 패킷 클라들한테 다 보냈다!
		//SetEvent(hSendEvt);
	};

	return 0;         
}

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
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
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

	// 두 개의 자동 리셋 이벤트 객체 생성(데이터 수신 이벤트, 갱신 이벤트)
	hUpdateInfoEvt = CreateEvent(NULL, FALSE, FALSE, NULL);	// 비신호 상태(바로 시작o)
	if (hUpdateInfoEvt == NULL) return 1;
	hSendEvt = CreateEvent(NULL, FALSE, TRUE, NULL);	// 신호 상태(바로 시작x)
	if (hSendEvt == NULL) return 1;

	// 테스트트트트트트트
	printf("보낼 패킷 사이즈 : %zd\n", SIZE_SToCPACKET);

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
			//hUpdatePackAndSend = CreateThread(NULL, 0, UpdatePackAndSend, (LPVOID)clientSock, 0, NULL);
			if (hRecvAndUpdateInfo[info.connectedP] == NULL) {
				closesocket(clientSock);
			}
			else { 
				CloseHandle(hRecvAndUpdateInfo[info.connectedP]);		//★ 스레드 핸들값을 바로 삭제할지도 논의 필요
			}
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