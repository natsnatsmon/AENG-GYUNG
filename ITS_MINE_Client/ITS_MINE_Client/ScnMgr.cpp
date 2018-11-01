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
	m_TexSeq = m_Renderer->CreatePngTexture("./textures/character.png");
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
			seqX = g_Seq % 8;
			seqY = (int)(g_Seq / 8);

			g_Seq++;
			if (g_Seq > 16)
				g_Seq = 0;


			// Renderer Test
			//m_Renderer->DrawSolidRect(x * 2.f, y * 2.f, 0, sizeX,sizeY, 1, 0, 1, 1);//미터로 바꾸기 위해서 x,y 에 100 씩 곱한다.
			//m_Renderer->DrawTextureRect(x * 2.f, y * 2.f, 0, sizeX, sizeY, 1, 1, 1, 1, m_TestTexture);
			//m_Renderer->DrawTextureRectHeight(newX, newY, 1.f, sizeX, sizeY, 1, 1, 1, 1, m_TestTexture, newZ);
			m_Renderer->DrawTextureRectSeqXY(newX, newY, 1.f, sizeX, sizeY, 1, 1, 1, 1, m_TexSeq, seqX, seqY, 8, 2);
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



