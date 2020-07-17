#include "ssa.h"
#include "../front/symboltable.h"
#include "../front/syntax.h"

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
				if (ci.getCodetype() == NOTE && ci.getOperand1().compare("func") == 0 && ci.getOperand2().compare("begin") == 0) {
					string funName = ci.getResult();				// 要调用的函数名
					int funNum = funName2Num[funName];	// 该函数对应的序号
					if (!inlineFlag[funNum]) continue;
					// 要调用的函数可以内联
					symbolTable st;								// 函数结构
					for (int iter1 = 1; iter1 < total.size(); iter1++) {
						if (total[iter1][0].getName().compare(funName) == 0) {
							st = total[iter1][0]; 
							break;
						}
					}
					int paraNum = st.getparaLength();	// 参数个数
					vector<string> paraTable;				// 参数列表
					for (int iter1 = 1; iter1 <= paraNum; iter1++) {
						paraTable.push_back(blockCore[funNum][1].Ir[iter1].getResult());
					}
					/* 1. 处理传参指令 */
					k = k + 1;
					for (int iter2 = paraNum; iter2 > 0; iter2--, k++) {	// 中间代码倒序，而符号表顺序
						ci = blockCore[i][j].Ir[k];
						if (ci.getCodetype() == LOAD || ci.getCodetype() == LOADARR) {
							k++;
							ci = blockCore[i][j].Ir[k];
							if (ci.getCodetype() == PUSH) {
								CodeItem nci(STORE, ci.getOperand1(), paraTable[iter2 - 1], "");
								blockCore[i][j].Ir[k] = nci;
							}
							else { cout << "inline_function: 我没有考虑到的情况1" << endl; }
						}
						else if (ci.getCodetype() == PUSH) {
							CodeItem nci(STORE, ci.getOperand1(), paraTable[iter2 - 1], "");
							blockCore[i][j].Ir[k] = nci;
						}
						else { cout << "inline_function: 我没有考虑到的情况2" << endl; }
					}
					/* 2. 插入调用函数的除函数和参数声明之外的指令 */
					vector<CodeItem> v = blockCore[funNum][1].Ir;
					for (int iter2 = paraNum + 1; iter2 < v.size(); iter2++, k++) {
						blockCore[i][j].Ir.insert(blockCore[i][j].Ir.begin() + k, v[iter2]);
					}
					/* 3. 删除call指令 */
					ci = blockCore[i][j].Ir[k];
					if (ci.getCodetype() == CALL) {
						if (st.getValuetype() == INT) {	// 有返回值函数调用
							k--;	ci = blockCore[i][j].Ir[k];
							if (ci.getCodetype() == RET) {
								CodeItem nci(MOV, "", blockCore[i][j].Ir[k + 1].getResult(), ci.getOperand1());
								blockCore[i][j].Ir[k] = nci;
								blockCore[i][j].Ir.erase(blockCore[i][j].Ir.begin() + k + 1);
							}
							else { cout << "inline_function: 我没有考虑到的情况3" << endl; }
						}
						else {	// 无返回值函数调用
							k--;	ci = blockCore[i][j].Ir[k];
							if (ci.getCodetype() == RET) {
								blockCore[i][j].Ir.erase(blockCore[i][j].Ir.begin() + k);
								blockCore[i][j].Ir.erase(blockCore[i][j].Ir.begin() + k);
								k--;
							}
							else { cout << "inline_function: 我没有考虑到的情况4" << endl; }
						}					
					}
					else { cout << "inline_function: 我没有考虑到的情况5" << endl; }
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