#ifndef _OPTIMIZE_H_
#define _OPTIMIZE_H_

#include "../util/meow.h"
#include "../ir/intermediatecode.h"
#include "../ir/ssa.h"
#include "../front/syntax.h"

#include <vector>
#include <string>
#include <map>

using namespace std;

extern vector<vector<CodeItem>> LIR;

extern vector<vector<string>> stackVars;

//保存每个函数使用的全局寄存器，从1开始编号，和LIR一样
extern vector<vector<string>> func2gReg;
//保存每个函数用到的临时变量，VR->定义顺序
extern vector<map<string, int>> func2Vr;

void irOptimize();

void countVars();

void printLIR(string outputFile);


//将MIR转换为LIR，主要是替换操作数，全部放到虚拟寄存器中VR中，编号从0开始
//void MIR2LIRpass();

#endif // !_OPTIMIZE_H_
