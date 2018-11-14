#pragma once
#include "Object.h"
#include "Renderer.h"
#include "global.h"

class ScnMgr
{
public:
	ScnMgr();
	~ScnMgr();

	void RenderScene();

	// ★ 필요 없다고 생각해서 주석처리했습니당

	//void AddObject(float x, float y, float z,
	//	float sx, float sy,
	//	float vx, float vy);
	//int FindEmptyObjectSlot();
	//void DeleteObject(int id);

private:
	Renderer *m_Renderer;
	Object *m_Objects[MAX_OBJECTS];

	//GLuint m_TestTexture = 0;
	//GLuint m_TexSeq = 0;

	GLuint m_PlayerTex[MAX_PLAYERS] = { 0, };
	GLuint m_BulletTex[MAX_PLAYERS] = { 0, };
	GLuint m_ItemTex = 0;
	GLuint m_LifeTex = 0;
	GLuint m_StartUITex = 0;
	GLuint m_LobbyUITex = 0;
	GLuint m_PlayUITex = 0;
	GLuint m_ResultUITex[MAX_PLAYERS] = { 0, };
};

