#include "stdafx.h"
#include "Object.h"


Object::Object()
{
}


Object::~Object()
{
}

void Object::GetPos(float *x, float *y, float *z)
{ 
	*x = m_PosX;
	*y = m_PosY;
	*z = m_PosZ;
}
void Object::SetPos(float x, float y, float z)
{
	this->m_PosX = x;
	this->m_PosY = y;
	this->m_PosZ = z;
}

void Object::GetSize(float *x, float *y)
{
	*x = this->m_SizeX;
	*y = this->m_SizeY;
}
void Object::SetSize(float x, float y)
{
	this->m_SizeX = x;
	this->m_SizeY = y;
}
void Object::GetColor(float *r, float *g, float *b, float *a)
{
	*r = this->m_r;
	*g = this->m_g;
	*b = this->m_b;
	*a = this->m_a;
}
void Object::SetColor(float r, float g, float b, float a)
{
	this->m_r = r;
	this->m_g = g;
	this->m_b = b;
	this->m_a = a;
}


void Object::SetKind(int kind)
{
	this->m_kind = kind;
}
void Object::GetKint(int *kind)
{
	*kind = this->m_kind;
}


