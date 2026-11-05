#ifndef RAILMAP_H_INCLUDED
#define RAILMAP_H_INCLUDED

void InitRailMap();
void DumpMapLine(VEC2, D3DCOLOR, VEC2, D3DCOLOR, bool shadow = true);
void RailMapLine(VEC3, D3DCOLOR, VEC3, D3DCOLOR, bool shadow = true, bool bold = false);
void RailMapText(VEC3, char *, D3DCOLOR);
void RenderRailMap();

extern bool g_MapDrawNeeded;

#endif
