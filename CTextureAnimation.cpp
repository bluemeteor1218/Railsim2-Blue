#include "stdafx.h"
#include "CCustomizerMisc.h"
#include "CModelPlugin.h"

//	外部グローバル
extern bool g_NamedObjectMipMap;
extern bool g_PreviewAnimation;

/*
 *	コンストラクタ
 */
CTexAnimFrame::CTexAnimFrame(){
	m_FrameTexture = NULL;
	m_TexTrans = NULL;
}

/*
 *	コピーコンストラクタ
 */
CTexAnimFrame::CTexAnimFrame(
	const CTexAnimFrame &src	//	コピー元
){
	m_FrameLength = src.m_FrameLength;
	m_TextureFileName = src.m_TextureFileName;
	m_FrameTexture = src.m_FrameTexture;
	m_TexTrans = src.m_TexTrans ? new CTextureTransformer(*src.m_TexTrans) : NULL;
}

/*
 *	コンストラクタ
 */
CTexAnimFrame::CTexAnimFrame(
	string tex,	//	テクスチャファイル名
	int len		//	フレーム長
){
	m_FrameLength = len;
	m_TextureFileName = tex;
	m_FrameTexture = NULL;
	m_TexTrans = NULL;
}

/*
 *	デストラクタ
 */
CTexAnimFrame::~CTexAnimFrame(){
	DELETE_V(m_TexTrans);
}

/*
 *	読込
 */
char *CTexAnimFrame::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	if(!(str = Assignment(str, "Frame"))) return NULL;
	if(!(str = StringLiteral(eee = str, &m_TextureFileName))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_FrameLength))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	CTextureTransformer textrans;
	if(tmp = textrans.Read(str, false)){
		str = tmp;
		m_TexTrans = new CTextureTransformer(textrans);
	}
	if(m_FrameLength<1) m_FrameLength = 1;
	return str;
}

/*
 *	データ読込
 */
void CTexAnimFrame::LoadData(){
	m_FrameTexture = g_TexList.Get(FALSE, m_TextureFileName.c_str(), 0, !g_NamedObjectMipMap);
}

/*
 *	アニメーション適用
 */
void CTexAnimFrame::Apply(
	CMesh *mesh,	//	メッシュ
	int matid		//	マテリアル ID
){
	mesh->SetCustomTexture(matid, m_FrameTexture);
	if(m_TexTrans){
		m_TexTrans->SetMaterialID(matid);
		m_TexTrans->ApplyCustomizer(mesh);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CTexAnimState::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = Assignment(eee = str, "TexAnimState"))) return NULL;
	if(!(str = ConstInteger(eee = str, &m_Frame))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_Count))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	保存
 */
void CTexAnimState::Save(
	FILE *df,	//	ファイル
	char *ind	//	インデント
){
	fprintf(df, "%sTexAnimState = %d, %d;\n", ind, m_Frame, m_Count);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CTextureAnimation::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *tmp, *eee;
	if(!(str = BeginNamedBlock(eee = str, "DefineAnimation", &m_AnimationName))) return NULL;

	if(mpi->FindAnimation(m_AnimationName))
		throw CSynErr(eee, "%s: \"%s\"", lang(OverlappedAnimation), m_AnimationName.c_str());

	m_FrameList.clear();
	while(true){
		int i, numbegin, numend, framecount, framelength;
		string texturefilename;
		CTexAnimFrame frame;
		if(tmp = frame.Read(str)){
			str = tmp;
			m_FrameList.push_back(frame);
		}else if(tmp = Assignment(str, "NumberedFrame")){
			str = tmp;
			if(!(str = StringLiteral(eee = str, &texturefilename))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstInteger(eee = str, &numbegin))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstInteger(eee = str, &numend))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstInteger(eee = str, &framelength))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
			int delta = numbegin<numend ? 1 : -1;
			while(true){
				m_FrameList.push_back(CTexAnimFrame(
					FlashIn((char *)texturefilename.c_str(), numbegin), framelength));
				if(numbegin==numend) break;
				numbegin += delta;
			}
		}else if(tmp = Assignment(str, "SlideUVFrame")){
			str = tmp;
			float u0, v0, du, dv;
			if(!(str = StringLiteral(eee = str, &texturefilename))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstFloat(eee = str, &u0))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstFloat(eee = str, &v0))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstFloat(eee = str, &du))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstFloat(eee = str, &dv))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstInteger(eee = str, &framecount))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstInteger(eee = str, &framelength))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
			for(i = 0; i<framecount; i++){
				m_FrameList.push_back(CTexAnimFrame(texturefilename, framelength));
				float a[2] = {u0+du*i, v0+dv*i};
				(m_FrameList.rbegin()->m_TexTrans
					= new CTextureTransformer)->SetUpMatrix(1, a);
			}
		}else if(tmp = Assignment(str, "TiledUVFrame")){
			str = tmp;
			float su, sv, tw, th;
			if(!(str = StringLiteral(eee = str, &texturefilename))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstFloat(eee = str, &su))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstFloat(eee = str, &sv))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstFloat(eee = str, &tw))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstFloat(eee = str, &th))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstInteger(eee = str, &framecount))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstInteger(eee = str, &framelength))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
			int cols = Round(1.0f/tw);
			if(cols<=0) cols = 1;
			for(i = 0; i<framecount; i++){
				m_FrameList.push_back(CTexAnimFrame(texturefilename, framelength));
				float a[4] = {su, sv, tw*(i%cols), th*(i/cols)};
				(m_FrameList.rbegin()->m_TexTrans
					= new CTextureTransformer)->SetUpMatrix(2, a);
			}
		}else if(tmp = Assignment(str, "RotationUVFrame")){
			str = tmp;
			float theta0, theta1, cu, cv;
			if(!(str = StringLiteral(eee = str, &texturefilename))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstFloat(eee = str, &theta0))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstFloat(eee = str, &theta1))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstFloat(eee = str, &cu))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstFloat(eee = str, &cv))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstInteger(eee = str, &framecount))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = ConstInteger(eee = str, &framelength))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
			for(i = 0; i<framecount; i++){
				m_FrameList.push_back(CTexAnimFrame(texturefilename, framelength));
				float a[3] = {theta0+theta1*i, cu, cv};
				(m_FrameList.rbegin()->m_TexTrans
					= new CTextureTransformer)->SetUpMatrix(3, a);
			}
		}else{
			break;
		}
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	データ読込
 */
void CTextureAnimation::LoadData(){
	int i, fn = m_FrameList.size();
	for(i = 0; i<fn; i++) m_FrameList[i].LoadData();
}

/*
 *	状態セット
 */
void CTextureAnimation::SetState(
	CTexAnimState *state	//	状態変数
){
	if(!state){
		state = &m_PreviewState;
		if(g_PreviewAnimation) m_PreviewState.Step();
	}
	if(!m_FrameList.size()) return;
	while(state->m_Count>=m_FrameList[state->m_Frame].m_FrameLength){
		state->m_Count -= m_FrameList[state->m_Frame].m_FrameLength;
		state->m_Frame++;
		if(state->m_Frame>=m_FrameList.size()) state->m_Frame = 0;
	}
	m_CurrentFrame = state->m_Frame;
}

/*
 *	アニメーション適用
 */
void CTextureAnimation::Apply(
	CMesh *mesh,	//	メッシュ
	int matid		//	マテリアル ID
){
	if(m_FrameList.size()) m_FrameList[m_CurrentFrame].Apply(mesh, matid);
	else mesh->SetCustomTexture(matid, NULL);
}
