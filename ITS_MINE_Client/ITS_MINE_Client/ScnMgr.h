#pragma once
#include "Object.h"
#include "Renderer.h"

class ScnMgr
{
public:
	ScnMgr();
	~ScnMgr();

	void RenderScene();
	void Update(float eTime);
	void ApplyForce(float x, float y, float eTime);
	void Shoot(int shootID);
	void AddObject(float x, float y, float z,
		float sx, float sy,
		float vx, float vy);
	int FindEmptyObjectSlot();
	void DeleteObject(int id);

private:
	Renderer * m_Renderer;
	Object *m_Objects[MAX_OBJECTS];

	GLuint m_TestTexture = 0;
	GLuint m_TexSeq = 0;

	GLuint m_testFloorTex = 0;
	GLuint m_lifeTex = 0;
	GLuint m_bulletTex = 0;
	GLuint m_itemTex = 0;


	// 플레이어는 지금 사용 안하고있습니당
	GLuint m_player1Tex = 0;
	GLuint m_player2Tex = 0;
	GLuint m_player3Tex = 0;
	GLuint m_player4Tex = 0;

};

