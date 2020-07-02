#include<string>
#include<fstream>
#include "word.h"
#define MAX_LENGTH 10000
using namespace std;
Word::Word(string filename)
{
	ifstream infile;
	infile.open(filename);
	char data[MAX_LENGTH];
	while (infile.getline(data, MAX_LENGTH)) {
		file = file + data;
		file += '\n';
	}
	infile.close();
	fRecord = 0;
}
int Word::getNumber()
{
	return number;
}
string Word::getToken()
{
	return token;
}
void Word::setToken(string Token)
{
	this->token = Token;
}
int Word::getfRecord()
{
	return this->fRecord;
}
void Word::setfRecord(int record)
{
	this->fRecord = record;
}
Memory Word::getSymbol()
{
	return symbol;
}
void Word::setSymbol(Memory symbol)
{
	this->symbol = symbol;
}
void Word::getmark()
{
	mark = file[fRecord++];
}
void Word::catToken()
{
	token += mark;
}
void Word::clearToken()
{
	token.clear();
}
void Word::retract()
{
	fRecord--;
}
void Word::transNum10()
{
	int i;
	number = 0;
	for (i = 0; i < token.size(); i++) {
		number = number * 10 + token[i] - '0';
	}
}
void Word::transNum8()
{
	int i;
	number = 0;
	for (i = 0; i < token.size(); i++) {
		number = number * 8 + token[i] - '0';
	}
}
void Word::transNum16()
{
	int i,j;
	number = 0;
	for (i = 2; i < token.size(); i++) {
		if (isdigit(token[i])) {
			j = token[i] - '0';
		}
		else {
			if (token[i] >= 'a' && token[i] <= 'f') {
				j = token[i] - 'a' + 10;
			}
			else {
				j = token[i] - 'A' + 10;
			}
		}
		number = number * 16 + j;
	}
}
bool Word::getsym()   //为后期做准备，可以判断是否读完文件了
{
	clearToken();
	do {
		getmark();
	} while (mark == ' ' || mark == '	' || mark == '\n' || mark == '\r');
	if (isalpha(mark) || mark == '_') {
		while (isalpha(mark) || isdigit(mark) || mark == '_') {
			catToken();
			getmark();
		}
		retract();
		int i;
		for (i = 0; i < keyword; i++) {
			if (token == remain[i]) {
				symbol = (Memory)i;
				break;
			}
		}
		if (i == keyword) {
			symbol = IDENFR;
		}
	}
	else if (isdigit(mark)) {
		if (mark == '0') {   //八进制或者16进制
			catToken();
			getmark();
			if (mark == 'x' || mark == 'X') {  //16进制
				while (isdigit(mark) || isalpha(mark)) {
					catToken();
					getmark();
				}
				retract();  //回退
				transNum16();
				symbol = INTCON;
			}
			else {          //8进制
				while (isdigit(mark)) {
					catToken();
					getmark();
				}
				retract();  //回退
				transNum8();
				symbol = INTCON;
			}
		}
		else {			//十进制
			while (isdigit(mark)) {
				catToken();
				getmark();
			}
			retract();  //回退
			transNum10();
			symbol = INTCON;
		}
		while (isdigit(mark)) {
			catToken();
			getmark();
		}
	}
	else if (mark == '=') {
		catToken();
		getmark();
		if (mark == '=') {
			catToken();
			symbol = EQL_WORD;
		}
		else {
			retract();  //回退
			symbol = ASSIGN;
		}
	}
	else if (mark == '>') {
		catToken();
		getmark();
		if (mark == '=') {
			catToken();
			symbol = GEQ;
		}
		else {
			retract();
			symbol = GRE;
		}
	}
	else if (mark == '<') {
		catToken();
		getmark();
		if (mark == '=') {
			catToken();
			symbol = LEQ;
		}
		else {
			retract();
			symbol = LSS;
		}
	}
	else if (mark == '+') {
		catToken();
		symbol = PLUS;
	}
	else if (mark == '-') {
		catToken();
		symbol = MINU;
	}
	else if (mark == '*') {
		catToken();
		symbol = MULT;
	}
	else if (mark == '/') {
		getmark();
		if (mark == '/') {
			while (mark != '\r' && mark != '\n') {
				getmark();
			}
			getsym();
		}
		else if (mark == '*') {
			while (true) {
				getmark();
				if (mark == '*') {
					getmark();
					if (mark == '/') {
						break;
					}
					else {
						retract();
					}
				}
			}
			getsym();
		}
		else {
			retract();
			token += '/' ;
			symbol = DIV_WORD;
		}
	}
	else if (mark == '%') {
		catToken();
		symbol = MOD;
	}
	else if (mark == '!') {
		catToken();
		getmark();  //把=读入
		if (mark == '=') {
			catToken();
			symbol = NEQ_WORD;
		}
		else {
			retract();
			symbol = REV;
		}
	}        
	else if (mark == '\"') {
		catToken();
		getmark();
		while (mark != '\"') {
			catToken();
			getmark();
		}
		catToken();
		//不用回退
		symbol = STRING;   //随便给的
 	}
	else if (mark == ';') {
		catToken();
		symbol = SEMICN;
	}
	else if (mark == ',') {
		catToken();
		symbol = COMMA;
	}
	else if (mark == '&') {
		catToken();
		getmark();
		catToken();
		symbol = AND_WORD;
	}
	else if (mark == '|') {
		catToken();
		getmark();
		catToken();
		symbol = OR_WORD;
	}
	else if (mark == '(') {
		catToken();
		symbol = LPARENT;
	}
	else if (mark == ')') {
		catToken();
		symbol = RPARENT;
	}
	else if (mark == '[') {
		catToken();
		symbol = LBRACK;
	}
	else if (mark == ']') {
		catToken();
		symbol = RBRACK;
	}
	else if (mark == '{') {
		catToken();
		symbol = LBRACE;
	}
	else if (mark == '}') {
		catToken();
		symbol = RBRACE;
	}
	else if (file.size() <= fRecord) {
		symbol = FINISH;
	}  //终止符
	if (symbol == FINISH) {
		return false;
	}
	else {
		return true;
	}
}