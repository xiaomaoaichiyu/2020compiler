#include <iostream>
#include "front/syntax.h"
#include "ir/ssa.h"

using namespace std;

int main()
{
	//运行前端
	frontExecute();

	//运行优化
	SSA ssa(codetotal, total);
	ssa.generate();

	//运行后端，生成arm代码

	return 0;
}
