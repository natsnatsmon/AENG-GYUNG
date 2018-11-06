#pragma once
#include "Object.h"
#include "Renderer.h"

class ScnMgr
{
public:
	ScnMgr();
	~ScnMgr();

	void RenderScene();
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

};

