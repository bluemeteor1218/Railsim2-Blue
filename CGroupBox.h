#ifndef CGROUPBOX_H_INCLUDED
#define CGROUPBOX_H_INCLUDED

#include "CInterface.h"

/*
 *	グループボックス
 */
class CGroupBox: public CInterface{
private:
public:
	bool ScanInput();
	void Render();
};

#endif
