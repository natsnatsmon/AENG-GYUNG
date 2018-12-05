#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include "stdafx.h"
#include "ScnMgr.h"
#include <math.h>
#include <string.h>
#include <iostream>

int g_Seq = 0;

ScnMgr::ScnMgr()
{
	m_Renderer = NULL;

	m_Renderer = new Renderer(900, 800);
	if (!m_Renderer->IsInitialized())
	{
		std::cout << "Renderer could not be initialized.. \n";
	}


	// 텍스쳐 파일 로드
	m_PlayerTex[0] = m_Renderer->CreatePngTexture("./textures/p1_sprite.png");
	m_PlayerTex[1] = m_Renderer->CreatePngTexture("./textures/p2_sprite.png");
	
	m_BulletTex[0] = m_Renderer->CreatePngTexture("./textures/p1_bullet.png");
	m_BulletTex[1] = m_Renderer->CreatePngTexture("./textures/p2_bullet.png");

	m_ArrowTex = m_Renderer->CreatePngTexture("./textures/arrow.png");
	m_ItemTex = m_Renderer->CreatePngTexture("./textures/item.png");
	m_LifeTex = m_Renderer->CreatePngTexture("./textures/life.png");

	for (int i = 0; i < 10; ++i) {
		std::string filePath = "./textures/";
		std::string fileName = filePath + std::to_string(i) + ".png";
		char texFile[50];
		strcpy_s(texFile, fileName.c_str());

		m_TimeTex[i] = m_Renderer->CreatePngTexture(texFile);
	}

	m_StartUITex = m_Renderer->CreatePngTexture("./textures/startUI.png");
	m_LobbyUITex = m_Renderer->CreatePngTexture("./textures/waitUI.png");
	m_PlayUITex = m_Renderer->CreatePngTexture("./textures/playUI.png");
	m_ResultWinUITex = m_Renderer->CreatePngTexture("./textures/winUI.png");
	m_ResultLoseUITex = m_Renderer->CreatePngTexture("./textures/loseUI.png");
}

ScnMgr::~ScnMgr()
{
	if (m_Renderer)
	{
		delete m_Renderer;
		m_Renderer = NULL;
	}
}

extern CInfo info;
extern StoCPacket *sTcPacket;
extern CtoSPacket *cTsPacket;
extern CItemObj *item[MAX_ITEMS];

void ScnMgr::RenderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.3f, 0.3f, 1.0f);

	// UI 그리기
	switch (info.gameState) {
	case MainState:
		m_Renderer->DrawTextureRectSeqXY(0, 0, 0, 900, 800, 1, 1, 1, 1, m_StartUITex, 1, 1, 1, 1);
		break;

	case LobbyState:
		m_Renderer->DrawTextureRectSeqXY(0, 0, 0, 900, 800, 1, 1, 1, 1, m_LobbyUITex, 1, 1, 1, 1);
		break;

	case GamePlayState:
		m_Renderer->DrawTextureRectSeqXY(0, 0, 0, 900, 800, 1, 1, 1, 1, m_PlayUITex, 1, 1, 1, 1);

		// 시간 그리기
		// Time API 추가해야함
		TimeRenderer();

		// 생명 그리기
		for (int i = 0; i < info.life; ++i) {
			m_Renderer->DrawTextureRect(390.f, -340.f + (60.f * i), 1.f, R_ITEM * 2, R_ITEM * 2, 1, 1, 1, 1, m_LifeTex);
		}

		// 플레이어 캐릭터, 상대 캐릭터 그리기
		int seqX, seqY;
		seqX = g_Seq % 8;
		seqY = (int)(g_Seq / 4);

		g_Seq++;
		if (g_Seq > 32)
			g_Seq = 0;


		for (int i = 0; i < MAX_PLAYERS; ++i) {
			m_Renderer->DrawTextureRectSeqXY(info.playersPos[i].x, info.playersPos[i].y, 1.f, R_PLAYER * 2, R_PLAYER * 2, 1, 1, 1, 1, m_PlayerTex[i], seqX, seqY, 8, 4);
			if (i == info.playerID) {
				m_Renderer->DrawTextureRect(info.playersPos[i].x, info.playersPos[i].y + 50.f, 1.f, 20.f, 20.f, 1, 1, 1, 1, m_ArrowTex);
			}
		}

		// 아이템(사과 + 총알) 그리기
		for (int i = 0; i < MAX_ITEMS; ++i) {
			if (info.items[i].isVisible == false)
				continue;
			else {
				float itemPosX = info.items[i].pos.x;
				float itemPosY = info.items[i].pos.y;

				switch (info.items[i].playerID) {
				case nullPlayer: // 사과
					m_Renderer->DrawTextureRect(itemPosX, itemPosY, 1.f, R_ITEM * 2, R_ITEM * 2, 1, 1, 1, 1, m_ItemTex);
					break;

				case player1:
					m_Renderer->DrawTextureRectSeqXY(itemPosX, itemPosY, 1.f, R_ITEM * 2, R_ITEM * 2, 1, 1, 1, 1, m_BulletTex[player1], 1, 1, 1, 1);
					break;

				case player2:
					m_Renderer->DrawTextureRectSeqXY(itemPosX, itemPosY, 1.f, R_ITEM * 2, R_ITEM * 2, 1, 1, 1, 1, m_BulletTex[player2], 1, 1, 1, 1);
					break;

				default:
					printf("item[itemNum]->playerID 오류!\n");
				}
			}
		}

		break;

	case WinState:
		m_Renderer->DrawTextureRectSeqXY(0, 0, 0, 900, 800, 1, 1, 1, 1, m_ResultWinUITex, 1, 1, 1, 1);
		break;

	case LoseState:
		m_Renderer->DrawTextureRectSeqXY(0, 0, 0, 900, 800, 1, 1, 1, 1, m_ResultLoseUITex, 1, 1, 1, 1);
		break;

	default:
		printf("gameState 오류!\n");
		return;
	}
}

void ScnMgr::TimeRenderer() {
	// 현재 시간을 1000으로 나눠서.. 몇초인지 세고..
	unsigned int time = (unsigned int)(info.gameTime) / 1000;
	unsigned int leaveTime = GAMEOVER_TIME - time;

	// 그걸 다시 10으로도 나눠서..
	unsigned int sec = leaveTime % 10;
	unsigned int tenSec = 0;

	if (leaveTime >= 10 && leaveTime < GAMEOVER_TIME) {
		tenSec = leaveTime / 10;
	}

	if (leaveTime == 100) {
		m_Renderer->DrawTextureRect(363.f, 320.f, 1.f, 30.f, 50.f, 1, 1, 1, 1, m_TimeTex[1]);
		m_Renderer->DrawTextureRect(385.f, 320.f, 1.f, 30.f, 50.f, 1, 1, 1, 1, m_TimeTex[0]);
		m_Renderer->DrawTextureRect(412.f, 320.f, 1.f, 30.f, 50.f, 1, 1, 1, 1, m_TimeTex[0]);
	}
	else {
		m_Renderer->DrawTextureRect(375.f, 320.f, 1.f, 30.f, 50.f, 1, 1, 1, 1, m_TimeTex[tenSec]);
		m_Renderer->DrawTextureRect(405.f, 320.f, 1.f, 30.f, 50.f, 1, 1, 1, 1, m_TimeTex[sec]);
	}
}