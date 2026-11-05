#ifndef MACRO_H_INCLUDED
#define MACRO_H_INCLUDED

#define DELETE_V(p) do{ if(p){ delete    p; p = NULL; } } while(false)	//	NULL 確認付き変数開放
#define DELETE_A(p) do{ if(p){ delete [] p; p = NULL; } } while(false)	//	NULL 確認付き配列開放

#define V2Norm D3DXVec2Normalize	//	ベクトル正規化
#define V2Len D3DXVec2Length		//	ベクトル絶対値
#define V2Dot D3DXVec2Dot			//	ベクトル内積

#define V3Norm D3DXVec3Normalize	//	ベクトル正規化
#define V3Len D3DXVec3Length		//	ベクトル絶対値
#define V3Dot D3DXVec3Dot			//	ベクトル内積
#define V3Cross D3DXVec3Cross		//	ベクトル外積

// 右辺値を左辺値に変換する (gcc対応)
template< class TYPE > inline TYPE& R2L( const TYPE& _Value )
{
	return const_cast< TYPE& >( _Value );
}

#endif
