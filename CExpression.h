#ifndef CEXPRESSION_H_INCLUDED
#define CEXPRESSION_H_INCLUDED

class CModelSwitch;
class CModelPlugin;

/*
 *	数式クラス
 */
class CExpression{
public:
	virtual ~CExpression(){}
	virtual CExpression *Duplicate() = 0;
	virtual int CalcInt() = 0;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	変数参照クラス
 */
class CVariableReference: public CExpression{
private:
	CModelSwitch *m_Link;	//	スイッチ
public:
	CVariableReference(CModelSwitch *sw){ m_Link = sw; }
	CExpression *Duplicate(){ return new CVariableReference(*this); }
	int CalcInt();
};

/*
 *	定数クラス
 */
class CConstInteger: public CExpression{
private:
	int m_Constant;	//	値
public:
	CConstInteger(int cons){ m_Constant = cons; }
	CExpression *Duplicate(){ return new CConstInteger(*this); }
	int CalcInt();
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	単項演算子
 */
class CMonomialOperator: public CExpression{
protected:
	CExpression *m_Child;	//	左辺
public:
	CMonomialOperator(){ m_Child = NULL; }
	CMonomialOperator(CExpression *child){ m_Child = child; }
	virtual ~CMonomialOperator();
	void CopyOperand(const CMonomialOperator *);
	virtual CExpression *Duplicate() = 0;
	virtual int CalcInt() = 0;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	二項演算子
 */
class CBinomialOperator: public CExpression{
protected:
	CExpression *m_Left;	//	左辺
	CExpression *m_Right;	//	右辺
public:
	CBinomialOperator(){ m_Left = m_Right = NULL; }
	CBinomialOperator(CExpression *left, CExpression *right){
		m_Left = left; m_Right = right;
	}
	virtual ~CBinomialOperator();
	void CopyOperand(const CBinomialOperator *);
	virtual CExpression *Duplicate() = 0;
	virtual int CalcInt() = 0;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	三項演算子
 */
class CTrinomialOperator: public CExpression{
protected:
	CExpression *m_Condition;	//	条件式
	CExpression *m_TrueExpr;	//	真式
	CExpression *m_FalseExpr;	//	偽式
public:
	CTrinomialOperator(){ m_Condition = m_TrueExpr = m_FalseExpr = NULL; }
	CTrinomialOperator(CExpression *cond, CExpression *te, CExpression *fe){
		m_Condition = cond; m_TrueExpr = te; m_FalseExpr = fe;
	}
	virtual ~CTrinomialOperator();
	void CopyOperand(const CTrinomialOperator *);
	virtual CExpression *Duplicate(){ return new CTrinomialOperator(*this); }
	virtual int CalcInt();
};

//	関数宣言
char *Expression(char *, CExpression **);

//	外部グローバル
extern CModelPlugin *g_SwitchOwner;

#endif
