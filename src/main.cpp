﻿#include <iostream>
#include "front/syntax.h"
#include "ir/ssa.h"
#include "arm/armgenerate.h"

using namespace std;

int main(int argc, char* argv[])
{
	//运行前端
	frontExecute();

	//运行优化
	//SSA ssa(codetotal, total);
	//ssa.generate();

	//运行后端，生成arm代码
	arm_generate_without_register();
	return 0;
}
