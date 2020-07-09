#include <iostream>
#include "front/syntax.h"
#include "ir/ssa.h"
#include "arm/armgenerate.h"

using namespace std;

int main(int argc, char* argv[])
{
	string syname = argv[4];
	string sname = argv[3];
	//运行前端
	frontExecute(syname);

	//运行优化
	//SSA ssa(codetotal, total);
	//ssa.generate();

	//运行后端，生成arm代码
	arm_generate_without_register(sname);
	return 0;
}
