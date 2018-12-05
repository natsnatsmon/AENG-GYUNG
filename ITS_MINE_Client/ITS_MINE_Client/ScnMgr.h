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
	void TimeRenderer();

private:
	Renderer *m_Renderer;
	Object *m_Objects[MAX_OBJECTS];

	GLuint m_PlayerTex[MAX_PLAYERS] = { 0, };
	GLuint m_BulletTex[MAX_PLAYERS] = { 0, };
	GLuint m_ArrowTex = 0;
	GLuint m_ItemTex = 0;
	GLuint m_LifeTex = 0;
	GLuint m_TimeTex[10] = { 0, };

	GLuint m_StartUITex = 0;
	GLuint m_LobbyUITex = 0;
	GLuint m_PlayUITex = 0;
	GLuint m_ResultWinUITex = 0;
	GLuint m_ResultLoseUITex = 0;
	GLuint m_ResultDrawUITex = 0;
};

