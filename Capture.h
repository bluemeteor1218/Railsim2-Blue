#ifndef CAPTURE_H_INCLUDED
#define CAPTURE_H_INCLUDED

class CSceneryMode;

//	関数宣言
void InitCapture();
void ReleaseCaptureRS();
void VideoCapture(int, CSceneryMode *);
void CountPicture();
void CountVideoBMP();
void CountVideoAVI();
void StartVideoCapture();
void StopVideoCapture();

//	外部グローバル
extern int g_VideoState;
extern int g_VideoFrame;
extern int g_PictureCount;

#endif
