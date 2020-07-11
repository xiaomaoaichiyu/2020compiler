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

void irOptimize();

//将MIR转换为LIR，主要是替换操作数，全部放到虚拟寄存器中VR中，编号从0开始
//void MIR2LIRpass(vector<vector<CodeItem>>& irCodes);


#endif // !_OPTIMIZE_H_
