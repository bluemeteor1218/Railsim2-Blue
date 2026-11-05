#ifndef LANGUAGE_H_INCLUDED
#define LANGUAGE_H_INCLUDED

#ifdef LANGUAGE_CPP
#define LANG_EXT
#else
#define LANG_EXT extern
#endif

namespace LanguageResource{
#define PROC_LANG(a) LANG_EXT string a;
#include "LanguageID.h"
}

extern string g_LanguageName;

#define lang(name) ((char *)LanguageResource::name.c_str())

#endif
