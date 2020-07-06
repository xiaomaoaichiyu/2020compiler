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

int frontExecute();

extern vector<vector<CodeItem>> codetotal;   //总中间代码

#endif // !_SYNTAX_H_
