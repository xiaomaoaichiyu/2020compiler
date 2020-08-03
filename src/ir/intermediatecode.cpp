#include "intermediatecode.h"
#include <string>
#include <iostream>
using namespace std;
string standardLength(string a)
{
	while (a.size() < 15) {
		a = a + " ";
	}
	return a;
}
string CodeItem::getContent()
{
	string content = "";
	switch (this->codetype) {
	case ADD: {
		content = "add        " + standardLength(result) + " " 
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case SUB: {
		content = "sub        " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case DIV: {
		content = "div        " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case MUL: {
		content = "mul        " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case REM: {
		content = "rem        " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case AND: {
		content = "and        " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case OR: {
		content = "or         " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case NOT: {
		content = "not        " + standardLength(result) + " "
			+ standardLength(operand1) + standardLength(invariant);
		break;
	}
	case EQL: {
		content = "eql        " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case NEQ: {
		content = "neq        " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case SGT: {
		content = "sgt        " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case SGE: {
		content = "sge        " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case SLT: {
		content = "slt        " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case SLE: {
		content = "sle        " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case ALLOC: {
		string ope1;
		if (operand1.size() == 0) {
			ope1 = "_";
		}
		else {
			ope1 = operand1;
		}
		content = "alloc      " + standardLength(result) + " "
			+ standardLength(ope1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case STORE: {
		content = "store      " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case STOREARR: {
		content = "storearr   " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case LOAD: {
		content = "load       " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case LOADARR: {
		content = "loadarr    " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case INDEX: {
		break;
	}
	case CALL: {
		content = "call       " + standardLength(result) + " "
			+ standardLength(operand1) + " " + standardLength(operand2);
		break;
	}
	case RET: {
		content = "ret                   " + standardLength(operand1)
			+ " " + standardLength(operand2) + standardLength(invariant);
		break;
	}
	case PUSH: {
		if (result == "int*") {
			string a = "0";
			if (inlineMatrixTag == 1) {
				a = "1";
			}
			content = "push       " + standardLength(result) + " " + standardLength(operand1) + " " + standardLength(operand2) + a;
		}
		else {
			content = "push       " + standardLength(result) + " " + standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		}
		break;
	}
	case POP: {
		break;
	}
	case LABEL: {
		content = "label      " + standardLength(result);
		break;
	}
	case BR: {
		if (operand1.size() > 0) {
			content = "br         " + standardLength(result) + " " + standardLength(operand1) + " " + standardLength(operand2) + standardLength(invariant);
		}
		else {
			content = "br                    " + standardLength(operand1) + standardLength(invariant);
		}
		break;
	}
	case DEFINE: {
		content = "define     " + standardLength(result) + " " + standardLength(operand1);
		break;
	}
	case PARA: {
		content = "para       " + standardLength(result) + " " + standardLength(operand1) + " " + standardLength(operand2);
		break;
	}
	case GLOBAL: {
		string ope1;
		if (operand1.size() == 0) {
			ope1 = "_";
		}
		else {
			ope1 = operand1;
		}
		content = "global     " + standardLength(result) + " " + standardLength(ope1) + " " + standardLength(operand2);
		break;
	}
	case MOV: {
		content = "mov                   " + standardLength(operand1) + " " + standardLength(operand2);
		break;
	}
	case NOTE: {
		if (operand1 == "func") {
			content = "note       " + standardLength(result) + " " + standardLength(operand1) + " " + standardLength(operand2) + "----------------";
		}
		else if (operand1 == "array") {
			content = "note       " + standardLength(result) + " " + standardLength(operand1) + " " + standardLength(operand2) + "--------";
		}
		else if (operand1 == "inline") {
			content = "note       " + standardLength(result) + "inline     " + standardLength(operand2);
		}
		else if (operand1 == "note"){
			content = "\t\t\t\t\t\t\t\t"+standardLength(result);
		}
		else {
			content = "";
		}
		break;
	}case LEA: {
		content = "LEA                   " + standardLength(operand1) + " " + standardLength(operand2);
		break;
	}
	case GETREG: {
		content = "getReg                " + standardLength(operand1);
		break;
	}
	case PHI: {
		content = "Phi        " + standardLength(result);
		break;
	}
	case ARRAYINIT: {
		content = "arrayinit  " + standardLength(result) + " " + standardLength(operand1) + " " + standardLength(operand2);
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
	this->id = -1;
	this->invariant = "";
	this->codeout = "";
}

void CodeItem::setID(int id) {
	this->id = id;
}

int CodeItem::getId()
{
	return this->id;
}

void CodeItem::setInvariant() {
	this->invariant = "invariant";
}

int CodeItem::getInvariant()
{
	return this->invariant == "invariant";
}

void CodeItem::setCodeOut() {
	this->codeout = "codeout!";
}

int CodeItem::getCodeOut()
{
	return this->codeout == "codeout!";
}

irCodeType CodeItem::getCodetype()
{
	return this->codetype;
}

void CodeItem::setCodetype(irCodeType type) {
	this->codetype = type;
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

void CodeItem::setInstr(string res, string ope1, string ope2) {
	this->result = res;
	this->operand1 = ope1;
	this->operand2 = ope2;
}

void CodeItem::setFatherBlock(vector<int> a)
{
	this->fatherBlock = a;
}
vector<int> CodeItem::getFatherBlock()
{
	return this->fatherBlock;
}
void CodeItem::setFuncName(string name)
{
	this->funcName = name;
}
string CodeItem::getFuncName()
{
	return this->funcName;
}

#define ENUM_TO_STRING(enumName) (#enumName)


void CodeItem::changeContent(string res, string ope1, string ope2)
{
	this->result = res;
	this->operand1 = ope1;
	this->operand2 = ope2;
}