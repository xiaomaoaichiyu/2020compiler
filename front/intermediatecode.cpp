#include "intermediatecode.h"
#include <string>
#include <iostream>
using namespace std;
string standardLength(string a)
{
	while (a.size() < 10) {
		a = a + " ";
	}
	return a;
}
string setContent(irCodeType type, string res, string ope1, string ope2)
{
	string content = "";
	switch (type) {
	case ADD: {
		content = "add        " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case SUB: {
		content = "sub        " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case DIV: {
		content = "div        " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case MUL: {
		content = "mul        " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case REM: {
		content = "rem        " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case AND: {
		content = "and        " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case OR: {
		content = "or         " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case NOT: {
		content = "not        " + standardLength(res) + " " + standardLength(ope1);
		break;
	}
	case EQL: {
		content = "eql        " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case NEQ: {
		content = "neq        " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case SGT: {
		content = "sgt        " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case SGE: {
		content = "sge        " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case SLT: {
		content = "slt        " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case SLE: {
		content = "sle        " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case ALLOC: {
		if (ope1.size() == 0) {
			ope1 = "_";
		}
		content = "alloc      " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case STORE: {
		content = "store      " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case LOAD: {
		content = "load       " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case INDEX: {
		break;
	}
	case CALL: {
		content = "call       " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case RET: {
		content = "ret        " + standardLength(res);
		break;
	}
	case PUSH: {
		content = "push       " + standardLength(res) + "           " + standardLength(ope2);
		break;
	}
	case POP: {
		break;
	}
	case LABEL: {
		content = "label      " + standardLength(res);
		break;
	}
	case BR: {
		if (ope1.size() > 0) {
			content = "br         " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		}
		else {
			content = "br         " + standardLength(res);
		}
		break;
	}
	case DEFINE: {
		content = "define     " + standardLength(res) + " " + standardLength(ope1);
		break;
	}
	case PARA: {
		content = "para       " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	case GLOBAL: {
		if (ope1.size() == 0) {
			ope1 = "_";
		}
		content = "global     " + standardLength(res) + " " + standardLength(ope1) + " " + standardLength(ope2);
		break;
	}
	default:
		break;
	}
	return content;
}
CodeItem::CodeItem(irCodeType type, string res, string ope1, string ope2) {
	this->codetype = type;
	this->result = res;
	this->operand1 = ope1;
	this->operand2 = ope2;
	this->content = setContent(type, res, ope1, ope2);
}

irCodeType CodeItem::getCodetype()
{
	return this->codetype;
}

string CodeItem::getOperand1() {
	return this->operand1;
}

string CodeItem::getOperand2() {
	return this->operand2;
}

string CodeItem::getResult() {
	return this->result;
}

void CodeItem::setOperand1(string ope1) {
	this->operand1 = ope1;
}

void CodeItem::setOperand2(string ope2) {
	this->operand2 = ope2;
}

void CodeItem::setResult(string res) {
	this->result = res;
}

void CodeItem::setFatherBlock(vector<int> a)
{
	this->fatherBlock = a;
}
vector<int> CodeItem::getFatherBlock()
{
	return this->fatherBlock;
}

string CodeItem::getContent()
{
	return this->content;
}

#define ENUM_TO_STRING(enumName) (#enumName)


void CodeItem::changeContent(string res, string ope1, string ope2) 
{
	this->result = res;
	this->operand1 = ope1;
	this->operand2 = ope2;
	this->content = setContent(this->codetype, res, ope1, ope2);
}