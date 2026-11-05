#include "stdafx.h"
#include "CCustomizerMover.h"
#include "CCustomizerMisc.h"
#include "CModelSwitch.h"

//	static メンバ
//CNamedObject *CCustomizerBase::ms_CurrentObject;	//	現在のオブジェクト

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CCustomizerContainer::CCustomizerContainer(){
	m_Customizer = NULL;
}

/*
 *	コピーコンストラクタ
 */
CCustomizerContainer::CCustomizerContainer(
	const CCustomizerContainer &src	//	コピー元
){
	m_Customizer = src.m_Customizer->Duplicate();
}

/*
 *	デストラクタ
 */
CCustomizerContainer::~CCustomizerContainer(){
	DELETE_V(m_Customizer);
}

/*
 *	読込
 */
char *CCustomizerContainer::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *tmp;
	DELETE_V(m_Customizer);
	CModelChanger modelchanger;
	CStaticRotator staticrotator;
	CStaticMover staticmover;
	CDynamicRotator dynamicrotator;
	CWindTracker windtracker;
	CWindmill windmill;
	CAnalogClock analogclock;
	CShadowInhibitor shadowinhibitor;
	CEnvMapper envmapper;
	CAlphaTester alphatester;
	CTextureChanger texturechanger;
	CTextureTransformer texturetransformer;
	CAnimationApplier animationapplier;
	CAlphaChanger alphachanger;
	CMaterialChanger materialchanger;
	CCustomizerSwitchApplier customizerswitchapplier;
	if(tmp = modelchanger.Read(str)){
		str = tmp;
		m_Customizer = new CModelChanger(modelchanger);
	}else if(tmp = staticrotator.Read(str, mpi)){
		str = tmp;
		m_Customizer = new CStaticRotator(staticrotator);
	}else if(tmp = staticmover.Read(str, mpi)){
		str = tmp;
		m_Customizer = new CStaticMover(staticmover);
	}else if(tmp = dynamicrotator.Read(str, mpi)){
		str = tmp;
		m_Customizer = new CDynamicRotator(dynamicrotator);
	}else if(tmp = windtracker.Read(str, mpi)){
		str = tmp;
		m_Customizer = new CWindTracker(windtracker);
	}else if(tmp = windmill.Read(str, mpi)){
		str = tmp;
		m_Customizer = new CWindmill(windmill);
	}else if(tmp = analogclock.Read(str)){
		str = tmp;
		m_Customizer = new CAnalogClock(analogclock);
	}else if(tmp = shadowinhibitor.Read(str)){
		str = tmp;
		m_Customizer = new CShadowInhibitor(shadowinhibitor);
	}else if(tmp = envmapper.Read(str)){
		str = tmp;
		m_Customizer = new CEnvMapper(envmapper);
	}else if(tmp = alphatester.Read(str)){
		str = tmp;
		m_Customizer = new CAlphaTester(alphatester);
	}else if(tmp = texturechanger.Read(str)){
		str = tmp;
		m_Customizer = new CTextureChanger(texturechanger);
	}else if(tmp = texturetransformer.Read(str, true)){
		str = tmp;
		m_Customizer = new CTextureTransformer(texturetransformer);
	}else if(tmp = animationapplier.Read(str, mpi)){
		str = tmp;
		m_Customizer = new CAnimationApplier(animationapplier);
	}else if(tmp = alphachanger.Read(str)){
		str = tmp;
		m_Customizer = new CAlphaChanger(alphachanger);
	}else if(tmp = materialchanger.Read(str)){
		str = tmp;
		m_Customizer = new CMaterialChanger(materialchanger);
	}else if(tmp = customizerswitchapplier.Read(str, mpi)){
		str = tmp;
		m_Customizer = new CCustomizerSwitchApplier(customizerswitchapplier);
	}else{
		str = NULL;
	}
	return str;
}
