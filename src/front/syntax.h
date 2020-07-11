#ifndef _SYNTAX_H_
#define _SYNTAX_H_

#include<string>
#include<fstream>
#include<iostream>
#include<vector>
#include<sstream>
#include<map>

#include "word.h"
#include "symboltable.h"
#include "../ir/intermediatecode.h"

int frontExecute(string syname);

void TestIrCode(string outputFile);

extern vector<vector<CodeItem>> codetotal;   //总中间代码
extern vector<vector<symbolTable>> total;	//总符号表

#endif // !_SYNTAX_H_
