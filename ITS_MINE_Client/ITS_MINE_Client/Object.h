#pragma once
class Object
{
public:
	Object();
	~Object();
	void GetPos(float *x, float *y, float *z);
	void SetPos(float x, float y, float z);

	void GetSize(float *x, float *y);
	void SetSize(float x, float y);

	void GetColor(float *r, float *g, float *b, float *a);
	void SetColor(float r, float g, float b, float a);


	void SetKind(int kind);
	void GetKint(int *kind);




private:
	float m_PosX, m_PosY, m_PosZ;//position
	float m_SizeX, m_SizeY;//size
	float m_r, m_g, m_b, m_a;//color

	int m_kind;
};

