#include "CPixelbit.h"

//	関数宣言
void Dialog(char *, ...);

#ifdef ENABLE_RECT_BOUNDARY

/*
 *	描画制限領域の初期化
 */
void CPixelbit::ResetBoundary(){
	m_BoundLeft = m_BoundTop = 0;
	m_BoundRight = m_Width;
	m_BoundBottom = m_Height;
}

/*
 *	描画制限領域の設定
 */
void CPixelbit::SetBoundary(
	int x, int y,	//	座標
	int w, int h	//	サイズ
){
	m_BoundLeft = x;
	m_BoundTop = y;
	m_BoundRight = x+w;
	m_BoundBottom = y+h;
	if(m_BoundLeft<0) m_BoundLeft = 0;
	if(m_BoundTop<0) m_BoundTop = 0;
	if(m_BoundRight>m_Width) m_BoundRight = m_Width;
	if(m_BoundBottom>m_Height) m_BoundBottom = m_Height;
}

#endif	//	ENABLE_RECT_BOUNDARY

/*
 *	アルファチャネルを指定値でリセット
 */
void CPixelbit::ResetAlphaChannel(
	BYTE alpha	//	アルファ値
){
	int i;
	for(i = 0; i<m_Height; i++){
		PDWORD line = m_PixelAdr+m_Width*i, stop = line+m_Width;
		for(; line<stop; line++) *((PBYTE)line+3) = alpha;
	}
}

/*
 *	コピー時の矩形範囲を画像内に補正
 *
 *	戻り値: コピー範囲があれば TRUE を返す
 */
BOOL CPixelbit::FixLocation(
	CPixelbit *dest,	//	コピー先画像
	int *dx, int *dy,	//	コピー先座標
	int *sx, int *sy,	//	コピー元座標
	int *w, int *h		//	コピーサイズ
){
#ifdef ENABLE_RECT_BOUNDARY
#define BOUNDLEFT	dest->m_BoundLeft
#define BOUNDTOP	dest->m_BoundTop
#else
#define BOUNDLEFT	0
#define BOUNDTOP	0
#endif
	if(*dx<=BOUNDLEFT-m_Width || *dy<=BOUNDTOP-m_Height
		|| *dx>=dest->m_BoundRight || *dy>=dest->m_BoundBottom) return FALSE;
	int delta;
	if((delta = BOUNDLEFT-*dx)>0){
		*w -= delta; *sx += delta;
		*dx = BOUNDLEFT;
	}
	if((delta = BOUNDTOP-*dy)>0){
		*h -= delta; *sy += delta;
		*dy = BOUNDTOP;
	}
	if((delta = *dx+*w-dest->m_BoundRight)>0) *w -= delta;
	if((delta = *dy+*h-dest->m_BoundBottom)>0) *h -= delta;
	return *w>0 && *h>0;
}

/*
 *	指定デバイスコンテキストに描画
 *
 *	戻り値: 成功したら TRUE を返す
 */
BOOL CPixelbit::WindowStamp(
	HDC hdc,		//	デバイスコンテキスト
	int dx, int dy,	//	コピー先座標
	int sx, int sy,	//	コピー元座標
	int w, int h	//	コピーサイズ
){
	return m_ColorKey==0xffffffff ? SetDIBitsToDevice(hdc, dx, dy, w, h,
		sx, m_Height-sy-h, 0, m_Height, m_PixelAdr, &m_BmpInfo, DIB_RGB_COLORS)
		: ms_TransBlt(hdc, dx, dy, w, h, GetHDC(), sx, sy, w, h, m_ColorKey);
}

/*
 *	矩形範囲のコピー
 *
 *	戻り値: 成功したら TRUE を返す
 */
BOOL CPixelbit::PlainStamp(
	CPixelbit *dest,	//	コピー先画像
	int dx, int dy,		//	コピー先座標
	int sx, int sy,		//	コピー元座標
	int w, int h		//	コピーサイズ
){
	if(!FixLocation(dest, &dx, &dy, &sx, &sy, &w, &h)) return FALSE;
	return m_ColorKey==0xffffffff ? SetDIBitsToDevice(dest->m_BmpHdc, dx, dy, w, h,
		sx, m_Height-sy-h, 0, m_Height, m_PixelAdr, &m_BmpInfo, DIB_RGB_COLORS)
		: ms_TransBlt(dest->m_BmpHdc, dx, dy, w, h, GetHDC(), sx, sy, w, h, m_ColorKey);
}

/*
 *	矩形範囲のコピー
 *
 *	戻り値: 成功したら TRUE を返す
 */
BOOL CPixelbit::PlainStamp32(
	CPixelbit *dest,	//	コピー先画像
	int dx, int dy,		//	コピー先座標
	int sx, int sy,		//	コピー元座標
	int w, int h		//	コピーサイズ
){
	int i;
	if(!FixLocation(dest, &dx, &dy, &sx, &sy, &w, &h)) return FALSE;
	for(i = 0; i<h; i++){
		PDWORD line1 = m_PixelAdr+m_Width*(i+sy)+sx, stop = line1+w;
		PDWORD line2 = dest->m_PixelAdr+dest->m_Width*(i+dy)+dx;
		while(line1<stop) *line2++ = *line1++;
	}
	return TRUE;
}

/*
 *	アルファブレンドコピー
 *
 *	戻り値: 成功したら TRUE を返す
 */
BOOL CPixelbit::AlphaBlendStamp(
	CPixelbit *dest,	//	コピー先画像
	int dx, int dy,		//	コピー先座標
	int sx, int sy,		//	コピー元座標
	int w, int h		//	コピーサイズ
){
	int i;
	if(!FixLocation(dest, &dx, &dy, &sx, &sy, &w, &h)) return FALSE;
	for(i = 0; i<h; i++){
		PDWORD line1 = m_PixelAdr+m_Width*(i+sy)+sx, stop = line1+w;
		PDWORD line2 = dest->m_PixelAdr+dest->m_Width*(i+dy)+dx;
		while(line1<stop){
			int alpha, r1, g1, b1, r2, g2, b2;
			SplitColorA(*line1, &alpha, &r1, &g1, &b1);
			SplitColor(*line2, &r2, &g2, &b2);
			int beta = 255-alpha;
			PBYTE c = (PBYTE)line2;
			*c = (b1*alpha+b2*beta)/255; c++;
			*c = (g1*alpha+g2*beta)/255; c++;
			*c = (r1*alpha+r2*beta)/255;
			line1++; line2++;
		}
	}
	return TRUE;
}

/*
 *	伸縮コピー
 *
 *	戻り値: 成功したら TRUE を返す
 */
BOOL CPixelbit::StretchStamp(
	CPixelbit *dest,	//	コピー先画像
	int dx, int dy,		//	コピー先座標
	int dw, int dh,		//	コピー先サイズ
	int sx, int sy,		//	コピー元座標
	int sw, int sh,		//	コピー元サイズ
	int halftone		//	伸縮モード
){
	SetStretchBltMode(dest->m_BmpHdc, halftone ? HALFTONE : COLORONCOLOR);
	return StretchBlt(dest->m_BmpHdc, dx, dy, dw, dh, m_BmpHdc, sx, sy, sw, sh, SRCCOPY);
}

/*
 *	バイリニア伸縮コピー (範囲チェックなし)
 *
 *	戻り値: 成功したら TRUE を返す
 */
BOOL CPixelbit::BilinearStamp(
	CPixelbit *dest,	//	コピー先画像
	int dx, int dy,		//	コピー先座標
	int dw, int dh,		//	コピー先サイズ
	int sx, int sy,		//	コピー元座標
	int sw, int sh		//	コピー元サイズ
){
	if(dw==sw && dh==sh) return PlainStamp(dest, dx, dy, sx, sy, sw, sh);
	bool first = true;
	int a1, r1, g1, b1, a2, r2, g2, b2;
	DWORD x, y, fx, fy, tx, ty, tw, th;
	CPixelbit temp, *from, *to;
	if(sw*dh<dw*sh) goto VERTICAL;
HORIZONTAL:
	if(dw!=sw){
		tw = dw;
		if(first || dh==sh){
			from = this; fx = sx; fy = sy;
		}else{
			from = &temp; fx = fy = 0;
		}
		if(!first || dh==sh){
			to = dest; tx = dx; ty = dy; th = dh;
		}else{
			to = &temp; tx = ty = 0; th = sh;
			temp.Clear(tw, th);
		}
		if(dw<sw){
			DWORD div = sw/tw, mod = sw%tw;
			double ratio = (double)tw/sw;
			for(y = 0; y<th; y++){
				DWORD alpha = 0;
				PDWORD line1 = from->m_PixelAdr+from->m_Width*(y+fy)+fx;
				PBYTE line2 = (PBYTE)(to->m_PixelAdr+to->m_Width*(y+ty)+tx);
				for(x = 0; x<tw; x++){
					DWORD i = div, j = mod;
					double asum = 0.0, rsum = 0.0, gsum = 0.0, bsum = 0.0;
					if(alpha){
						double beta = (double)(tw-alpha)/tw;
						SplitColorA(*line1++, &a1, &r1, &g1, &b1);
						asum += a1*beta; rsum += r1*beta;
						gsum += g1*beta; bsum += b1*beta;
						i--;
					}
					do{
						if(!i){
							if((alpha += mod)<tw) break;
							alpha -= tw;
						}
						SplitColorA(*line1++, &a1, &r1, &g1, &b1);
						asum += a1; rsum += r1; gsum += g1; bsum += b1;
					} while(i--);
					if(alpha){
						double beta = (double)alpha/tw;
						SplitColorA(*line1, &a1, &r1, &g1, &b1);
						asum += a1*beta; rsum += r1*beta;
						gsum += g1*beta; bsum += b1*beta;
					}
					*line2++ = (BYTE)(ratio*bsum+0.5);
					*line2++ = (BYTE)(ratio*gsum+0.5);
					*line2++ = (BYTE)(ratio*rsum+0.5);
					*line2++ = (BYTE)(ratio*asum+0.5);
				}
			}
		}else{
			DWORD round = tw/2;
			for(y = 0; y<th; y++){
				DWORD alpha = (tw+sw)/2;
				PDWORD begin = from->m_PixelAdr+from->m_Width*(y+fy)+fx;
				PDWORD line1 = begin-1, end = line1+sw;
				PBYTE line2 = (PBYTE)(to->m_PixelAdr+to->m_Width*(y+ty)+tx);
				for(x = 0; x<tw; x++){
					SplitColorA(*(line1<begin ? begin : line1), &a1, &r1, &g1, &b1);
					SplitColorA(*(line1<end ? line1+1 : end), &a2, &r2, &g2, &b2);
					DWORD beta = tw-alpha;
					*line2++ = (BYTE)((b1*beta+b2*alpha+round)/tw);
					*line2++ = (BYTE)((g1*beta+g2*alpha+round)/tw);
					*line2++ = (BYTE)((r1*beta+r2*alpha+round)/tw);
					*line2++ = (BYTE)((a1*beta+a2*alpha+round)/tw);
					if((alpha += sw)>=tw){
						line1++;
						alpha -= tw;
					}
				}
			}
		}
	}
	if(!first) return TRUE;
	first = false;
VERTICAL:
	if(dh!=sh){
		th = dh;
		if(first || dw==sw){
			from = this; fx = sx; fy = sy;
		}else{
			from = &temp; fx = fy = 0;
		}
		if(!first || dw==sw){
			to = dest; tx = dx; ty = dy; tw = dw;
		}else{
			to = &temp; tx = ty = 0; tw = sw;
			temp.Clear(tw, th);
		}
		if(dh<sh){
			DWORD div = sh/th, mod = sh%th;
			double ratio = (double)th/sh;
			for(x = 0; x<tw; x++){
				DWORD alpha = 0;
				PDWORD line1 = from->m_PixelAdr+from->m_Width*fy+x+fx;
				PBYTE line2 = (PBYTE)(to->m_PixelAdr+to->m_Width*ty+x+tx);
				for(y = 0; y<th; y++){
					DWORD i = div, j = mod;
					double asum = 0.0, rsum = 0.0, gsum = 0.0, bsum = 0.0;
					if(alpha){
						double beta = (double)(th-alpha)/th;
						SplitColorA(*line1, &a1, &r1, &g1, &b1);
						line1 += from->m_Width;
						asum += a1*beta; rsum += r1*beta;
						gsum += g1*beta; bsum += b1*beta;
						i--;
					}
					do{
						if(!i){
							if((alpha += mod)<th) break;
							alpha -= th;
						}
						SplitColorA(*line1, &a1, &r1, &g1, &b1);
						line1 += from->m_Width;
						asum += a1; rsum += r1; gsum += g1; bsum += b1;
					} while(i--);
					if(alpha){
						double beta = (double)alpha/th;
						SplitColorA(*line1, &a1, &r1, &g1, &b1);
						asum += a1*beta; rsum += r1*beta;
						gsum += g1*beta; bsum += b1*beta;
					}
					*line2++ = (BYTE)(ratio*bsum+0.5);
					*line2++ = (BYTE)(ratio*gsum+0.5);
					*line2++ = (BYTE)(ratio*rsum+0.5);
					*line2 = (BYTE)(ratio*asum+0.5);
					line2 += (to->m_Width<<2)-3;
				}
			}
		}else{
			DWORD round = th/2;
			for(x = 0; x<tw; x++){
				DWORD alpha = (th+sh)/2;
				PDWORD begin = from->m_PixelAdr+from->m_Width*fy+x+fx;
				PDWORD line1 = begin-from->m_Width, end = line1+from->m_Width*sh;
				PBYTE line2 = (PBYTE)(to->m_PixelAdr+to->m_Width*ty+x+tx);
				for(y = 0; y<th; y++){
					SplitColorA(*(line1<begin ? begin : line1), &a1, &r1, &g1, &b1);
					SplitColorA(*(line1<end ? line1+from->m_Width : end), &a2, &r2, &g2, &b2);
					DWORD beta = th-alpha;
					*line2++ = (BYTE)((b1*beta+b2*alpha+round)/th);
					*line2++ = (BYTE)((g1*beta+g2*alpha+round)/th);
					*line2++ = (BYTE)((r1*beta+r2*alpha+round)/th);
					*line2 = (BYTE)((a1*beta+a2*alpha+round)/th);
					line2 += (to->m_Width<<2)-3;
					if((alpha += sh)>=th){
						line1 += from->m_Width;
						alpha -= th;
					}
				}
			}
		}
	}
	if(!first) return TRUE;
	first = false;
	goto HORIZONTAL;
}

/*
 *	最近点伸縮コピー (範囲チェックなし)
 *
 *	戻り値: 成功したら TRUE を返す
 */
BOOL CPixelbit::NearestStamp(
	CPixelbit *dest,	//	コピー先画像
	int dx, int dy,		//	コピー先座標
	int dw, int dh,		//	コピー先サイズ
	int sx, int sy,		//	コピー元座標
	int sw, int sh		//	コピー元サイズ
){
	if(dw==sw && dh==sh) return PlainStamp(dest, dx, dy, sx, sy, sw, sh);
	int wdiv = sw/dw, wmod = sw%dw, wrm = (sw/2)%dw;
	int hdiv = sh/dh, hmod = sh%dh, alpha = (sh/2)%dh, i;
	PDWORD line0 = m_PixelAdr+m_Width*(sy+sh/(dh*2))+sx+sw/(dw*2);
	for(i = 0; i<dh; i++){
		int beta = wrm;
		PDWORD line2 = dest->m_PixelAdr+dest->m_Width*(i+dy)+dx;
		PDWORD line1 = line0, stop = line2+dw;
		while(line2<stop){
			*line2++ = *line1;
			line1 += wdiv+((beta += wmod)>=dw ? ((beta -= dw), 1) : 0);
		}
		line0 += m_Width*(hdiv+((alpha += hmod)>=dh ? ((alpha -= dh), 1) : 0));
	}
	return TRUE;
}
