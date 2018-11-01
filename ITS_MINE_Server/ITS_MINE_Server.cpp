#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "Global.h"

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

// 사용자 정의 데이터 수신 함수(클라이언트로부터 입력받은 데이터 수신)
//★ recvn과 합칠 수 있는지 논의 해보기
void RecvFromClient(LPVOID arg, short PlayerID)
{

}

// 사용자 정의 데이터 송신 함수(클라이언트에게 갱신된 데이터 송신)
void SendToClient(LPVOID arg, short PlayerID)
{

}

// 모든 연산 및 갱신 스레드 함수
DWORD WINAPI CalculateThread(LPVOID ard)
{
	//★ 생성 시 출력(테스트용)
	printf("CalculateThread 생성\n");

	return 0;
}

// 클라이언트로부터 데이터를 주고 받는 스레드 함수
DWORD WINAPI ProccessClient(LPVOID ard)
{
	//★ 생성 시 출력(테스트용)
	printf("ProccessClient 생성\n");

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

	// 두 개의 자동 리셋 이벤트 객체 생성(데이터 수신 이벤트, 갱신 이벤트)
	hRecvEvt = CreateEvent(NULL, FALSE, FALSE, NULL);	// 비신호 상태
	if (hRecvEvt == NULL) return 1;
	hUpdateEvt = CreateEvent(NULL, FALSE, TRUE, NULL);	// 신호 상태
	if (hUpdateEvt == NULL) return 1;

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;

	HANDLE hCalculateThread;	// 연산 스레드 핸들
	HANDLE hProccessClient[2];	// 클라이언트 처리 스레드 핸들

	// hCalculateThread 생성
	hCalculateThread = CreateThread(NULL, 0, CalculateThread, NULL, 0, NULL);

	while (info.connectedP < MAX_PLAYERS)		// ★ 이 조건은 다시 회의 후 결정
	{
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		//★ 접속한 클라이언트 정보 출력(테스트용)
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		// hProccessClient 생성
		hProccessClient[info.connectedP] = CreateThread(NULL, 0, ProccessClient, (LPVOID)client_sock, 0, NULL);
		if (hProccessClient[info.connectedP] == NULL) { closesocket(client_sock); }
		else { CloseHandle(hProccessClient[info.connectedP]); }		//★ 스레드 핸들값을 바로 삭제할지도 논의 필요

		// 접속자 수 증가
		info.connectedP++;
	}

	// closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}