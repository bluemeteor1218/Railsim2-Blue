#define LANGUAGE_CPP

#include "stdafx.h"
#include "CPlugin.h"

//	内部定数
char *LANG_FILE_NAME = "Language.txt";

//	外部グローバル
extern char *g_DayOfWeek[];

//	内部グローバル1
string g_LanguageName;

namespace LanguageResource
{

/*
 *	言語初期化
 */
bool InitLanguage(){
	chdir(g_BaseDir);
	char *buf = LoadBinaryText(LANG_FILE_NAME), *str = buf, *eee;

	//	絶対必要なもの
	SyntaxError = "Syntax error";
	InvalidVersion = "Invalid version";
	UnsupportedVersion = "Unsupported version";
	InvalidDatafileType = "Invalid datafile type";

	try{
		if(!(str = Space(eee = str))) return false;
		if(!(str = BeginBlock(eee = str, "DatafileHeader"))) return false;
		float version;
		if(!(str = AsgnFloat(eee = str, "RailSimVersion", &version))) throw CSynErr(eee);
		if(version<2.02f) throw CSynErr(eee, "%s: %.2f", lang(InvalidVersion), version);
		if(RAILSIM_VERSION<version) throw CSynErr(eee, "%s: %.2f", lang(UnsupportedVersion), version);
		string datafiletype;
		if(!(str = AsgnIdentifier(eee = str, "DatafileType", &datafiletype))) throw CSynErr(eee);
		if(datafiletype!="Language") throw CSynErr(eee, lang(InvalidDatafileType));
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Language"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Name", &g_LanguageName))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Resource"))) throw CSynErr(eee);

		char **lang_id_list = new char *[1024];
		char **lang_id = lang_id_list;
#define PROC_LANG(a) *lang_id++ = #a;
#include "LanguageID.h"
		*lang_id = NULL;
		string **lang_str_list = new string *[1024];
		string **lang_str = lang_str_list;
#define PROC_LANG(a) *lang_str++ = &a;
#include "LanguageID.h"
		*lang_str = NULL;
		lang_id = lang_id_list;
		lang_str = lang_str_list;
		while(*lang_id){
			if(!(str = AsgnString(eee = str, *lang_id, *lang_str))) throw CSynErr(eee);
			lang_id++;
			lang_str++;
		}
		delete [] lang_id_list;
		delete [] lang_str_list;

		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	}
	catch(CSynErr err){
		err.Handle(FlashIn("%s <%s>", LANG_FILE_NAME, "Layout"), buf);
		return false;
	}
	DELETE_A(buf);

	g_DayOfWeek[0] = lang(Sun);
	g_DayOfWeek[1] = lang(Mon);
	g_DayOfWeek[2] = lang(Tue);
	g_DayOfWeek[3] = lang(Wed);
	g_DayOfWeek[4] = lang(Thu);
	g_DayOfWeek[5] = lang(Fri);
	g_DayOfWeek[6] = lang(Sat);
	
	return true;
}

}	//	namespace LanguageResource
