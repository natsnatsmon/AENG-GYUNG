/*
Copyright 2017 Lee Taek Hee (Korea Polytech University)

This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/
//#pragma comment(lib,"winmm.lib")
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include "stdafx.h"
#include <iostream>
#include <Windows.h>
#include "Dependencies\glew.h"
#include "Dependencies\freeglut.h"
#include <math.h>
#include <chrono>
#include "ScnMgr.h"


#define FORCE 1000.f
using namespace std;

// 윈속 초기화에 쓰일 변수
WSADATA wsa;

// socket()
SOCKET sock;

// connect()
SOCKADDR_IN serveraddr;

ScnMgr *g_ScnMgr = NULL;
DWORD g_PrevTime = 0;

int g_Shoot = SHOOT_NONE;

// 구조체들 선언
CInfo info;
CItemObj *item[MAX_ITEMS];

CtoSPacket *cTsPacket = new CtoSPacket;
StoCPacket *sTcPacket = new StoCPacket;

void Init() {

	// 아이템 구조체 초기화
	for (int i = 0; i < MAX_ITEMS; ++i) {
		item[i] = new CItemObj;
		item[i]->pos.x = 0;
		item[i]->pos.y = 0;
		item[i]->playerID = nullPlayer;
		item[i]->isVisible = false;
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



	// 게임 정보 구조체 초기화
	info.gameState = MainState;
	info.gameTime = 0;
	for (int i = 0; i < MAX_PLAYERS; ++i)
		info.playerPos[i] = { 0, };
	for (int i = 0; i < MAX_ITEMS; ++i)
		info.item[i] = { {0, 0}, 0, nullPlayer };

}


int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;
	while (left > 0)
	{
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

void RenderScene(void)
{
	int retval;
	if (g_PrevTime == 0)
	{
		g_PrevTime = GetTickCount();
		return;
	}
	DWORD CurrTime = GetTickCount();//밀리세컨드
	DWORD ElapsedTime = CurrTime - g_PrevTime;
	g_PrevTime = CurrTime;
	float eTime = (float)ElapsedTime / 1000.0f;

	/*retval = send(sock, (char*)cTsPacket, sizeof(cTsPacket), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		exit(1);
	}*/

	g_ScnMgr->RenderScene();

	glutSwapBuffers();
}

void Idle(void)
{
	RenderScene();
}

void MouseInput(int button, int state, int x, int y)
{
	RenderScene();
}


void KeyDownInput(unsigned char key, int x, int y)
{
	if (key == 'w' || key == 'W')
	{
		cTsPacket->keyDown[W] = true;
	}
	else if (key == 'a' || key == 'A')
	{
		cTsPacket->keyDown[A] = true;
	}
	else if (key == 's' || key == 'S')
	{
		cTsPacket->keyDown[S] = true;
	}
	else if (key == 'd' || key == 'D')
	{
		cTsPacket->keyDown[D] = true;
	}
}
void KeyUpInput(unsigned char key, int x, int y)
{
	if (key == 'w' || key == 'W')
	{
		cTsPacket->keyDown[W] = false;
	}
	else if (key == 'a' || key == 'A')
	{
		cTsPacket->keyDown[A] = false;
	}
	else if (key == 's' || key == 'S')
	{
		cTsPacket->keyDown[S] = false;
	}
	else if (key == 'd' || key == 'D')
	{
		cTsPacket->keyDown[D] = false;
	}
}
void SpecialKeyInput(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		g_Shoot = SHOOT_UP;
		break;
	case GLUT_KEY_DOWN:
		g_Shoot = SHOOT_DOWN;
		break;
	case GLUT_KEY_RIGHT:
		g_Shoot = SHOOT_RIGHT;
		break;
	case GLUT_KEY_LEFT:
		g_Shoot = SHOOT_LEFT;
		break;
	}
}

void SpecialKeyUpInput(int key, int x, int y)
{
	g_Shoot = SHOOT_NONE;
}

void SendToServer(SOCKET s) {
	cout << "SendToServer() 호출\n";

	int retVal;
	// ★ 테스트용 데이터 통신에 사용할 변수
	char buf[SIZE_CToSPACKET];


	// 보내려는 버퍼에 값 대입
	buf[W] = cTsPacket->keyDown[W];
	buf[A] = cTsPacket->keyDown[A];
	buf[S] = cTsPacket->keyDown[S];
	buf[D] = cTsPacket->keyDown[D];

	// 전송(송신버퍼에 복사)
	retVal = send(sock, buf, sizeof(CtoSPacket), 0);
	if (retVal == SOCKET_ERROR)
	{
		err_display("send()");
		exit(1);
	}
	else
		cout << "다보냄!!!" << endl;
}

int main(int argc, char **argv)
{
	int retval;

	// Initialize GL things
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(900, 800);
	glutCreateWindow("Game Software Engineering KPU");

	glewInit();
	if (glewIsSupported("GL_VERSION_3_0"))
	{
		std::cout << " GLEW Version is 3.0\n ";
	}
	else
	{
		std::cout << "GLEW 3.0 not supported\n ";
	}

	glutDisplayFunc(RenderScene);
	glutIdleFunc(Idle);
	glutKeyboardFunc(KeyDownInput);
	glutKeyboardUpFunc(KeyUpInput);
	glutMouseFunc(MouseInput);
	glutSpecialFunc(SpecialKeyInput);
	glutSpecialUpFunc(SpecialKeyUpInput);


	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

	Init();

	// 윈속 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) 
		err_quit("connect()");
	else
		cout << "connect() 완료!\n";

	g_ScnMgr = new ScnMgr();

	SendToServer(sock);

	glutMainLoop();		//메인 루프 함수
	delete g_ScnMgr;

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();

    return 0;
}

