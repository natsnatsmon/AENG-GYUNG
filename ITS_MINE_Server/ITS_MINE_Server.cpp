#define _WINSOCK_DEPRECATED_NO_WARNINGS		// inet_addr errror 
#pragma comment(lib, "ws2_32")

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "Global.h"
#include <iostream>

// 동기화를 위한 이벤트 객체 
HANDLE hRecvEvt;		// 클라이언트로부터 데이터 수신 이벤트 객체
HANDLE hUpdateEvt;		// 연산 및 갱신 이벤트 객체

// 서버 구동에 사용할 변수(게임 관련 정보)
SInfo info;

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

// 플레이어, 아이템 구조체
//SPlayer *players[MAX_PLAYERS];
//SItemObj *item[MAX_ITEMS];
CtoSPacket *cTsPacket = new CtoSPacket;
StoCPacket *sTcPacket = new StoCPacket;

void Init() {
	
	// 게임 정보 구조체 초기화
	info.connectedP = 0;
	info.gameTime = 0;

	// 게임 정보 구조체 내의 플레이어 구조체 정보 초기화
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		info.p[i] = new SPlayer;
		info.p[i]->gameState = MainState;
		info.p[i]->pos.x = INIT_POS;
		info.p[i]->pos.y = INIT_POS;
		info.p[i]->life = INIT_LIFE;
		for (int j = 0; j < 4; ++j)
			info.p[i]->keyDown[j] = false;
	}

	/*printf("[info 초기화 후]\n");
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		printf("[player %d]\n", i);
		printf("state: %d\n", info.p[i]->gameState);
		printf("life: %d\n", info.p[i]->life);
		printf("pos: %f, %f\n", info.p[i]->pos.x, info.p[i]->pos.y);
		for (int j = 0; j < 4; ++j)
			printf("%d ", info.p[i]->keyDown[j]);
		printf("\n\n");
	}*/

	// 게임 정보 구조체 내의 아이템 구조체 정보 초기화
	for (int i = 0; i < MAX_ITEMS; ++i) {
		info.i[i] = new SItemObj;
		// 아이템의 좌표를 40 ~ 760 사이로 랜덤하게 놓기
		info.i[i]->pos.x = (float)(rand() % 720 + (R_ITEM * 2));
		info.i[i]->pos.y = (float)(rand() % 720 + (R_ITEM * 2));
		info.i[i]->direction.x = 0;
		info.i[i]->direction.y = 0;
		info.i[i]->velocity = 0.f;
		info.i[i]->playerID = nullPlayer;
		info.i[i]->isVisible = false;
	}


	// C -> S Packet 구조체 초기화
	for (int i = 0; i < 4; ++i)
		cTsPacket->keyDown[i] = false;

	// S -> C Packet 구조체 초기화
	sTcPacket->p1Pos.x = INIT_POS;
	sTcPacket->p1Pos.y = INIT_POS;
	sTcPacket->p2Pos.x = INIT_POS;
	sTcPacket->p2Pos.y = INIT_POS;

	// 아이템은 아직 초기화 안함
	sTcPacket->time = 0;
	sTcPacket->life = INIT_LIFE;
	sTcPacket->gameState = MainState;

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
	delete cTsPacket;
	delete sTcPacket;

	for (int i = 0; i < MAX_PLAYERS; ++i) {
		delete info.p[i];
	}

	for (int i = 0; i < MAX_ITEMS; ++i) {
		delete info.i[i];
	}
}

// 사용자 정의 데이터 수신 함수(클라이언트로부터 입력받은 데이터 수신)
//★ recvn과 합칠 수 있는지 논의 해보기
void RecvFromClient(SOCKET client_sock, short PlayerID)
{
	int retVal;

	SOCKADDR_IN clientAddr;
	int addrLen;

	SOCKET sock = client_sock;

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
			break;
		}
		else if (retVal == 0) { break; }

		buf[retVal] = '\0';

		// 받은 buf 형 변환 후 대입
		cTsPacket = (CtoSPacket*)buf;

		// 받은 데이터 출력 (테스트)
		std::cout << "\nw: "<< cTsPacket->keyDown[0] << " " 
			<< "a: " << cTsPacket->keyDown[1] << " "
			<< "s: " << cTsPacket->keyDown[2] << " "
			<< "d: " << cTsPacket->keyDown[3] << std::endl;
		std::cout << "[TCP 서버] " << info.connectedP << "번 클라이언트에서 받음"
			<< "( IP 주소 = " << inet_ntoa(clientAddr.sin_addr)
			<< ", 포트 번호 = " << ntohs(clientAddr.sin_port) << " )"
			<< std::endl;
	}
}

// 사용자 정의 데이터 송신 함수(클라이언트에게 갱신된 데이터 송신)
void SendToClient(LPVOID arg, short PlayerID)
{
	// 테스트 송신
	char buf[SIZE_StoCPACKET];


}


// 클라이언트로부터 데이터를 주고 받는 스레드 함수
DWORD WINAPI ProccessClient(LPVOID arg)
{
	//★ 생성 시 출력(테스트용)
	printf("ProccessClient 생성\n");

	// 접속자 수를 활용해 플레이어ID 부여
	short playerID = info.connectedP++;

	// 클라한테 데이터 받기
	SOCKET client_sock = (SOCKET)arg;

	RecvFromClient(client_sock, playerID);

	// 이 곳에 recv 이벤트 신호 해주기
	// 바로 다음 줄에 waitfor() 작성

	switch (info.p[playerID]->gameState) {
	case LobbyState:
		break;

	case GamePlayState:
		break;

	case GameOverState:
		break;

	default:
		printf("state 오류!\n");
		return 1;
	}



	// 받는 이벤트 기다린다.....

	// 클라한테 데이터 보내기



	// closesocket()	★ closesocket()하려면 우리가 정의한 데이터 송.수신 함수 인자로 arg말고 socket 넘겨주는게 나을 듯..
	closesocket(client_sock);
	//★ 소멸 시 출력(테스트용)
	printf("[TCP 서버] 클라이언트 %d 종료\n", info.connectedP);
	// 접속자 수 감소
	info.connectedP--;
	return 0;
}

void UpdatePosition() {

	BOOL tempKeyDown[4] = { cTsPacket->keyDown[0], cTsPacket->keyDown[1], cTsPacket->keyDown[2], cTsPacket->keyDown[3] };
	
	// 계산


	// 내 위치, 상대방 위치(다른 스레드에서 위치값을 알아와야 함), 아이템 위치 계산




}

// 종원
void CollisionCheck()
{

}

// 하연
bool GameEndCheck()
{
	// LifeCheck 작성

	// 목숨이 0이라면 gameState를 변경하기 
	return false;
}

// 모든 연산 및 갱신 스레드 함수
DWORD WINAPI CalculateThread(LPVOID arg)
{
	// ★ 생성 시 출력(테스트용)
	printf("CalculateThread 생성\n");


	// "데이터 다 받음" 이벤트 신호 체크(waitfor())
	//   이 곳에 작성해야함.

	// 충돌로 인한 내 생명, 아이템 먹은 플레이어ID, 아이템 표시 여부 계산하여 sTcPacket에 각 각 대입
	CollisionCheck();	// ★ 종원

	// 위에서 계산한 결과를 토대로 게임이 종료되었는지 체크하여 
	if(!GameEndCheck())	// 게임이 끝나지 않았으면 update해라 (★ 함수 내부: 하연)
		UpdatePosition();

	
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

	HANDLE hProccessClient[2];	// 클라이언트 처리 스레드 핸들
	HANDLE hCalculateThread;	// 연산 스레드 핸들

	hCalculateThread = CreateThread(NULL, 0, CalculateThread, NULL, 0, NULL); // hCalculateThread 생성

	// 두 개의 자동 리셋 이벤트 객체 생성(데이터 수신 이벤트, 갱신 이벤트)
	hRecvEvt = CreateEvent(NULL, FALSE, FALSE, NULL);	// 비신호 상태(바로 시작o)
	if (hRecvEvt == NULL) return 1;
	hUpdateEvt = CreateEvent(NULL, FALSE, TRUE, NULL);	// 신호 상태(바로 시작x)
	if (hUpdateEvt == NULL) return 1;
	
	// 구조체 초기화
	Init();

	//UpdateValues();

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
	
			// hProccessClient 생성
			hProccessClient[info.connectedP] = CreateThread(NULL, 0, ProccessClient, (LPVOID)clientSock, 0, NULL);
			if (hProccessClient[info.connectedP] == NULL) { 
				closesocket(clientSock);
			}
			else { 
				CloseHandle(hProccessClient[info.connectedP]);		//★ 스레드 핸들값을 바로 삭제할지도 논의 필요
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