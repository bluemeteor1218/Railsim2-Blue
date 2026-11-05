#include "stdafx.h"
#include "HighTimer.h"
#include "CJobTimer.h"

CJobTimer g_JobTimer;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CTimerObj::CTimerObj(const char* name)
	: m_FrameCount(-1)
	, m_IsStarted(false)
	, m_InactiveFrames(0)
	, m_OddFrame(0)
{
	SetName(name);
}

void CTimerObj::InitFrame(){
	CFrameData &frame_data = m_FrameData[m_OddFrame];
	if(frame_data.m_Data.size()){
		frame_data.m_Data.clear();
		m_InactiveFrames = 0;
	}else{
		++m_InactiveFrames;
	}
	frame_data.m_Total = 0;
}

void CTimerObj::Start(){
	if(m_FrameCount!=g_JobTimer.GetFrameCount()){
		m_FrameCount = g_JobTimer.GetFrameCount();
		InitFrame();
		g_JobTimer.AddObj(this);
	}
	CFrameData &frame_data = m_FrameData[m_OddFrame];
	if(m_IsStarted) ErrorDialog("CTimerObj::Start() [%s] Already started!", GetName());
	m_IsStarted = true;
	frame_data.m_Data.push_back(CTimerData());
	CTimerData &data = frame_data.m_Data.back();
	data.m_Start = data.m_End = HighTimer();
}

void CTimerObj::Stop(){
	CFrameData &frame_data = m_FrameData[m_OddFrame];
	if(!m_IsStarted) ErrorDialog("CTimerObj::Stop() [%s] Not started!", GetName());
	CTimerData &data = frame_data.m_Data.back();
	data.m_End = HighTimer();
	frame_data.m_Total += data.GetDelta();
	m_IsStarted = false;
}

void CTimerObj::Update(){
	if(m_IsStarted) ErrorDialog("CTimerObj::Update() [%s] Already started!", GetName());
	m_OddFrame = !m_OddFrame;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CJobTimer::CJobTimer()
	: m_OddFrame(0)
	, m_FrameCount(0)
	, m_SpanFrames(1)
	, m_SpanFrameCount(0)
{
}

void CJobTimer::Clear(){
	CFrameData &frame_data = m_FrameData[m_OddFrame];
	frame_data.m_ObjList.clear();
}

void CJobTimer::AddObj(
	CTimerObj *obj	//	object
){
	CFrameData &frame_data = m_FrameData[m_OddFrame];
	frame_data.m_ObjList.push_back(obj);
}

void CJobTimer::Start(){
#if ENABLE_JOB_TIMER
	++m_FrameCount;
	Clear();
	CFrameData &frame_data = m_FrameData[m_OddFrame];
	frame_data.m_Start = frame_data.m_End = HighTimer();
#endif
}

void CJobTimer::Stop(){
#if ENABLE_JOB_TIMER
	CFrameData &frame_data = m_FrameData[m_OddFrame];
	frame_data.m_End = HighTimer();
	unsigned int i;
	for(i = 0; i<frame_data.m_ObjList.size(); ++i){
		frame_data.m_ObjList[i]->Update();
	}
	m_OddFrame = !m_OddFrame;
#endif
}

void CJobTimer::DrawResult(){
#if ENABLE_JOB_TIMER
	TIMER_RAII("CJobTimer::DrawResult()");
	CFrameData &frame_data = m_FrameData[!m_OddFrame];
	unsigned int i, j;
	static const double FRAME_UNIT = 1000.0/MAXFPS; // milliseconds
	const double delta = FromHighTimerCountToMs(frame_data.m_End-frame_data.m_Start);
	const int span_frames = (int)ceil(delta/FRAME_UNIT);
	if(span_frames<m_SpanFrames){
		++m_SpanFrameCount;
		if(m_SpanFrameCount>MAXFPS) m_SpanFrames = span_frames;
	}else{
		m_SpanFrames = span_frames;
		m_SpanFrameCount = 0;
	}
	const double rcp_span = 1.0/(FRAME_UNIT*m_SpanFrames);
	const int bar_height = FONT_HEIGHT;
	const int bar_hilite = 4;
	const int area_x = 50;
	const int area_y = g_DispHeight-50;
	const int area_width = g_DispWidth-200;
	const int area_height = bar_height*frame_data.m_ObjList.size();
	const int number_width = 50;
	int pos_y = area_y;
	devResetMatrix();
	devSetLighting(false);
	devSetTexture(0, 0);
	devBLEND_ALPHA();
	Fill2DRect(area_x, area_y-area_height, area_x+area_width, area_y, 0x80000000);
	for(i = 0; i<frame_data.m_ObjList.size(); ++i){
		CTimerObj* obj = frame_data.m_ObjList[i];
		const CTimerObj::CFrameData &obj_frame = obj->GetFrameData(1);
		for(j = 0; j<obj_frame.m_Data.size(); ++j){
			const CTimerObj::CTimerData &timer_data = obj_frame.m_Data[j];
			const int bar_start = (int)(FromHighTimerCountToMs(timer_data.m_Start-frame_data.m_Start)*area_width*rcp_span);
			int bar_end = (int)(FromHighTimerCountToMs(timer_data.m_End-frame_data.m_Start)*area_width*rcp_span);
			if(bar_end==bar_start) bar_end=bar_start+1;
			Fill2DRect(area_x+bar_start, pos_y-bar_height, area_x+bar_end, pos_y, MultiplyColor(0xc0ff0000, j&1 ? 0xffc0c0c0 : 0xffffffff));
			Fill2DRect(area_x+bar_start, pos_y-bar_hilite, area_x+bar_end, pos_y, j&1 ? 0x80c0c0c0 : 0x80ffffff);
		}
		pos_y -= bar_height;
	}
	Draw2DRect(area_x, area_y-area_height, area_x+area_width, area_y, 0xffffffff);
	for(i = 1; i<m_SpanFrames; ++i){
		const int line_x = area_width*i/m_SpanFrames;
		Draw2DLine(area_x+line_x, area_y-area_height, area_x+line_x, area_y, 0xffffffff);
	}
	pos_y = area_y;
	const int str_base_x = area_x+number_width;
	for(i = 0; i<frame_data.m_ObjList.size(); ++i){
		CTimerObj* obj = frame_data.m_ObjList[i];
		const CTimerObj::CFrameData &obj_frame = obj->GetFrameData(1);
		const double frame_percent = FromHighTimerCountToMs(obj_frame.m_Total)*100.0/FRAME_UNIT;
		const int str_base_y = pos_y-bar_height;
		g_StrTex->RenderRight(str_base_x, str_base_y, 0xffffffff, 0, FlashIn("%.0lf x%d", frame_percent, obj_frame.m_Data.size()));
		g_StrTex->RenderLeft(str_base_x, str_base_y, 0xffffffff, 0, FlashIn(": %s", obj->GetName()));
		pos_y -= bar_height;
	}
#endif
}
