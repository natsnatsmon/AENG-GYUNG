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

// ���� �ʱ�ȭ�� ���� ����
WSADATA wsa;

// socket()
SOCKET sock;

// connect()
SOCKADDR_IN serveraddr;

ScnMgr *g_ScnMgr = NULL;
DWORD g_PrevTime = 0;

int g_Shoot = SHOOT_NONE;

// ����ü�� ����
CInfo info;
//CItemObj *item[MAX_ITEMS];

// ��Ŷ ����ü ����
CtoSPacket *cTsPacket = new CtoSPacket;
StoCPacket *sTcPacket = new StoCPacket;

void Init() {

	// ������ ����ü �ʱ�ȭ
	for (int i = 0; i < MAX_ITEMS; ++i) {
		info.items[i] = new CItemObj;
		info.items[i]->pos.x = 0;
		info.items[i]->pos.y = 0;
		info.items[i]->playerID = nullPlayer;
		info.items[i]->isVisible = false;
	}

	// C -> S Packet ����ü �ʱ�ȭ
	for (int i = 0; i < 4; ++i)
		cTsPacket->keyDown[i] = false;
	cTsPacket->life = 5;

	// S -> C Packet ����ü �ʱ�ȭ
	sTcPacket->p1Pos.x = INIT_POS;
	sTcPacket->p1Pos.y = INIT_POS;
	sTcPacket->p2Pos.x = INIT_POS;
	sTcPacket->p2Pos.y = INIT_POS;

	for (int i = 0; i < MAX_ITEMS; ++i) {
		sTcPacket->itemPos[i].x = INIT_POS;
		sTcPacket->itemPos[i].y = INIT_POS;
		sTcPacket->playerID[i] = nullPlayer;
		sTcPacket->isVisible[i] = false;

	}

	sTcPacket->time = 0;
	sTcPacket->gameState = MainState;

	// ���� ���� ����ü �ʱ�ȭ
	info.gameState = MainState;
	info.gameTime = 0;
	for (int i = 0; i < MAX_PLAYERS; ++i)
		info.playersPos[i] = { 0, };
	for (int i = 0; i < MAX_ITEMS; ++i) {
		info.items[i]->pos = { 0, 0 };
		info.items[i]->isVisible = 0;
		info.items[i]->playerID = nullPlayer;
	}
	

	// �׽�Ʈ��!!!
	info.gameState = MainState;

	info.playersPos[0].x = 0.f; 
	info.playersPos[0].y = 10.f;
	info.playersPos[1].x = 22.f;
	info.playersPos[1].y = -38.f;


	info.items[0]->pos.x = 30.f;
	info.items[0]->pos.y = 20.f;
	info.items[0]->playerID = nullPlayer;
	info.items[0]->isVisible = true;

	info.items[1]->pos.x = -70.f;
	info.items[1]->pos.y = -72.f;
	info.items[1]->playerID = player1;
	info.items[1]->isVisible = true;

	info.items[2]->pos.x = 143.f;
	info.items[2]->pos.y = 153.f;
	info.items[2]->playerID = player2;
	info.items[2]->isVisible = true;

	info.items[3]->pos.x = -111.f;
	info.items[3]->pos.y = 111.f;
	info.items[3]->playerID = nullPlayer;
	info.items[3]->isVisible = true;
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

void RecvFromServer(SOCKET sock) {

}

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

void SendToServer(SOCKET s) {
	std::cout << "SendToServer() ȣ��" << std::endl;

	int retVal;
	// ������ ��ſ� ����� ����
	char buf[SIZE_CToSPACKET];

	// ������ �� ����(�׽�Ʈ)
//	cTsPacket->keyDown[W] = false;
//	cTsPacket->keyDown[A] = true;
//	cTsPacket->keyDown[S] = true;
//	cTsPacket->keyDown[D] = false;
//	cTsPacket->life = 3;


	// ��� ���ۿ� ��Ŷ �޸� ����
	memcpy(buf, cTsPacket, SIZE_CToSPACKET);

	// ����(�۽Ź��ۿ� ����)
	retVal = send(sock, buf, sizeof(CtoSPacket), 0);
	if (retVal == SOCKET_ERROR)
	{
		err_display("send()");
		exit(1);
	}
}

void RenderScene(void)
{
	if (info.gameState == GamePlayState || info.gameState == LobbyState) {
		SendToServer(sock);
	}


	if (g_PrevTime == 0)
	{
		g_PrevTime = GetTickCount();
		return;
	}
	DWORD CurrTime = GetTickCount();//�и�������
	DWORD ElapsedTime = CurrTime - g_PrevTime;
	g_PrevTime = CurrTime;
	float eTime = (float)ElapsedTime / 1000.0f;



	g_ScnMgr->RenderScene();

	glutSwapBuffers();


	// �̰� �����ϴ°� �� ���� ������ �Ⱥ����� ��� �����ſ���!!!
	Sleep(20);

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
	if (info.gameState == MainState) {
		int retval = 0;
		retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR)
			err_quit("connect()");
		else {
			cout << "connect() �Ϸ�!\n";

			// ���� connect()�� �����ߴٸ�~(�׽�Ʈ)
			SendToServer(sock);

			info.gameState = LobbyState;
			cout << info.gameState;
		}
	}

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


// �� ���� ��°� ������ ���ֵ� ���� �������? ���� �ʿ�
//void SpecialKeyInput(int key, int x, int y)
//{
//	switch (key)
//	{
//	case GLUT_KEY_UP:
//		g_Shoot = SHOOT_UP;
//		break;
//	case GLUT_KEY_DOWN:
//		g_Shoot = SHOOT_DOWN;
//		break;
//	case GLUT_KEY_RIGHT:
//		g_Shoot = SHOOT_RIGHT;
//		break;
//	case GLUT_KEY_LEFT:
//		g_Shoot = SHOOT_LEFT;
//		break;
//	}
//}
//void SpecialKeyUpInput(int key, int x, int y)
//{
//	g_Shoot = SHOOT_NONE;
//}




void DeleteAll()
{
	delete cTsPacket;
	delete sTcPacket;

	for (int i = 0; i < MAX_ITEMS; ++i) {
		delete info.items[i];
	}
}


int main(int argc, char **argv)
{
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
	//glutSpecialFunc(SpecialKeyInput);
	//glutSpecialUpFunc(SpecialKeyUpInput);


	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

	Init();

	// ���� �ʱ�ȭ
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

	g_ScnMgr = new ScnMgr();

	glutMainLoop();		//���� ���� �Լ�
	delete g_ScnMgr;

	// closesocket()
	closesocket(sock);

	// ���� ����
	WSACleanup();

	return 0;
}