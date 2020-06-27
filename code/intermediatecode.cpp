#include "intermediatecode.h"
#include <string>
#include <iostream>
using namespace std;
string setContent(irCodeType type, string res, string ope1, string ope2)
{
	string content = "";
	switch (type) {
	case ADD: {
		content = "add " + res + " " + ope1 + " " + ope2;
		break;
	}
	case SUB: {
		content = "sub " + res + " " + ope1 + " " + ope2;
		break;
	}
	case DIV: {
		content = "div " + res + " " + ope1 + " " + ope2;
		break;
	}
	case MUL: {
		content = "mul " + res + " " + ope1 + " " + ope2;
		break;
	}
	case REM: {
		content = "rem " + res + " " + ope1 + " " + ope2;
		break;
	}
	case AND: {
		content = "and " + res + " " + ope1 + " " + ope2;
		break;
	}
	case OR: {
		content = "or " + res + " " + ope1 + " " + ope2;
		break;
	}
	case NOT: {
		content = "not " + res + " " + ope1;
		break;
	}
	case EQL: {
		content = "eql " + res + " " + ope1 + " " + ope2;
		break;
	}
	case NEQ: {
		content = "neq " + res + " " + ope1 + " " + ope2;
		break;
	}
	case SGT: {
		content = "sgt " + res + " " + ope1 + " " + ope2;
		break;
	}
	case SGE: {
		content = "sge " + res + " " + ope1 + " " + ope2;
		break;
	}
	case SLT: {
		content = "slt " + res + " " + ope1 + " " + ope2;
		break;
	}
	case SLE: {
		content = "sle " + res + " " + ope1 + " " + ope2;
		break;
	}
	case ALLOC: {
		if (ope1.size() == 0) {
			ope1 = "_";
		}
		content = "alloc " + res + " " + ope1 + " " + ope2;
		break;
	}
	case STORE: {
		content = "store " + res + " " + ope1 + " " + ope2;
		break;
	}
	case LOAD: {
		content = "load " + res + " " + ope1 + " " + ope2;
		break;
	}
	case INDEX: {
		break;
	}
	case CALL: {
		content = "call " + res + " " + ope1 + " " + ope2;
		break;
	}
	case RET: {
		content = "ret " + res;
		break;
	}
	case PUSH: {
		content = "push " + res + " " + ope2;
		break;
	}
	case POP: {
		break;
	}
	case LABEL: {
		content = "label " + res;
		break;
	}
	case BR: {
		if (ope1.size() > 0) {
			content = "br " + res + " " + ope1 + " " + ope2;
		}
		else {
			content = "br " + res;
		}
		break;
	}
	case DEFINE: {
		content = "define " + res + " " + ope1;
		break;
	}
	case PARA: {
		content = "para " + res + " " + ope1 + " " + ope2;
		break;
	}
	case GLOBAL: {
		if (ope1.size() == 0) {
			ope1 = "_";
		}
		content = "global " + res + " " + ope1 + " " + ope2;
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