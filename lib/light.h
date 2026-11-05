//	Copyright (c) 2002 Midikyou

struct SYSVALUE_L{
	D3DLIGHT8	dir;	//	ïΩçsåıåπ
	BOOL		fDir;	//	ON/OFF
};
extern SYSVALUE_L svl;

void SetDirLight(VEC3 dir, D3DCOLORVALUE cv);
void EnableDirLight(BOOL f);
MTX4 GetShadowMtx(VEC3 point, VEC3 normal);
