#ifndef _SYNTAX_H_
#define _SYNTAX_H_

#include<string>
#include<fstream>
#include<iostream>
#include<vector>
#include<sstream>
#include<map>
#include<set>

#include "word.h"
#include "symboltable.h"
#include "../ir/intermediatecode.h"

int frontExecute(string syname);

void TestIrCode(string outputFile);

extern vector<vector<CodeItem>> codetotal;   //���м����
extern vector<vector<symbolTable>> total;	//�ܷ��ű�
extern vector<int> func2tmpIndex;	//��ʱ��������ֵ

#endif // !_SYNTAX_H_
