#include "ssa.h"

void SSA::ssa_optimize() {
	// 重新计算use-def关系
	build_def_use_chain();
	// 重新进行活跃变量分析
	active_var_analyse();
	// 死代码删除
	delete_dead_codes();
}

// 删除死代码
void SSA::delete_dead_codes() {
	int i, j, k;
	int size1, size2, size3;
	size1 = blockCore.size();
	for (i = 1; i < size1; i++) {	// 遍历函数，跳过全局定义
		size2 = blockCore[i].size();
		for (j = 0; j < size2; j++) {	// 遍历基本块，可选择跳过entry和exit块
			bool update = true;
			while (update) {
				update = false;
				set<string> tmpout = blockCore[i][j].out;
				size3 = blockCore[i][j].Ir.size();
				for (k = size3 - 1; k >= 0; k--) {
					CodeItem ci = blockCore[i][j].Ir[k];
					switch (ci.getCodetype())
					{
					case ADD: case SUB: case MUL: case DIV: case REM:
					case AND: case OR: case NOT: case EQL: case NEQ: case SGT: case SGE: case SLT: case SLE:
					case LOAD:
					case BR:
						if (tmpout.find(ci.getResult()) == tmpout.end()) {
							update = true;
							blockCore[i][j].Ir.erase(blockCore[i][j].Ir.begin() + k);
						}
						else {
							if (ifTempVariable(ci.getOperand1()) || ifLocalVariable(ci.getOperand1())) tmpout.insert(ci.getOperand1());
							if (ifTempVariable(ci.getOperand2()) || ifLocalVariable(ci.getOperand2())) tmpout.insert(ci.getOperand2());
						}
						break;
					case STORE:
						if (tmpout.find(ci.getOperand1()) == tmpout.end()) {
							update = true;
							blockCore[i][j].Ir.erase(blockCore[i][j].Ir.begin() + k);
						}
						else {
							if (ifTempVariable(ci.getResult()) || ifLocalVariable(ci.getResult())) tmpout.insert(ci.getResult());
						}
						break;
					case STOREARR: case LOADARR:
						// 数组不敢删除
						if (ifTempVariable(ci.getResult()) || ifLocalVariable(ci.getResult())) tmpout.insert(ci.getResult());
						if (ifTempVariable(ci.getOperand1()) || ifLocalVariable(ci.getOperand1())) tmpout.insert(ci.getOperand1());
						if (ifTempVariable(ci.getOperand2()) || ifLocalVariable(ci.getOperand2())) tmpout.insert(ci.getOperand2());
						break;
					case CALL: case RET: case PUSH: case PARA:
						// 与函数相关，不能删除
						if (ifTempVariable(ci.getResult()) || ifLocalVariable(ci.getResult())) tmpout.insert(ci.getResult());
						if (ifTempVariable(ci.getOperand1()) || ifLocalVariable(ci.getOperand1())) tmpout.insert(ci.getOperand1());
						if (ifTempVariable(ci.getOperand2()) || ifLocalVariable(ci.getOperand2())) tmpout.insert(ci.getOperand2());
						break;
					case LABEL: case DEFINE: case ALLOC: case GLOBAL: case NOTE:
						// 不做处理
						break;
					default:
						break;
					}
				}
			}
		}
	}
}