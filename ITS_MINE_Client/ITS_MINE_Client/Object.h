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

	void GetVel(float *x, float *y);
	void SetVel(float x, float y);

	void GetAcc(float *x, float *y);
	void SetAcc(float x, float y);

	void GetMass(float *x);
	void SetMass(float x);

	void GetCoefFrict(float *x);
	void SetCoefFrict(float x);

	void SetKind(int kind);
	void GetKint(int *kind);



	void ApplyForce(float x, float y, float eTime);

	void Update(float eTime);
private:
	float m_PosX, m_PosY, m_PosZ;//position
	float m_SizeX, m_SizeY;//size
	float m_r, m_g, m_b, m_a;//color
	float m_VelX, m_VelY;//velocity
	float m_AccX, m_AccY;//accelaration
	float m_Mass;//mass
	float m_CoefFrict;//¸¶Âû°è¼ö

	int m_kind;
};

