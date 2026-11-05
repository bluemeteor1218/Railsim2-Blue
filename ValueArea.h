#ifndef VALUEAREA_H_INCLUDED
#define VALUEAREA_H_INCLUDED

/*
 *	変数の値を一定の範囲に丸める (int)
 */
inline void ValueArea(
	int *dest,	//	対象値
	int min,	//	最小値
	int max		//	最大値
){
	if(*dest<min) *dest = min;
	if(*dest>max) *dest = max;
}

/*
 *	変数の値を一定の範囲に丸める (float)
 */
inline void ValueArea(
	float *dest,	//	対象値
	float min,		//	最小値
	float max		//	最大値
){
	if(*dest<min) *dest = min;
	if(*dest>max) *dest = max;
}

/*
 *	変数の値を一定の範囲でループさせる (char)
 */
inline void ValueCircular(
	char *dest,	//	対象値
	char min,	//	最小値
	char max		//	最大値
){
	char area = max-min+1;
	*dest = (*dest+area)%area;
}

/*
 *	変数の値を一定の範囲でループさせる (int)
 */
inline void ValueCircular(
	int *dest,	//	対象値
	int min,	//	最小値
	int max		//	最大値
){
	int area = max-min+1;
	*dest = (*dest+area)%area;
}

/*
 *	変数の値を一定の範囲でループさせる (float)
 */
inline void ValueCircular(
	float *dest,	//	対象値
	float min,		//	最小値
	float max		//	最大値
){
	float area = max-min, tmp = *dest-min;
	*dest = tmp-area*(int)(tmp/area)+min;
}

/*
 *	変数の値が一定の範囲内か調べる (int)
 */
inline bool CheckValueArea(
	int target,	//	対象値
	int min,	//	最小値
	int max		//	最大値
){
	return (target>=min) && (target<=max);
}

/*
 *	座標が一定矩形内にあるか調べる
 */
inline bool CheckRect(
	int tx,	//	対象左上 x
	int ty,	//	対象左上 y
	int x,	//	矩形左上 x
	int y,	//	矩形左上 y
	int w,	//	矩形幅
	int h	//	矩形高さ
){
	tx -= x;
	ty -= y;
	return 0<=tx && tx<w && 0<=ty && ty<h;
}

#endif
