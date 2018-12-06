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
#include <stdlib.h>
#include "resource.h"
#include <commctrl.h>

#include "Dependencies\glew.h"
#include "Dependencies\freeglut.h"
#include <math.h>
#include <chrono>
#include "ScnMgr.h"

using namespace std;

// ���� �ʱ�ȭ�� ���� ����
WSADATA wsa;

// socket()
SOCKET sock;

// connect()
SOCKADDR_IN serveraddr;
char* serverIP;

HANDLE hConnectEvt;

ScnMgr *g_ScnMgr = NULL;
DWORD g_PrevTime = 0;

int g_Shoot = SHOOT_NONE;

// ����ü�� ����
CInfo info;
//CItemObj *item[MAX_ITEMS];

// ��Ŷ ����ü ����
CtoSPacket cTsPacket;
StoCPacket sTcPacket;

HWND hIpControl, hSendButton;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI ProcessClient(LPVOID arg);

void err_quit(const char *msg);
void err_display(const char *msg);
int recvn(SOCKET s, char *buf, int len, int flags);
void Init();

void RecvFromServer(SOCKET s);
void SendToServer(SOCKET s);

void RenderScene(void);
void Idle(void);
void MouseInput(int button, int state, int x, int y);
void KeyDownInput(unsigned char key, int x, int y);
void KeyUpInput(unsigned char key, int x, int y);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// ����ü �ʱ�ȭ
	Init();
	hConnectEvt = CreateEvent(NULL, FALSE, FALSE, NULL);

	// ��ȭ���� ����
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DlgProc);


	// Initialize GL things
	glutInit(&__argc, __argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(200, 200);
	glutInitWindowSize(900, 800);
	glutCreateWindow("IT'S MINE !");

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

	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

	// ���� ��� ������ ����
	CreateThread(NULL, 0, ProcessClient, NULL, 0, NULL);

	g_ScnMgr = new ScnMgr();

	glutMainLoop();		//���� ���� �Լ�

	delete g_ScnMgr;

	// closesocket()
	closesocket(sock);

	WSACleanup();
	return 0;
}

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	DWORD address = 0;
	DWORD dwAddress = 0;

	switch (uMsg) {
	case WM_INITDIALOG:
		hIpControl = GetDlgItem(hDlg, IDC_IPADDRESS1);
		hSendButton = GetDlgItem(hDlg, IDOK);
		//SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			//ip �ּ� ��Ʈ�ѿ��� ���� �������� ����� ����!

			// hDlg���̾�α� �ڵ鿡�� IDC_IPADDRESS ip�ּ� ��Ʈ���� �ڵ��� ù��° �Ķ���Ϳ� �ְ�, 
			// ���� ���� ���ڴٴ� ��ũ�� IPM_GETADDRESS�� �ִ´�.
			// ������ �Ķ���ʹ� �� �о�� �ּҰ��� DWORD�� �ִµ� �� �ּҸ� �ѱ��.
			SendMessage(GetDlgItem(hDlg, IDC_IPADDRESS1), IPM_GETADDRESS, 0, (LPARAM)&address);

			// ��Ʈ��ũ ������ ���ĵ� ���� ȣ��Ʈ ������ ��ȯ
			dwAddress = ntohl(address);

			// �Է¹��� 32��Ʈ IP �ּҸ� ���ڿ��� ��ȯ
			serverIP = inet_ntoa(*(IN_ADDR*)&dwAddress);

			// ���� �ʱ�ȭ
			if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
				return 1;

			sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sock == INVALID_SOCKET) err_quit("socket()");

			ZeroMemory(&serveraddr, sizeof(serveraddr));
			serveraddr.sin_family = AF_INET;
			serveraddr.sin_addr.s_addr = inet_addr(serverIP);
			serveraddr.sin_port = htons(SERVERPORT);

			EndDialog(hDlg, IDOK);
			return TRUE;

		case IDCANCEL:

			MessageBox(NULL, "IP�� �Է����� ������ ������ ������ �� �����ϴ�!", "IT'S MINE !", MB_OK);
			exit(1);

			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

DWORD WINAPI ProcessClient(LPVOID arg) {
	WaitForSingleObject(hConnectEvt, INFINITE);
	printf("Ŭ���̾�Ʈ ��� ������ ����\n");
	   
	while (1)
	{
		if (info.gameState != MainState) {
			// ������ �۽�
			SendToServer(sock);

			// ������ ����
			RecvFromServer(sock);
		}
	}

	return 0;
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

// ����ü �ʱ�ȭ �Լ�
void Init() {
	printf("Init()\n");
	serverIP = 0;

	// ���� ���� ����ü �ʱ�ȭ
	info.gameState = MainState;
	info.gameTime = 0;
	info.life = INIT_LIFE;
	for (int i = 0; i < MAX_PLAYERS; ++i)
		info.playersPos[i] = { INIT_POS, INIT_POS };
	for (int i = 0; i < MAX_ITEMS; ++i) {
		info.items[i].pos = { INIT_POS, INIT_POS };
		info.items[i].isVisible = false;
		info.items[i].playerID = nullPlayer;
	}

	// C -> S Packet ����ü �ʱ�ȭ
	cTsPacket = { {false, false, false, false} };

	sTcPacket.gameState = LobbyState;
	sTcPacket.time = 0;

	sTcPacket.p1Pos = { INIT_POS, INIT_POS };
	sTcPacket.p2Pos = { INIT_POS, INIT_POS };

	for (int i = 0; i < MAX_ITEMS; ++i) {
		sTcPacket.itemPos[i] = info.items[i].pos;
		sTcPacket.playerID[i] = nullPlayer;
		sTcPacket.isVisible[i] = false;
	}

	// ��Ŷ ������ Ȯ��
	if (sizeof(cTsPacket) != SIZE_CToSPACKET) {
		printf("define�� c -> s ��Ŷ�� ũ�Ⱑ �ٸ��ϴ�!\n");
		printf("cTsPacket�� ũ�� : %zd\t SIZE_CToSPACKET�� ũ�� : %d\n", sizeof(cTsPacket), SIZE_CToSPACKET);
		return;
	}
	if (sizeof(sTcPacket) != SIZE_SToCPACKET) {
		printf("define�� s -> c ��Ŷ�� ũ�Ⱑ �ٸ��ϴ�!\n");
		printf("sTcPacket�� ũ�� : %zd\t SIZE_SToCPACKET�� ũ�� : %d\n", sizeof(sTcPacket), SIZE_SToCPACKET);
	}
}

void RecvFromServer(SOCKET s) {

	int retVal;

	// ������ ��ſ� ����� ����
	char buf[SIZE_SToCPACKET + 1];
	ZeroMemory(buf, SIZE_SToCPACKET);

	// ������ �ޱ�
	retVal = recvn(s, buf, SIZE_SToCPACKET, 0);
	if (retVal == SOCKET_ERROR) {
		err_display("recv()");
	}


	// ���� ������ ���� ���� ��Ŷ�� ����
	buf[retVal] = '\0';
	memcpy(&sTcPacket, buf, SIZE_SToCPACKET);

	info.gameState = sTcPacket.gameState;
	info.gameTime = sTcPacket.time;
	for (int i = 0; i < MAX_ITEMS; i++)
	{
		info.items[i].pos = sTcPacket.itemPos[i];
		info.items[i].isVisible = sTcPacket.isVisible[i];
		info.items[i].playerID = sTcPacket.playerID[i];
	}
	info.playersPos[0] = sTcPacket.p1Pos;
	info.playersPos[1] = sTcPacket.p2Pos;

	info.life = sTcPacket.life;
}

void SendToServer(SOCKET s) {
	//std::cout << "SendToServer() ȣ��" << std::endl;

	int retVal;
	// ������ ��ſ� ����� ����
	char buf[SIZE_CToSPACKET];

	// ��� ���ۿ� ��Ŷ �޸� ����
	memcpy(buf, &cTsPacket, SIZE_CToSPACKET);

	// ����(�۽Ź��ۿ� ����)
	retVal = send(s, buf, sizeof(CtoSPacket), 0);
	if (retVal == SOCKET_ERROR)
	{
		err_display("send()");
	}
}

void RenderScene(void)
{
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
			std::cout << "connect() �Ϸ�!\n";
			recv(sock, (char*)&info.playerID, sizeof(short), 0);
			printf("%d�� Ŭ����", info.playerID);
			// ���� connect()�� �����ߴٸ�~(�׽�Ʈ)
			SendToServer(sock);

			info.gameState = LobbyState;
			SetEvent(hConnectEvt);
		}
	}

	if (key == 'w' || key == 'W')
	{
		cTsPacket.keyDown[W] = true;
	}
	else if (key == 'a' || key == 'A')
	{
		cTsPacket.keyDown[A] = true;
	}
	else if (key == 's' || key == 'S')
	{
		cTsPacket.keyDown[S] = true;
	}
	else if (key == 'd' || key == 'D')
	{
		cTsPacket.keyDown[D] = true;
	}
	else if (key == VK_RETURN)
	{
		if (info.gameState == WinState || info.gameState == LoseState || info.gameState == drawState)
		{
			cTsPacket.keyDown[W] = true;
			cTsPacket.keyDown[A] = true;
			cTsPacket.keyDown[S] = true;
			cTsPacket.keyDown[D] = true;
		}
	}
	else if (key == VK_ESCAPE)
	{
		if (info.gameState == WinState || info.gameState == LoseState || info.gameState == drawState)
		{
			exit(0);
		}
	}
	
}

void KeyUpInput(unsigned char key, int x, int y)
{
	if (key == 'w' || key == 'W')
	{
		cTsPacket.keyDown[W] = false;
	}
	else if (key == 'a' || key == 'A')
	{
		cTsPacket.keyDown[A] = false;
	}
	else if (key == 's' || key == 'S')
	{
		cTsPacket.keyDown[S] = false;
	}
	else if (key == 'd' || key == 'D')
	{
		cTsPacket.keyDown[D] = false;
	}
	else if (key == VK_RETURN)
	{
		if (cTsPacket.keyDown[W] == true &&
			cTsPacket.keyDown[A] == true &&
			cTsPacket.keyDown[S] == true &&
			cTsPacket.keyDown[D] == true)
		{
			cTsPacket.keyDown[W] = false;
			cTsPacket.keyDown[A] = false;
			cTsPacket.keyDown[S] = false;
			cTsPacket.keyDown[D] = false;
		}
	}
}