#include "stdafx.h"
#include "CExpression.h"
#include "CModelPlugin.h"

//	内部グローバル
CModelPlugin *g_SwitchOwner = NULL;	//	スイッチ所有者

/*
 *	式評価
 */
int CVariableReference::CalcInt(){
	return m_Link->GetValue();
}

/*
 *	式評価
 */
int CConstInteger::CalcInt(){
	return m_Constant;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	デストラクタ
 */
CMonomialOperator::~CMonomialOperator(){
	DELETE_V(m_Child);
}

/*
 *	コピーコンストラクタ
 */
void CMonomialOperator::CopyOperand(
	const CMonomialOperator *src	//	コピー元
){
	m_Child = src->m_Child->Duplicate();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	クラス定義マクロ
#define CMONOMIALOPERATOR_DEFINECLASS(type) \
	class type: public CMonomialOperator{ \
	public: \
		type(CExpression *child): CMonomialOperator(child){} \
		type(const type &src){ CopyOperand(&src); } \
		CExpression *Duplicate(){ return new type(*this); } \
		int CalcInt(); \
	};

CMONOMIALOPERATOR_DEFINECLASS(COpr1Plus);
CMONOMIALOPERATOR_DEFINECLASS(COpr1Minus);
CMONOMIALOPERATOR_DEFINECLASS(COpr1Not);
CMONOMIALOPERATOR_DEFINECLASS(COpr1Negative);

/*
 *	式評価
 */
int COpr1Plus::CalcInt(){
	return m_Child->CalcInt();
}
int COpr1Minus::CalcInt(){
	return -m_Child->CalcInt();
}
int COpr1Not::CalcInt(){
	return !m_Child->CalcInt();
}
int COpr1Negative::CalcInt(){
	return ~m_Child->CalcInt();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	デストラクタ
 */
CBinomialOperator::~CBinomialOperator(){
	DELETE_V(m_Left);
	DELETE_V(m_Right);
}

/*
 *	コピーコンストラクタ
 */
void CBinomialOperator::CopyOperand(
	const CBinomialOperator *src	//	コピー元
){
	m_Left = src->m_Left->Duplicate();
	m_Right = src->m_Right->Duplicate();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	クラス定義マクロ
#define CBINOMIALOPERATOR_DEFINECLASS(type) \
	class type: public CBinomialOperator{ \
	public: \
		type(CExpression *left, CExpression *right): \
			CBinomialOperator(left, right){} \
		type(const type &src){ CopyOperand(&src); } \
		CExpression *Duplicate(){ return new type(*this); } \
		int CalcInt(); \
	};

CBINOMIALOPERATOR_DEFINECLASS(COpr2Multiply);
CBINOMIALOPERATOR_DEFINECLASS(COpr2Divide);
CBINOMIALOPERATOR_DEFINECLASS(COpr2Surplus);
CBINOMIALOPERATOR_DEFINECLASS(COpr2Plus);
CBINOMIALOPERATOR_DEFINECLASS(COpr2Minus);
CBINOMIALOPERATOR_DEFINECLASS(COpr2ShiftLeft);
CBINOMIALOPERATOR_DEFINECLASS(COpr2ShiftRight);
CBINOMIALOPERATOR_DEFINECLASS(CCmp2Less);
CBINOMIALOPERATOR_DEFINECLASS(CCmp2Greater);
CBINOMIALOPERATOR_DEFINECLASS(CCmp2LessEqual);
CBINOMIALOPERATOR_DEFINECLASS(CCmp2GreaterEqual);
CBINOMIALOPERATOR_DEFINECLASS(CCmp2Equal);
CBINOMIALOPERATOR_DEFINECLASS(CCmp2NotEqual);
CBINOMIALOPERATOR_DEFINECLASS(COpr2And);
CBINOMIALOPERATOR_DEFINECLASS(COpr2Xor);
CBINOMIALOPERATOR_DEFINECLASS(COpr2Or);
CBINOMIALOPERATOR_DEFINECLASS(COpr2LogicalAnd);
CBINOMIALOPERATOR_DEFINECLASS(COpr2LogicalOr);

/*
 *	式評価
 */
int COpr2Multiply::CalcInt(){
	return m_Left->CalcInt()*m_Right->CalcInt();
}
int COpr2Divide::CalcInt(){
	int rhs = m_Right->CalcInt();
	return rhs ? m_Left->CalcInt()/rhs : 0;
}
int COpr2Surplus::CalcInt(){
	int rhs = m_Right->CalcInt();
	return rhs ? m_Left->CalcInt()%rhs : 0;
}
int COpr2Plus::CalcInt(){
	return m_Left->CalcInt()+m_Right->CalcInt();
}
int COpr2Minus::CalcInt(){
	return m_Left->CalcInt()-m_Right->CalcInt();
}
int COpr2ShiftLeft::CalcInt(){
	return m_Left->CalcInt()<<m_Right->CalcInt();
}
int COpr2ShiftRight::CalcInt(){
	return m_Left->CalcInt()>>m_Right->CalcInt();
}
int CCmp2Less::CalcInt(){
	return m_Left->CalcInt()<m_Right->CalcInt();
}
int CCmp2Greater::CalcInt(){
	return m_Left->CalcInt()>m_Right->CalcInt();
}
int CCmp2LessEqual::CalcInt(){
	return m_Left->CalcInt()<=m_Right->CalcInt();
}
int CCmp2GreaterEqual::CalcInt(){
	return m_Left->CalcInt()>=m_Right->CalcInt();
}
int CCmp2Equal::CalcInt(){
	return m_Left->CalcInt()==m_Right->CalcInt();
}
int CCmp2NotEqual::CalcInt(){
	return m_Left->CalcInt()!=m_Right->CalcInt();
}
int COpr2And::CalcInt(){
	return m_Left->CalcInt()&m_Right->CalcInt();
}
int COpr2Or::CalcInt(){
	return m_Left->CalcInt()|m_Right->CalcInt();
}
int COpr2Xor::CalcInt(){
	return m_Left->CalcInt()^m_Right->CalcInt();
}
int COpr2LogicalAnd::CalcInt(){
	return m_Left->CalcInt() && m_Right->CalcInt();
}
int COpr2LogicalOr::CalcInt(){
	return m_Left->CalcInt() || m_Right->CalcInt();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	デストラクタ
 */
CTrinomialOperator::~CTrinomialOperator(){
	DELETE_V(m_Condition);
	DELETE_V(m_TrueExpr);
	DELETE_V(m_FalseExpr);
}

/*
 *	コピーコンストラクタ
 */
void CTrinomialOperator::CopyOperand(
	const CTrinomialOperator *src	//	コピー元
){
	m_Condition = src->m_Condition->Duplicate();
	m_TrueExpr = src->m_TrueExpr->Duplicate();
	m_FalseExpr = src->m_FalseExpr->Duplicate();
}

/*
 *	式評価
 */
int CTrinomialOperator::CalcInt(){
	return m_Condition->CalcInt() ? m_TrueExpr->CalcInt() : m_FalseExpr->CalcInt();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	数式
 */
char *VariableReference(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee;
	string sw;
	if(!(str = StringLiteral(eee = str, &sw))) return NULL;
	CModelSwitch *link;
	int i;
	for(i = 0; i<SYSTEM_SWITCH_NUM; i++)
		if(link = g_SystemSwitch[i].Check(sw)) break;
	if(!link && !(link = g_SwitchOwner->FindModelSwitch(sw)))
		throw CSynErr(eee, "%s: \"%s\"", lang(UndefinedSwitch), sw.c_str());
	*ret = new CVariableReference(link);
	return str;
}

/*
 *	数式
 */
char *ConstantExpression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	int val;
	if(!(str = ConstInteger(str, &val))) return NULL;
	*ret = new CConstInteger(val);
	return str;
}

/*
 *	数式
 */
char *PrimaryExpression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee, *tmp;
	if(tmp = VariableReference(str, ret)){
		str = tmp;
	}else if(tmp = ConstantExpression(str, ret)){
		str = tmp;
	}else if(tmp = Character2(eee = str, '(')){
		if(!(str = Expression(str = tmp, ret))) throw CSynErr(eee);
		if(!(str = Character2(eee = str, ')'))) throw CSynErr(eee);
	}else{
		return NULL;
	}
	return str;
}

/*
 *	数式
 */
char *UnaryExpression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee, *tmp;
	if(tmp = Character2(eee = str, '+')){
		if(!(str = UnaryExpression(str = tmp, ret))) throw CSynErr(eee);
		*ret = new COpr1Plus(*ret);
	}else if(tmp = Character2(eee = str, '-')){
		if(!(str = UnaryExpression(str = tmp, ret))) throw CSynErr(eee);
		*ret = new COpr1Minus(*ret);
	}else if(tmp = Character2(eee = str, '!')){
		if(!(str = UnaryExpression(str = tmp, ret))) throw CSynErr(eee);
		*ret = new COpr1Not(*ret);
	}else if(tmp = Character2(eee = str, '~')){
		if(!(str = UnaryExpression(str = tmp, ret))) throw CSynErr(eee);
		*ret = new COpr1Negative(*ret);
	}else{
		return PrimaryExpression(str, ret);
	}
	return str;
}

/*
 *	数式
 */
char *MultiplicativeExpression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee, *tmp;
	if(!(str = UnaryExpression(str, ret))) return NULL;
	while(true){
		CExpression *expr;
		if(tmp = Character2(eee = str, '*')){
			if(!(str = UnaryExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new COpr2Multiply(*ret, expr);
		}else if(tmp = Character2(eee = str, '/')){
			if(!(str = UnaryExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new COpr2Divide(*ret, expr);
		}else if(tmp = Character2(eee = str, '%')){
			if(!(str = UnaryExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new COpr2Surplus(*ret, expr);
		}else{
			break;
		}
	}
	return str;
}

/*
 *	数式
 */
char *AdditiveExpression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee, *tmp;
	if(!(str = MultiplicativeExpression(str, ret))) return NULL;
	while(true){
		CExpression *expr;
		if(tmp = Character2(eee = str, '+')){
			if(!(str = MultiplicativeExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new COpr2Plus(*ret, expr);
		}else if(tmp = Character2(eee = str, '-')){
			if(!(str = MultiplicativeExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new COpr2Minus(*ret, expr);
		}else{
			break;
		}
	}
	return str;
}

/*
 *	数式
 */
char *ShiftExpression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee, *tmp;
	if(!(str = AdditiveExpression(str, ret))) return NULL;
	while(true){
		CExpression *expr;
		if(tmp = String2(eee = str, "<<")){
			if(!(str = AdditiveExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new COpr2ShiftLeft(*ret, expr);
		}else if(tmp = String2(eee = str, ">>")){
			if(!(str = AdditiveExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new COpr2ShiftRight(*ret, expr);
		}else{
			break;
		}
	}
	return str;
}

/*
 *	数式
 */
char *RelationalExpression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee, *tmp;
	if(!(str = ShiftExpression(str, ret))) return NULL;
	while(true){
		CExpression *expr;
		if(tmp = String2(eee = str, "<=")){
			if(!(str = ShiftExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new CCmp2LessEqual(*ret, expr);
		}else if(tmp = String2(eee = str, ">=")){
			if(!(str = ShiftExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new CCmp2GreaterEqual(*ret, expr);
		}else if(tmp = Character2(eee = str, '<')){
			if(!(str = ShiftExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new CCmp2Less(*ret, expr);
		}else if(tmp = Character2(eee = str, '>')){
			if(!(str = ShiftExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new CCmp2Greater(*ret, expr);
		}else{
			break;
		}
	}
	return str;
}

/*
 *	数式
 */
char *EqualityExpression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee, *tmp;
	if(!(str = RelationalExpression(str, ret))) return NULL;
	while(true){
		CExpression *expr;
		if(tmp = String2(eee = str, "==")){
			if(!(str = RelationalExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new CCmp2Equal(*ret, expr);
		}else if(tmp = String2(eee = str, "!=")){
			if(!(str = RelationalExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new CCmp2NotEqual(*ret, expr);
		}else{
			break;
		}
	}
	return str;
}

/*
 *	数式
 */
char *AndExpression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee, *tmp;
	if(!(str = EqualityExpression(str, ret))) return NULL;
	while(true){
		CExpression *expr;
		if(!String2(str, "&&") && (tmp = Character2(eee = str, '&'))){
			if(!(str = EqualityExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new COpr2And(*ret, expr);
		}else{
			break;
		}
	}
	return str;
}

/*
 *	数式
 */
char *XorExpression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee, *tmp;
	if(!(str = AndExpression(str, ret))) return NULL;
	while(true){
		CExpression *expr;
		if(tmp = Character2(eee = str, '^')){
			if(!(str = AndExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new COpr2Xor(*ret, expr);
		}else{
			break;
		}
	}
	return str;
}

/*
 *	数式
 */
char *OrExpression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee, *tmp;
	if(!(str = XorExpression(str, ret))) return NULL;
	while(true){
		CExpression *expr;
		if(!String2(str, "||") && (tmp = Character2(eee = str, '|'))){
			if(!(str = XorExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new COpr2Or(*ret, expr);
		}else{
			break;
		}
	}
	return str;
}

/*
 *	数式
 */
char *LogicalAndExpression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee, *tmp;
	if(!(str = OrExpression(str, ret))) return NULL;
	while(true){
		CExpression *expr;
		if(tmp = String2(eee = str, "&&")){
			if(!(str = OrExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new COpr2LogicalAnd(*ret, expr);
		}else{
			break;
		}
	}
	return str;
}

/*
 *	数式
 */
char *LogicalOrExpression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee, *tmp;
	if(!(str = LogicalAndExpression(str, ret))) return NULL;
	while(true){
		CExpression *expr;
		if(tmp = String2(eee = str, "||")){
			if(!(str = LogicalAndExpression(str = tmp, &expr))) throw CSynErr(eee);
			*ret = new COpr2LogicalOr(*ret, expr);
		}else{
			break;
		}
	}
	return str;
}

/*
 *	数式
 */
char *Expression(
	char *str,			//	対象文字列
	CExpression **ret	//	格納先
){
	char *eee, *tmp;
	if(!(str = LogicalOrExpression(str, ret))) return NULL;
	if(tmp = Character2(eee = str, '?')){
		CExpression *te, *fe;
		if(!(str = Expression(str = tmp, &te))) throw CSynErr(eee);
		if(!(str = Character2(str, ':'))) throw CSynErr(eee);
		if(!(str = Expression(eee = str, &fe))) throw CSynErr(eee);
		*ret = new CTrinomialOperator(*ret, te, fe);
	}
	return str;
}
