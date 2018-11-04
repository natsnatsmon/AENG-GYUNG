#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include "stdafx.h"
#include "ScnMgr.h"
#include <math.h>

int g_Seq = 0;

ScnMgr::ScnMgr()
{
	m_Renderer = NULL;

	m_Renderer = new Renderer(900, 800);
	if (!m_Renderer->IsInitialized())
	{
		std::cout << "Renderer could not be initialized.. \n";
	}

	m_TestTexture = m_Renderer->CreatePngTexture("./textures/texture.png");
	m_TexSeq = m_Renderer->CreatePngTexture("./textures/p1_sprite.png");

	m_testFloorTex = m_Renderer->CreatePngTexture("./textures/AG_testUI.png");
	m_lifeTex = m_Renderer->CreatePngTexture("./textures/life.png");
	m_bulletTex = m_Renderer->CreatePngTexture("./textures/bullet_p1.png");
	m_itemTex = m_Renderer->CreatePngTexture("./textures/item.png");

	for (int i = 0; i < MAX_OBJECTS; i++)
	{
		m_Objects[i] = NULL;
	}
	//Create Hero Object
	m_Objects[HERO_ID] = new Object();
	m_Objects[HERO_ID]->SetPos(0, 0, 20.f);
	m_Objects[HERO_ID]->SetSize(80, 80);
	m_Objects[HERO_ID]->SetColor(1, 1, 1, 1);
	m_Objects[HERO_ID]->SetVel(0.f, 0.f);
	m_Objects[HERO_ID]->SetAcc(0.f, 0.f);
	m_Objects[HERO_ID]->SetMass(1.f);
	m_Objects[HERO_ID]->SetCoefFrict(60.8f);
	m_Objects[HERO_ID]->SetKind(KIND_HERO);
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

void ScnMgr::RenderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.3f, 0.3f, 1.0f);

	// 리소스 테스트용으로 만들었는데 좌표 맞춘거니까 지우지 마세영!
	// 바닥
	m_Renderer->DrawTextureRect(0, 0, 0, 900, 800, 1, 1, 1, 1, m_testFloorTex);
	
	// 아이템
	m_Renderer->DrawTextureRectSeqXY(250, 250, 0, 40, 40, 1, 1, 1, 1, m_itemTex,1,1,1,1);

	// 총알
	m_Renderer->DrawTextureRectSeqXY(100, 100, 0, 40, 40, 1, 1, 1, 1, m_bulletTex,1,1,1,1);

	// 생명
	m_Renderer->DrawTextureRectSeqXY(390, -330, 0, 40, 40, 1, 1, 1, 1, m_lifeTex, 1, 1, 1, 1);


	for (int i = 0; i < MAX_OBJECTS; i++)
	{
		if (m_Objects[i] != NULL)
		{
			float x, y, z, sizeX, sizeY;
			m_Objects[i]->GetPos(&x, &y, &z);
			m_Objects[i]->GetSize(&sizeX, &sizeY);

			float newX, newY, newZ;
			newX = 2.f * x;
			newY = 2.f * y;
			newZ = 2.f * z;


			int seqX, seqY;
			seqX = g_Seq % 4;
			seqY = 1;

			g_Seq++;
			if (g_Seq > 5)
				g_Seq = 0;


			// Renderer Test
			//m_Renderer->DrawSolidRect(x * 2.f, y * 2.f, 0, sizeX,sizeY, 1, 0, 1, 1);//미터로 바꾸기 위해서 x,y 에 100 씩 곱한다.
			//m_Renderer->DrawTextureRect(x * 2.f, y * 2.f, 0, sizeX, sizeY, 1, 1, 1, 1, m_TestTexture);
			//m_Renderer->DrawTextureRectHeight(newX, newY, 1.f, sizeX, sizeY, 1, 1, 1, 1, m_TestTexture, newZ);
			
			// 플레이어
			m_Renderer->DrawTextureRectSeqXY(newX, newY, 1.f, sizeX, sizeY, 1, 1, 1, 1, m_TexSeq, seqX, seqY, 4, 1);
		}
	}
}

void ScnMgr::Update(float eTime)
{
	for (int i = 0; i < MAX_OBJECTS; i++)
	{
		if(m_Objects[i] != NULL)
			m_Objects[i]->Update(eTime);
	}
}

void ScnMgr::ApplyForce(float x, float y, float eTime)
{
	m_Objects[HERO_ID]->ApplyForce(x, y, eTime);
	/*for (int i = 0; i < MAX_OBJECTS; i++)
	{
		if (m_Objects[i] != NULL)
			m_Objects[i]->ApplyForce(x, y, eTime);
	}*/
}


void ScnMgr::AddObject(float x, float y, float z,
	float sx, float sy,
	float vx, float vy)
{
	int id = FindEmptyObjectSlot();
	if (id < 0)
	{
		return;
	}
	m_Objects[id] = new Object;
	m_Objects[id]->SetPos(x, y, z);
	m_Objects[id]->SetSize(sx, sy);
	m_Objects[id]->SetColor(1, 1, 1, 1);
	m_Objects[id]->SetVel(vx, vy);
	m_Objects[id]->SetAcc(0.f, 0.f);
	m_Objects[id]->SetMass(1.f);
	m_Objects[id]->SetCoefFrict(60.8f);
	m_Objects[id]->SetKind(KIND_BULLET);

}

void ScnMgr::DeleteObject(int id)
{
	if (m_Objects[id] != NULL)
	{
		delete m_Objects[id];
		m_Objects[id] = NULL;
	}
}

int ScnMgr::FindEmptyObjectSlot()
{
	for (int i = 0; i < MAX_OBJECTS; i++)
	{
		if (m_Objects[i] == NULL)
			return i;
	}
	std::cout << "No more empty slot!\n";
	return -1;
}

void ScnMgr::Shoot(int shootID)
{
	if (shootID == SHOOT_NONE)
		return;
	float amount = 3.f;
	float bvX, bvY;
	bvX = 0.f;
	bvY = 0.f;

	switch (shootID)
	{
	case SHOOT_UP:
		bvX = 0.f;
		bvY = amount;
		break;
	case SHOOT_DOWN:
		bvX = 0.f;
		bvY = -amount;
		break;
	case SHOOT_LEFT:
		bvX = -amount;
		bvY = 0.f;
		break;
	case SHOOT_RIGHT:
		bvX = amount;
		bvY = 0.f;
		break;
	}

	float pX, pY, pZ;
	m_Objects[HERO_ID]->GetPos(&pX, &pY, &pZ);
	float vX, vY;
	m_Objects[HERO_ID]->GetVel(&vX, &vY);
}



