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
Info info;

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

	printf("[info 초기화 후]\n");
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		printf("[player %d]\n", i);
		printf("state: %d\n", info.p[i]->gameState);
		printf("life: %d\n", info.p[i]->life);
		printf("pos: %f, %f\n", info.p[i]->pos.x, info.p[i]->pos.y);
		for (int j = 0; j < 4; ++j)
			printf("%d ", info.p[i]->keyDown[j]);
		printf("\n\n");
	}

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
	char buf[5];

	while (1) {
		ZeroMemory(buf, sizeof(CtoSPacket));

		// 데이터 받기
		retVal = recvn(sock, buf, SIZE_CToSPACKET, 0);
		if (retVal == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		
		else if (retVal == 0) { break; }

		// 받은 데이터 출력
		buf[retVal] = '\0';
		printf("w: %d, a: %d, s: %d, d: %d \n", buf[0], buf[1], buf[2], buf[3]);

		std::cout << "[TCP/" << inet_ntoa(clientAddr.sin_addr) << ":"
			<< ntohs(clientAddr.sin_port) << "] " << buf << std::endl;
	}
}

// 사용자 정의 데이터 송신 함수(클라이언트에게 갱신된 데이터 송신)
void SendToClient(LPVOID arg, short PlayerID)
{
}


// 클라이언트로부터 데이터를 주고 받는 스레드 함수
DWORD WINAPI ProccessClient(LPVOID arg)
{
	//★ 생성 시 출력(테스트용)
	printf("ProccessClient 생성\n");
	
	//printf("\n[TCP 서버] 클라이언트 %d 접속: IP 주소=%s, 포트 번호=%d\n",
	//	info.connectedP, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

	short playerID = info.connectedP++;

	// 클라한테 데이터 받기
	SOCKET client_sock = (SOCKET)arg;

	const short currentThreadNum = playerID++;

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


	// 계산해줘 ~~~~~ 이벤트 켜기
	// CalculateThread에서 계산하도록 넘겨주기

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

// 모든 연산 및 갱신 스레드 함수
DWORD WINAPI CalculateThread(LPVOID arg)
{
	//★ 생성 시 출력(테스트용)
	printf("CalculateThread 생성\n");

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
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;

	HANDLE hProccessClient[2];	// 클라이언트 처리 스레드 핸들
	HANDLE hCalculateThread;	// 연산 스레드 핸들

	hCalculateThread = CreateThread(NULL, 0, CalculateThread, NULL, 0, NULL); // hCalculateThread 생성

	// 두 개의 자동 리셋 이벤트 객체 생성(데이터 수신 이벤트, 갱신 이벤트)
	hRecvEvt = CreateEvent(NULL, FALSE, FALSE, NULL);	// 비신호 상태
	if (hRecvEvt == NULL) return 1;
	hUpdateEvt = CreateEvent(NULL, FALSE, TRUE, NULL);	// 신호 상태
	if (hUpdateEvt == NULL) return 1;
	
	// 구조체 초기화
	Init();

	// 테스트트트트트트트
	printf("cTsPacket.life : %d\n", cTsPacket->life);
	printf("보낼 패킷 사이즈 : %zd\n", sizeof(StoCPacket));

	while(1)
	{
		while (info.connectedP < MAX_PLAYERS)		// ★ 이 조건은 다시 회의 후 결정
		{
			// accept()
			addrlen = sizeof(clientaddr);
			client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
			if (client_sock == INVALID_SOCKET) {
				err_display("accept()");
				break;
			}
	
			// hProccessClient 생성
			hProccessClient[info.connectedP] = CreateThread(NULL, 0, ProccessClient, (LPVOID)client_sock, 0, NULL);
			if (hProccessClient[info.connectedP] == NULL) { 
				closesocket(client_sock); 
			}
			else { 
				CloseHandle(hProccessClient[info.connectedP]);		//★ 스레드 핸들값을 바로 삭제할지도 논의 필요
			}
	
			//★ 접속한 클라이언트 정보 출력(테스트용)
			printf("\n[TCP 서버] 클라이언트 %d 접속: IP 주소=%s, 포트 번호=%d\n",
				info.connectedP, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
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