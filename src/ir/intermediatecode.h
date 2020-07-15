#ifndef _INTERMEDIATECODE_H
#define _INTERMEDIATECODE_H

#include <string>
#include "../front/symboltable.h"

using namespace std;

//中间代码类型SSA格式 v1
enum irCodeType {
	ADD, SUB, DIV, MUL, REM,  		//算术运算
	AND, OR, NOT,					//逻辑运算
	EQL, NEQ, SGT, SGE, SLT, SLE,	//关系运算
	ALLOC,							//栈空间申请
	STORE,							//存值到变量
	STOREARR,						//赋值数组
	LOAD,							//从变量取值
	LOADARR,						//从数组取值
	INDEX,							//数组的索引
	CALL,							//函数调用
	RET,							//函数返回
	PUSH,							//压栈
	POP,							//退栈
	LABEL,							//标签
	BR,								//直接跳转 + 条件跳转
	DEFINE,							//函数定义
	PARA,							//参数定义
	GLOBAL,							//全局声明：常量+变量
	NOTE,							//注释
	MOV,							//移动
	LEA,							//取地址
	GETREG,							//取返回值
};

class CodeItem
{
public:
	CodeItem(irCodeType type, string res, string ope1, string ope2);
	CodeItem(const CodeItem& instr);
	CodeItem& operator=(const CodeItem& instr);
	void setID(int id);
	int getId();
	irCodeType getCodetype();
	void setCodetype(irCodeType type);
	string getOperand1();
	string getOperand2();
	string getResult();
	void setOperand1(string ope1);
	void setOperand2(string ope2);
	void setResult(string res);
	void setInstr(string res, string ope1, string ope2);
	void setFatherBlock(vector<int> a);        //可以set的位置：所有变量、参数出现的地方
	void changeContent(string res, string ope1, string ope2);
	vector<int> getFatherBlock();
	string getContent();
private:
	int id;
	irCodeType codetype;     //中间代码种类
	string result;				//结果
	string operand1;			//左操作数
	string operand2;			//右操作数
	vector<int> fatherBlock;	//当前中间代码所在作用域
};

#endif 
