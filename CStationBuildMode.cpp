#include "stdafx.h"
#include "CStation.h"
#include "CStationPlugin.h"
#include "CStationBuildMode.h"

/*
 *	モードを有効化
 */
void CStationBuildMode::EnterStructBuild(){
	ms_ModeLabel = lang(BuildStation);
	if(g_Station) g_Station->SetPreview();
}

/*
 *	建設
 */
void CStationBuildMode::Build(){
	if(g_Station){
		VEC3 tup, tdir;
		GetBuildDir(&tdir, &tup);
		if(g_Station) g_Station->SetTempSwitch();
		new CStation(g_Station, m_SnapPos, tdir, tup);
	}
}

/*
 *	レンダリング
 */
void CStationBuildMode::RenderStructBuild(){
	VEC3 tdir, tup;
	GetBuildDir(&tdir, &tup);
	if(g_Station) g_Station->SetTempSwitch();
	CStationPlugin::RenderPreview(m_SnapPos, tdir, tup);
}
