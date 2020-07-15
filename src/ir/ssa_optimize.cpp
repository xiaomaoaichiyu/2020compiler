#include "ssa.h"

void SSA::ssa_optimize() {
	// 重新计算use-def关系
	build_def_use_chain();
	// 重新进行活跃变量分析
	active_var_analyse();
	// 死代码删除
	delete_dead_codes();
	// 函数内联
	judge_inline_function();
	inline_function();
}

void SSA::judge_inline_function() {
	// 函数内联的条件：只有一个基本块且其中没有函数调用指令
	inlineFlag.push_back(false);
	funName2Num[""] = 0;	// 全局定义，这个可加可不加，不影响
	int i, j, k;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {	// 遍历函数
		bool flag = true;
		int size2 = blockCore[i].size();
		if (size2 > 3) flag = false;	// 除entry和exit块外不只有一个基本块
		else {
			int size3 = blockCore[i][1].Ir.size();
			for (j = 0; j < size3; j++) {
				CodeItem ci = blockCore[i][1].Ir[j];
				if (ci.getCodetype() == CALL) {	// 含有函数调用指令
					flag = false;
					break;
				}
			}
		}
		inlineFlag.push_back(flag);
		funName2Num[blockCore[i][1].Ir[0].getResult()] = i;
	}
}

// 函数内联
void SSA::inline_function() {
	int i, j, k;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {	// 遍历函数
		int size2 = blockCore[i].size();
		for (j = 0; j < size2; j++) {	// 遍历基本块
			// int size3 = blockCore[i][j].Ir.size(); 不能事先求好size3，因为其大小可能会变
			for (k = 0; k < blockCore[i][j].Ir.size(); k++) {
				CodeItem ci = blockCore[i][j].Ir[k];
				if (ci.getCodetype() == CALL) {
					int funNum = funName2Num[ci.getOperand1()];
					if (inlineFlag[funNum]) {
						// 要调用的函数可以内联

					}
				}
			}
		}
	}
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
					case STOREARR: case LOADARR:	// 数组不敢删除
					case CALL: case RET: case PUSH: case PARA:	// 与函数相关，不能删除
					case BR:	// 跳转指令不能删除
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