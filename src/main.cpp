#include <iostream>
#include "front/syntax.h"
#include "ir/ssa.h"
#include "arm/armgenerate.h"
#include "ir/optimize.h"

using namespace std;

string syname = "testcase.sy";
string sname = "testcase.s";

void checkArguments(int argc, char* argv[]) {
	if (argc < 2) return;
	try {
		for (int i = 1; i < argc; i++) {
			if (argv[i][0] == '-') {
				string argument(argv[i]);
				if (argument == "-o") {
					sname = argv[i + 1];
					i++;
				}
				else if (argument == "-S") {
					//生成汇编
					cout << "Generate ARM" << endl;
				}
				else {
					//待定
				}
			}
			else {
				syname = argv[i];
			}
		}
	}
	catch (exception e) {
		cerr << e.what() << "\t Execute argument wrong!" << endl;
	}
}

int main(int argc, char* argv[])
{
	checkArguments(argc, argv);
	//运行前端
	frontExecute(syname);

	//运行优化
	/*SSA ssa;
	ssa.generate();*/

	irOptimize();

	TestIrCode("ir2.txt");
	//运行后端，生成arm代码

	//arm_generate_without_register(sname);

	return 0;
}
