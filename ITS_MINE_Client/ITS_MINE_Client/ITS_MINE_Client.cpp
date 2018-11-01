/*
Copyright 2017 Lee Taek Hee (Korea Polytech University)

This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/
//#pragma comment(lib,"winmm.lib")
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



ScnMgr *g_ScnMgr = NULL;
DWORD g_PrevTime = 0;
BOOL g_KeyW = FALSE;
BOOL g_KeyA = FALSE;
BOOL g_KeyD = FALSE;
BOOL g_KeyS = FALSE;

int g_Shoot = SHOOT_NONE;


void RenderScene(void)
{
	if (g_PrevTime == 0)
	{
		g_PrevTime = GetTickCount();
		return;
	}
	DWORD CurrTime = GetTickCount();//밀리세컨드
	DWORD ElapsedTime = CurrTime - g_PrevTime;
	g_PrevTime = CurrTime;
	float eTime = (float)ElapsedTime / 1000.0f;

	float forceX = 0.f;
	float forceY = 0.f;
	if (g_KeyW)
	{
		forceY += FORCE;
	}
	if (g_KeyS)
	{
		forceY -= FORCE;
	}
	if (g_KeyA)
	{
		forceX -= FORCE;
	}
	if (g_KeyD)
	{
		forceX += FORCE;
	}
	g_ScnMgr->ApplyForce(forceX, forceY, eTime);

	g_ScnMgr->Update(eTime);

	g_ScnMgr->RenderScene();

	g_ScnMgr->Shoot(g_Shoot);

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
	if (key == 'w')
	{
		g_KeyW = TRUE;
	}
	else if (key == 's')
	{
		g_KeyS = TRUE;
	}
	else if (key == 'a')
	{
		g_KeyA = TRUE;
	}
	else if (key == 'd')
	{
		g_KeyD = TRUE;
	}
}
void KeyUpInput(unsigned char key, int x, int y)
{
	if (key == 'w')
	{
		g_KeyW = FALSE;
	}
	else if (key == 's')
	{
		g_KeyS = FALSE;
	}
	else if (key == 'a')
	{
		g_KeyA = FALSE;
	}
	else if (key == 'd')
	{
		g_KeyD = FALSE;
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
	glutSpecialFunc(SpecialKeyInput);
	glutSpecialUpFunc(SpecialKeyUpInput);

	int a = GLUT_KEY_UP;

	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

	g_ScnMgr = new ScnMgr();

	glutMainLoop();//메인 루프 함수

	delete g_ScnMgr;


    return 0;
}

