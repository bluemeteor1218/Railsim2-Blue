#pragma warning (disable: 4786)

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <algorithm>
#include "lib\udx.h"

typedef list<string>::iterator Istring;

using namespace std;

#include "Const.h"
#include "Macro.h"
#include "Language.h"
#include "ValueArea.h"
#include "SystemCover.h"
#include "GraphicCover.h"
#include "Script.h"
#include "CWaveArray.h"
#include "CVertexDump.h"
#include "CStringTexture.h"

extern char g_BaseDir[];
extern int g_DispWidth;
extern int g_DispHeight;
extern int g_RSPV;
extern CTexList g_TexList;
extern CMeshList g_MeshList;
extern bool g_NetworkInitialized;
extern bool g_ManualControl;