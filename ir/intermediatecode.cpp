#include "intermediatecode.h"
#include <string>

CodeItem::CodeItem(irCodeType type, string res, string ope1, string ope2){
	this->codetype = type;
	this->result = res;
	this->operand1 = ope1;
	this->operand2 = ope2;
	this->content = res + " " + ope1 + " " + ope2;  //需要写一个将enum转换为字符串的函数
	//setContent(type, res, ope1, ope2);
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

string setContent(irCodeType type, string res, string ope1, string ope2) {
	string res = "";
	switch (type) {
		case ADD:{
			
			break;
		}
		case SUB:{
			break;
		}
		case DIV:{
			break;
		}
		case MUL:{
			break;
		}
		case REM:{
			break;
		}
		case AND:{
			break;
		}
		case OR:{
			break;
		}
		case NOT:{
			break;
		}
		case EQL:{
			break;
		}
		case NEQ:{
			break;
		}
		case SGT:{
			break;
		}
		case SGE:{
			break;
		}
		case SLT:{
			break;
		}
		case SLE:{
			break;
		}
		case ALLOC:{
			break;
		}
		case STORE:{
			break;
		}
		case LOAD:{
			break;
		}
		case INDEX:{
			break;
		}
		case CALL:{
			break;
		}
		case RET:{
			break;
		}
		case PUSH:{
			break;
		}
		case POP:{
			break;
		}
		case LABEL:{
			break;
		}
		case BR:{
			break;
		}
		case DEFINE:{
			break;
		}
		case PARA:{
			break;
		}
		case GLOBAL:{
			break;
		}
		default:
			break;
	}
}