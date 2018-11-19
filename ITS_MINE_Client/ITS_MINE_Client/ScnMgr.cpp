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
	for (int playerID = 0; playerID < MAX_PLAYERS; ++playerID) {
		std::string filePath = "./textures/";
		std::string playerFileName = filePath + "p" + std::to_string(playerID + 1) + "_sprite.png";
		std::string bulletFileName = filePath + "p" + std::to_string(playerID + 1) + "_bullet.png";
		std::string resultFileName = filePath + "p" + std::to_string(playerID + 1) + "_resultUI.png";

		char playerTexFilePath[50];
		char bulletTexFilePath[50];
		char resultUITexFilePath[50];
		strcpy_s(playerTexFilePath, playerFileName.c_str());
		strcpy_s(bulletTexFilePath, bulletFileName.c_str());
		strcpy_s(resultUITexFilePath, resultFileName.c_str());

		m_PlayerTex[playerID] = m_Renderer->CreatePngTexture(playerTexFilePath);
		m_BulletTex[playerID] = m_Renderer->CreatePngTexture(bulletTexFilePath);
		m_ResultUITex[playerID] = m_Renderer->CreatePngTexture(resultUITexFilePath);
	}

	m_ItemTex = m_Renderer->CreatePngTexture("./textures/item.png");
	m_LifeTex = m_Renderer->CreatePngTexture("./textures/life.png");

	m_StartUITex = m_Renderer->CreatePngTexture("./textures/startUI.png");
	m_LobbyUITex = m_Renderer->CreatePngTexture("./textures/waitUI_1.png");
	m_PlayUITex = m_Renderer->CreatePngTexture("./textures/playUI.png");
}

ScnMgr::~ScnMgr()
{
	if (m_Renderer)
	{
		delete m_Renderer;
		m_Renderer = NULL;
	}
	if (m_Objects[HERO_ID])
	{
		delete m_Objects[HERO_ID];
		m_Objects[HERO_ID] = NULL;
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

		// 플레이어 캐릭터, 상대 캐릭터 그리기
		int seqX, seqY;
		seqX = g_Seq % 4;
		seqY = 1;

		g_Seq++;
		if (g_Seq > 4)
			g_Seq = 0;

		for (int playerID = 0; playerID < MAX_PLAYERS; ++playerID) {
			m_Renderer->DrawTextureRectSeqXY(info.playersPos[playerID].x, info.playersPos[playerID].y, 1.f, R_PLAYER * 2, R_PLAYER * 2, 1, 1, 1, 1, m_PlayerTex[playerID], seqX, seqY, 4, 1);
		}

		//// 아이템(사과 + 총알) 그리기
		//for (int itemNum = 0; itemNum < MAX_ITEMS; ++itemNum) {
		//	if (info.items[itemNum].isVisible == false)
		//		continue;
		//	else {
		//		float itemPosX = info.items[itemNum].pos.x;
		//		float itemPosY = info.items[itemNum].pos.y;

		//		switch (info.items[itemNum].playerID) {
		//		case nullPlayer: // 사과
		//			m_Renderer->DrawTextureRectSeqXY(itemPosX, itemPosY, 1.f, R_ITEM * 2, R_ITEM * 2, 1, 1, 1, 1, m_ItemTex, 1, 1, 1, 1);
		//			break;

		//		case player1:
		//			m_Renderer->DrawTextureRectSeqXY(itemPosX, itemPosY, 1.f, R_ITEM * 2, R_ITEM * 2, 1, 1, 1, 1, m_BulletTex[player1], 1, 1, 1, 1);
		//			break;

		//		case player2:
		//			m_Renderer->DrawTextureRectSeqXY(itemPosX, itemPosY, 1.f, R_ITEM * 2, R_ITEM * 2, 1, 1, 1, 1, m_BulletTex[player2], 1, 1, 1, 1);
		//			break;

		//		case player3:
		//			m_Renderer->DrawTextureRectSeqXY(itemPosX, itemPosY, 1.f, R_ITEM * 2, R_ITEM * 2, 1, 1, 1, 1, m_BulletTex[player3], 1, 1, 1, 1);
		//			break;

		//		case player4:
		//			m_Renderer->DrawTextureRectSeqXY(itemPosX, itemPosY, 1.f, R_ITEM * 2, R_ITEM * 2, 1, 1, 1, 1, m_BulletTex[player4], 1, 1, 1, 1);
		//			break;

		//		default:
		//			printf("item[itemNum]->playerID 오류!\n");
		//		}
		//	}
		//}

		break;

	case GameOverState:
		m_Renderer->DrawTextureRectSeqXY(0, 0, 0, 900, 800, 1, 1, 1, 1, m_ResultUITex[0], 1, 1, 1, 1);
		break;

	default:
		printf("gameState 오류!\n");
		return;
	}
}

// ★ 필요 없다고 생각해서 주석처리했습니당

//void ScnMgr::AddObject(float x, float y, float z,
//	float sx, float sy,
//	float vx, float vy)
//{
//	int id = FindEmptyObjectSlot();
//	if (id < 0)
//	{
//		return;
//	}
//	m_Objects[id] = new Object;
//	m_Objects[id]->SetPos(x, y, z);
//	m_Objects[id]->SetSize(sx, sy);
//	m_Objects[id]->SetColor(1, 1, 1, 1);
//	m_Objects[id]->SetKind(KIND_BULLET);
//
//}
//
//void ScnMgr::DeleteObject(int id)
//{
//	if (m_Objects[id] != NULL)
//	{
//		delete m_Objects[id];
//		m_Objects[id] = NULL;
//	}
//}
//
//int ScnMgr::FindEmptyObjectSlot()
//{
//	for (int i = 0; i < MAX_OBJECTS; i++)
//	{
//		if (m_Objects[i] == NULL)
//			return i;
//	}
//	std::cout << "No more empty slot!\n";
//	return -1;
//}
//
