#ifndef _INTERMEDIATECODE_H
#define _INTERMEDIATECODE_H

#include <string>
#include <vector>

using namespace std;

//中间代码格式SSA v1
enum irCodeType { 
	ADD, SUB, DIV, MUL, REM,
	AND, OR, NOT,
	EQL, NEQ, SGT, SGE, SLT, SLE,
	ALLOC,
	STORE,
	LOAD,
	INDEX,
	CALL,
	RET,
	PUSH,
	POP,
	LABEL,
	BR,
	DEFINE,
	PARA,
	GLOBAL
};

class CodeItem
{
public:
	CodeItem(irCodeType type, string res, string ope1, string ope2);
	irCodeType getCodetype();
	string getOperand1();
	string getOperand2();
	string getResult();
	void setOperand1(string ope1);
	void setOperand2(string ope2);
	void setResult(string res);
	void setFatherBlock(vector<int> a);
	vector<int> getFatherBlock();
	string getContent();
private:
	irCodeType codetype;
	string result;
	string operand1;
	string operand2;
	vector<int> fatherBlock;
	string content;
};

#endif 
