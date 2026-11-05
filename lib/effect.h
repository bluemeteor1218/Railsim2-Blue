//	Copyright (c) 2002 Midikyou

D3DCOLOR GetPixelColor(int x, int y);
void RenderLensFlare(VEC3 pos, float size, BOOL fWhite);

/*
 *	ビルボード変換
 */
inline void devTransBillboard(VEC3 pos){
	MTX4 mtx = sv3.mtxViewInv;
	mtx._41 = pos.x, mtx._42 = pos.y, mtx._43 = pos.z;
	devTransform(&mtx);
}
