#include "ssa.h"
#include "../front/symboltable.h"
#include "../front/syntax.h"
#include "../util/meow.h"
#include "../ir/dataAnalyse.h"

#include <queue>
#include <unordered_map>

//==========================================================

void printCircleIr(vector<vector<basicBlock>>& v, ofstream& debug_ssa) {
	if (TIJIAO) {
		int size1 = v.size();
		for (int i = 1; i < size1; i++) {
			// 首先输出这是第几个函数
			debug_ssa << "---------------- function " << i << " ----------------" << endl;
			int size2 = v[i].size();
			// 依次输出该函数下所有的基本块信息
			for (int j = 0; j < size2; j++) {
				debug_ssa << "\n" << "B" << v[i][j].number << "\t\t";	// 基本块编号
				set<int>::iterator iter;
				// 即可以跳转到该基本块的基本块序号
				debug_ssa << "前序节点和后序节点: {  ";
				for (iter = v[i][j].pred.begin(); iter != v[i][j].pred.end(); iter++) debug_ssa << *iter << "  ";
				debug_ssa << "}" << "\t\t";
				// 即通过该基本块可以跳转到的基本块序号
				debug_ssa << "{  ";
				for (iter = v[i][j].succeeds.begin(); iter != v[i][j].succeeds.end(); iter++) debug_ssa << *iter << "  ";
				debug_ssa << "}" << endl;
				// 输出该基本块中的中间代码
				for (int k = 0; k < v[i][j].Ir.size(); k++) {
					auto instr = v[i][j].Ir.at(k);
					debug_ssa << k << "    " << instr.getContent() << endl;
				}
			}
		}
	}
}

// 在睿轩生成的中间代码做优化
void SSA::pre_optimize() {
	// 简化load和store指令相邻的指令
	load_and_store();
	// 简化加减0、乘除模1这样的指令
	simplify_add_minus_multi_div_mod();
	// 简化紧邻的跳转
	simplify_br_label();
	// 删除多余alloc指令
	simplify_alloc();
}

void SSA::get_avtiveAnalyse_result() {

}

//ssa形式上的优化
void SSA::ssa_optimize() {
	
	// 常量传播
	if (1) {
		// 重新计算use-def关系
		build_def_use_chain();
		// 重新进行活跃变量分析
		active_var_analyse();
		//常量传播
		const_propagation();
	}

	// 死代码删除
	if (1) {
		// 重新计算use-def关系
		build_def_use_chain();
		// 重新进行活跃变量分析
		active_var_analyse();
		// 死代码删除
		delete_dead_codes();
	}
	
	if (0) { // 关闭函数内联
	// 重新计算use-def关系
		build_def_use_chain();
		// 重新进行活跃变量分析
		active_var_analyse();
		// 函数内联
		inline_function();
	}

	if (0) { // 关闭循环优化
	// 将phi函数加入到中间代码
		add_phi_to_Ir();

		//循环优化
		count_UDchains();		//计算使用-定义链
		back_edge();			//找循环
		mark_invariant();		//确定不变式
		ofstream ly1("lyceshi1.txt");
		printCircleIr(this->blockCore, ly1);

		code_outside();			//不变式外提

		ofstream ly2("lyceshi2.txt");
		printCircleIr(this->blockCore, ly2);

		// 删除中间代码中的phi
		delete_Ir_phi();
	}
	//count_use_def_chain();
}

//========================================================

// 删除多余alloc指令
void SSA::simplify_alloc() {
	int i, j, k;
	int size1 = codetotal.size();
	for (i = 1; i < size1; i++) {
		map<string, bool> ifUse;
		int size2 = codetotal[i].size();
		for (j = 1; j < size2; j++) {
			CodeItem ci = codetotal[i][j];
			if (ci.getCodetype() != ALLOC) {
				if (ifLocalVariable(ci.getResult())) ifUse[ci.getResult()] = true;
				if (ifLocalVariable(ci.getOperand1())) ifUse[ci.getOperand1()] = true;
				if (ifLocalVariable(ci.getOperand2())) ifUse[ci.getOperand2()] = true;
			}
		}
		for (j = 1; j < codetotal[i].size(); j++) {
			CodeItem ci = codetotal[i][j];
			if (ci.getCodetype() == ALLOC && ifUse.find(ci.getResult()) == ifUse.end()) {
				codetotal[i].erase(codetotal[i].begin() + j);
				j--;
			}
			if (ci.getCodetype() != ALLOC && ci.getCodetype() != DEFINE && ci.getCodetype() != PARA)
				break;
		}
	}
}

// 简化条件判断为常值的跳转指令
void SSA::simplify_br() {
	int i, j, k;
	int size1 = codetotal.size();
	for (i = 1; i < size1; i++) {
		int size2 = codetotal[i].size();
		for (j = 0; j < size2; j++) {
			CodeItem ci = codetotal[i][j];
			if (ci.getCodetype() == BR && ifDigit(ci.getOperand1())) {
				// CodeItem nci(BR, "", "", "");
				if (ci.getOperand1().compare("0") == 0) {
					CodeItem nci(BR, "", ci.getResult(), "");
					codetotal[i].erase(codetotal[i].begin() + j);
					codetotal[i].insert(codetotal[i].begin() + j, nci);
					// nci.setResult(ci.getOperand1());
				}
				else {
					CodeItem nci(BR, "", ci.getOperand2(), "");
					codetotal[i].erase(codetotal[i].begin() + j);
					codetotal[i].insert(codetotal[i].begin() + j, nci);
					// nci.setResult(ci.getOperand2());
				}
			}
		}
	}
}

// 简化load和store指令相邻的指令
/*
 a = a;
 load       %0         %a
 store      %0         %a
 */
void SSA::load_and_store() {
	int i, j, k;
	int size = codetotal.size();
	for (i = 1; i < codetotal.size(); i++) {
		for (j = 0; j < codetotal[i].size() - 1; j++) {
			CodeItem ci1 = codetotal[i][j];
			CodeItem ci2 = codetotal[i][j + 1];
			if (((ci1.getCodetype() == LOAD && ci2.getCodetype() == STORE) || 
				(ci1.getCodetype() == STORE && ci2.getCodetype() == LOAD) || 
				(ci1.getCodetype() == LOADARR && ci2.getCodetype() == STOREARR) || 
				(ci1.getCodetype() == STOREARR && ci2.getCodetype() == LOADARR))
				&& ci1.getResult().compare(ci2.getResult()) == 0
				&& ci1.getOperand1().compare(ci2.getOperand1()) == 0
				&& ci1.getOperand2().compare(ci2.getOperand2()) == 0) {
				codetotal[i].erase(codetotal[i].begin() + j);
				codetotal[i].erase(codetotal[i].begin() + j);
				j--;
			}
		}
	}
}

// 简化加减0、乘除模1这样的指令
/*
 b = a + 0;
 load       %1         %a
 add        %2         %1         0
 store      %2         %b
 -->
 load       %2         %a
store      %2         %b
 */
void SSA::simplify_add_minus_multi_div_mod() {
	int i, j, k;
	int size = codetotal.size();
	for (i = 1; i < codetotal.size(); i++) {
		for (j = 1; j < codetotal[i].size(); j++) {
			CodeItem ci = codetotal[i][j];
			CodeItem ci0 = codetotal[i][j - 1]; // 前一条中间代码
			if (ci.getCodetype() == ADD) {
				if (ci.getOperand1().compare("0") == 0) {
					if ((ci0.getCodetype() == LOAD || ci0.getCodetype() == LOADARR)
						&& (ci0.getResult().compare(ci.getOperand2()) == 0)) {
						CodeItem nci(ci0.getCodetype(), ci.getResult(), ci0.getOperand1(), ci0.getOperand2());
						codetotal[i].insert(codetotal[i].begin() + j - 1, nci);
						codetotal[i].erase(codetotal[i].begin() + j);
						codetotal[i].erase(codetotal[i].begin() + j);
						j--;
					}
				}
				else if (ci.getOperand2().compare("0") == 0) {
					if ((ci0.getCodetype() == LOAD || ci0.getCodetype() == LOADARR)
						&& (ci0.getResult().compare(ci.getOperand1()) == 0)) {	// 只有这句不同
						CodeItem nci(ci0.getCodetype(), ci.getResult(), ci0.getOperand1(), ci0.getOperand2());
						codetotal[i].insert(codetotal[i].begin() + j - 1, nci);
						codetotal[i].erase(codetotal[i].begin() + j);
						codetotal[i].erase(codetotal[i].begin() + j);
						j --;
					}
				}
			}
			else if (ci.getCodetype() == SUB) {
				if (ci.getOperand2().compare("0") == 0) {
					if ((ci0.getCodetype() == LOAD || ci0.getCodetype() == LOADARR)
						&& (ci0.getResult().compare(ci.getOperand1()) == 0)) {	// 只有这句不同
						CodeItem nci(ci0.getCodetype(), ci.getResult(), ci0.getOperand1(), ci0.getOperand2());
						codetotal[i].insert(codetotal[i].begin() + j - 1, nci);
						codetotal[i].erase(codetotal[i].begin() + j);
						codetotal[i].erase(codetotal[i].begin() + j);
						j --;
					}
				}
			}
			else if (ci.getCodetype() == MUL) {
				if (ci.getOperand1().compare("1") == 0) {
					if ((ci0.getCodetype() == LOAD || ci0.getCodetype() == LOADARR)
						&& (ci0.getResult().compare(ci.getOperand2()) == 0)) {	// 只有这句不同
						CodeItem nci(ci0.getCodetype(), ci.getResult(), ci0.getOperand1(), ci0.getOperand2());
						codetotal[i].insert(codetotal[i].begin() + j - 1, nci);
						codetotal[i].erase(codetotal[i].begin() + j);
						codetotal[i].erase(codetotal[i].begin() + j);
						j --;
					}
				}
				else if (ci.getOperand2().compare("1") == 0) {
					if ((ci0.getCodetype() == LOAD || ci0.getCodetype() == LOADARR)
						&& (ci0.getResult().compare(ci.getOperand1()) == 0)) {	// 只有这句不同
						CodeItem nci(ci0.getCodetype(), ci.getResult(), ci0.getOperand1(), ci0.getOperand2());
						codetotal[i].insert(codetotal[i].begin() + j - 1, nci);
						codetotal[i].erase(codetotal[i].begin() + j);
						codetotal[i].erase(codetotal[i].begin() + j);
						j --;
					}
				}
			}
			else if (ci.getCodetype() == DIV) {
				if (ci.getOperand2().compare("1") == 0) {
					if ((ci0.getCodetype() == LOAD || ci0.getCodetype() == LOADARR)
						&& (ci0.getResult().compare(ci.getOperand1()) == 0)) {	// 只有这句不同
						CodeItem nci(ci0.getCodetype(), ci.getResult(), ci0.getOperand1(), ci0.getOperand2());
						codetotal[i].insert(codetotal[i].begin() + j - 1, nci);
						codetotal[i].erase(codetotal[i].begin() + j);
						codetotal[i].erase(codetotal[i].begin() + j);
						j --;
					}
				}
			}
			else if (ci.getCodetype() == REM) {
				if (ci.getOperand2().compare("1") == 0 || ci.getOperand2().compare(ci.getOperand1()) == 0) {

				}
			}
		}
	}
}

// 简化紧邻的跳转
/*
 br                    %if.end_0            
 label      %if.end_0 
 */
void SSA::simplify_br_label() {
	int i, j, k;
	int size = codetotal.size();
	for (i = 1; i < codetotal.size(); i++) {
		for (j = 0; j < codetotal[i].size() - 1; j++) {
			CodeItem ci1 = codetotal[i][j];
			CodeItem ci2 = codetotal[i][j + 1];
			if (ci1.getCodetype() == BR && ci2.getCodetype() == LABEL && ci1.getOperand1().compare(ci2.getResult()) == 0) {
				codetotal[i].erase(codetotal[i].begin() + j);
				// label就不删了
			}
		}
	}
}

// 函数内联
void SSA::inline_function() {
	int i, j, k;
	int size1 = blockCore.size();
	// 建立函数名与对应序号的对应关系
	for (i = 1; i < size1; i++) {
		string funName = codetotal[i][0].getResult();	// define
		funName2Num[funName] = i;
		funNum2Name[i] = funName;
	}
	for (i = 1; i < size1; i++) {	// 遍历函数
		int size2 = blockCore[i].size();
		vector<CodeItem> newInsertAllocIr;	// 要在该函数开始添加的alloc中间代码
		set<string> newInsertAllocName;	// 要在该函数开始添加的alloc变量名
		int labelIndex = 0;					// 重命名标签编号
		for (j = 0; j < size2; j++) {	// 遍历基本块
			// int size3 = blockCore[i][j].Ir.size(); 不能事先求好size3，因为其大小可能会变
			for (k = 0; k < blockCore[i][j].Ir.size(); k++) {
				CodeItem ci = blockCore[i][j].Ir[k];
				if (ci.getCodetype() == NOTE 
					&& ci.getOperand1().compare("func") == 0 
					&& ci.getOperand2().compare("begin") == 0
					&& funName2Num.find(ci.getResult()) != funName2Num.end()) {
					string funName = ci.getResult();				// 函数名
					int funNum = funName2Num[funName];	// 该函数对应的序号
					symbolTable funSt;									// 函数对应的符号表结构
					for (int iter1 = 1; iter1 < total.size(); iter1++) {
						if (total[iter1][0].getName().compare(funName) == 0) {
							funSt = total[iter1][0]; 
							break;
						}
					}
					if (funSt.getName().compare(funName) != 0) {
						cout << "函数内联不应该发生的情况：没有在符号表中找到函数定义." << endl;
						break;
					}
					else if (!funSt.getisinlineFunc()) {	// 不能内联，该调用函数不是叶子函数
						continue;
					}
					else {
						cout << "在函数 " << funNum2Name[i] << " 中内联函数 " << funName << endl;
					}
					int paraNum = funSt.getparaLength();	// 参数个数
					vector<string> paraTable;					// 参数列表
					for (int iter1 = 1; iter1 <= paraNum; iter1++) {
						string paraName = blockCore[funNum][1].Ir[iter1].getResult();
						paraTable.push_back(paraName);
						if (newInsertAllocName.find(paraName) == newInsertAllocName.end()) {
							newInsertAllocName.insert(paraName);
							CodeItem nci(ALLOC, paraName, "", "1");
						}
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
						if (funSt.getValuetype() == INT) {	// 有返回值函数调用
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
							if (ifGlobalVariable(ci.getOperand1()) || ifLocalVariable(ci.getOperand1()) || ifTempVariable(ci.getOperand1())) 
								tmpout.insert(ci.getOperand1());
							if (ifGlobalVariable(ci.getOperand2()) || ifLocalVariable(ci.getOperand2()) || ifTempVariable(ci.getOperand2()))
								tmpout.insert(ci.getOperand2());
						}
						break;
					case STORE:
						if (tmpout.find(ci.getOperand1()) == tmpout.end() && !ifGlobalVariable(ci.getOperand1())) {
							update = true;
							blockCore[i][j].Ir.erase(blockCore[i][j].Ir.begin() + k);
						}
						else {
							if (ifGlobalVariable(ci.getResult()) || ifLocalVariable(ci.getResult()) || ifTempVariable(ci.getResult()))
								tmpout.insert(ci.getResult());
						}
						break;
					case STOREARR: case LOADARR:	// 数组不敢删除
					case CALL: case RET: case PUSH: case PARA:	// 与函数相关，不能删除
					case BR:	// 跳转指令不能删除
						if (ifGlobalVariable(ci.getResult()) || ifLocalVariable(ci.getResult()) || ifTempVariable(ci.getResult()))
							tmpout.insert(ci.getResult());
						if (ifGlobalVariable(ci.getOperand1()) || ifLocalVariable(ci.getOperand1()) || ifTempVariable(ci.getOperand1())) 
							tmpout.insert(ci.getOperand1());
						if (ifGlobalVariable(ci.getOperand2()) || ifLocalVariable(ci.getOperand2()) || ifTempVariable(ci.getOperand2()))
							tmpout.insert(ci.getOperand2());
						break;
					case LABEL: case DEFINE: case GLOBAL: case NOTE:case ALLOC:
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

//==================================================
//	常量传播 + 复写传播
//==================================================

map<string, string> var2value;

void SSA::const_propagation() {
	for (int i = 1; i < blockCore.size(); i++) {	//遍历每个函数
		auto& blocks = blockCore.at(i);
		for (int j = 0; j < blocks.size(); j++) {	//遍历每个基本的ir
			auto& ir = blocks.at(j).Ir;
			var2value.clear();
			//常量替换+常量折叠
			for (int k = 0; k < ir.size(); k++) {
				auto& instr = ir.at(k);
				auto op = instr.getCodetype();
				auto res = instr.getResult();
				auto ope1 = instr.getOperand1();
				auto ope2 = instr.getOperand2();
				switch (op) {
				case ADD: case SUB: case DIV: case MUL: case REM: 
				case AND: case OR: case NOT:
				case EQL: case NEQ: case SGT: case SGE: case SLT: case SLE: {	//一个def 两个use （not例外）
					if (var2value.find(ope1) != var2value.end()) {	//操作数1的值是一个立即数，替换
						instr.setOperand1(var2value[ope1]);
					}
					if (var2value.find(ope2) != var2value.end()) {	//操作数2的值是一个立即数，替换
						instr.setOperand2(var2value[ope2]);
					}
					if (isNumber(instr.getOperand1()) && isNumber(instr.getOperand2())) {	//两个操作数都是立即数
						string tmp = calculate(op, instr.getOperand1(), instr.getOperand2());
						//instr.setResult(tmp); //设置结果对应的值即可，在死代码删除的时候删去这一指令
						var2value[res] = tmp;
					}
					break;
				}
				case STORE: {	//一个use
					if (var2value.find(res) != var2value.end()) {
						instr.setResult(var2value[res]);
					}
					if (isNumber(instr.getResult()) && !isGlobal(ope1)) {	//设置被赋值的变量
						var2value[ope1] = instr.getResult();
					}
					break;
				}
				case STOREARR: {	//两个use
					if (var2value.find(res) != var2value.end()) {
						instr.setResult(var2value[res]);
					}
					if (var2value.find(ope2) != var2value.end()) {
						instr.setOperand2(var2value[ope2]);
					}
					break;
				}
				case LOAD: {	//一开始的值是全局变量的初始值？怎么处理，标记一下，只能用一次？
					if (ope2 != "para" && ope2 != "array") {
						if (var2value.find(ope1) != var2value.end()) {
							var2value[res] = var2value[ope1];
						}
					}
					break;
				}
				case LOADARR: {	//数组常量的值的加载这么处理？同样只能用第一次？
					if (var2value.find(ope2) != var2value.end()) {
						instr.setOperand2(var2value[ope2]);
					}
					break;
				}
				case RET: case PUSH: {
					if (var2value.find(ope1) != var2value.end()) {
						instr.setOperand1(var2value[ope1]);
					}
					break;
				}
				case BR: {
					if (var2value.find(ope1) != var2value.end()) {
						instr.setOperand1(var2value[ope1]);
						if (A2I(var2value[ope1]) == 0) {
							instr.setInstr(res, instr.getResult(), "");	//如果结果==0，就直接跳转到res对应的标签
						}
						else {
							instr.setInstr(res, instr.getOperand2(), "");	//如果结果!=0，直接跳转到ope2对应的标签
						}
					}
					break;
				}
				default:
					break;
				}
			}
		}
	}
}

//存放相等变量的对应关系
map<string, string> var2copy;

//将对应到变量var的关系删除，当变量被重新赋值后使用这一函数删除之前的联系！
void releaseCopy(string var) {
	for (auto it = var2copy.begin(); it != var2copy.end();) {
		if (it->second == var) {
			it = var2copy.erase(it);
			continue;
		}
		it++;
	}
}

void SSA::copy_propagation() {
	for (int i = 1; i < blockCore.size(); i++) {	//遍历每个函数
		auto& blocks = blockCore.at(i);
		for (int j = 0; j < blocks.size(); j++) {	//遍历每个基本的ir
			auto& ir = blocks.at(j).Ir;
			var2copy.clear();
			//常量替换+常量折叠
			for (int k = 0; k < ir.size(); k++) {
				auto& instr = ir.at(k);
				auto op = instr.getCodetype();
				auto res = instr.getResult();
				auto ope1 = instr.getOperand1();
				auto ope2 = instr.getOperand2();
				switch (op)
				{
				case STORE: {
					if (isTmp(res)) {	//变量赋值，非立即数赋值
						if (var2copy.find(res) != var2copy.end()) {	//如果赋值的中间变量对应某个变量，那么被赋值的变量也对应那个变量
							var2copy[ope1] = var2copy[res];
						}
						releaseCopy(ope1);
					}
					break;
				}
				case LOAD: {
					if (ope2 != "para" && ope2 != "array") {	//加载变量的值
						if (var2copy.find(ope1) != var2copy.end()) {	//变量ope1已经有了对应关系
							var2copy[res] = var2copy[ope1];
							instr.setOperand1(var2copy[ope1]);	//将ope1设置为ope1对应的变量
						}
						else {
							//创建新的变量对应关系
							var2copy[res] = ope1;
						}
					}
					break;
				}
				default:
					break;
				}
			}
		}
	}
}

string calculate(irCodeType op, string ope1, string ope2) {
	if (op == ADD) {
		return I2A(A2I(ope1) + A2I(ope2));
	}
	else if (op == SUB) {
		return I2A(A2I(ope1) - A2I(ope2));
	}
	else if (op == MUL) {
		return I2A(A2I(ope1) * A2I(ope2));
	}
	else if (op == DIV) {
		return I2A(A2I(ope1) / A2I(ope2));
	}
	else if (op == REM) {
		return I2A(A2I(ope1) % A2I(ope2));
	}
	else if (op == AND) {
		int res = A2I(ope1) && A2I(ope2);
		return I2A(res);
	}
	else if (op == OR) {
		int res = A2I(ope1) || A2I(ope2);
		return I2A(res);
	}
	else if (op == NOT) {
		int res = !A2I(ope1);
		return I2A(res);
	}
	else if (op == EQL) {
		int res = A2I(ope1) == A2I(ope2);
		return I2A(res);
	}
	else if (op == NEQ) {
		int res = A2I(ope1) != A2I(ope2);
		return I2A(res);
	}
	else if (op == SLT) {
		int res = A2I(ope1) < A2I(ope2);
		return I2A(res);
	}
	else if (op == SLE) {
		int res = A2I(ope1) <= A2I(ope2);
		return I2A(res);
	}
	else if (op == SGT) {
		int res = A2I(ope1) > A2I(ope2);
		return I2A(res);
	}
	else if (op == SGE) {
		int res = A2I(ope1) >= A2I(ope2);
		return I2A(res);
	}
	else {
		WARN_MSG("problem in calculate!!");
	}
	return "wrong";
}

//============================================================
// 循环优化：不变式外提、强度削弱、归纳变量删除
//============================================================

class Circle {
public:
	set<int> cir_blks;	//循环的基本块结点
	set<int> cir_outs;	//循环的退出结点
	int cir_begin;
	Circle() {}
	Circle(set<int>& blks) : cir_blks(blks) {}
};

//每个函数的循环，已连续的基本块构成；
vector<vector<Circle>> func2circles;

void printCircle() {
	if (TIJIAO) {
		int i = 0;
		for (auto circles : func2circles) {
			if (circles.empty()) {
				continue;
			}
			cout << "function_" << i << endl;
			i++;
			for (auto circle : circles) {
				cout << "circle's blocks: { ";
				for (auto blk : circle.cir_blks) {
					cout << "B" << blk << " ";
				}
				cout << "}	Quit block in circle: { ";
				for (auto blk : circle.cir_outs) {
					cout << "B" << blk << " ";
				}
				cout << "}  ";
				cout << "Begin block of circle: { B" << circle.cir_begin << " }" << endl;
			}
			cout << endl;
		}
		cout << endl;
	}
}

//存放每个函数的使用-定义链
vector<UDchain> func2udChains;

//计算每个函数的使用-定义链
void SSA::count_UDchains() {
	func2udChains.push_back(UDchain());
	for (int i = 1; i < blockCore.size(); i++) {
		UDchain tmp(blockCore.at(i));
		func2udChains.push_back(tmp);
	}
	ofstream ud("udchain.txt");
	for (int i = 1; i < func2udChains.size(); i++) {
		ud << "function_" << i << endl;
		func2udChains.at(i).printUDchain(ud);
		ud << endl;
	}
	ud.close();
}


void SSA::back_edge() {
	func2circles.push_back(vector<Circle>());
	for (int i = 1; i < blockCore.size(); i++) {
		func2circles.push_back(vector<Circle>());
		//每一个函数的基本块
		auto blocks = blockCore.at(i);
		vector<pair<int, int>> backEdges;		//存储找到的回边
		for (auto blk : blocks) {
			int num = blk.number;
			auto succs = blk.succeeds;
			auto doms = blk.domin;
			for (auto suc : succs) {
				//后继结点是必经节点说明是一条回边
				if (doms.find(suc) != doms.end()) {
					backEdges.push_back(pair<int, int>(num, suc));
					if (suc > num) {
						WARN_MSG("wrong in back_edge find!");
					}
				}
			}
		}
		
		//查找循环的基本块，每个回边对应着一个循环，后续应该合并某些循环，不合并效率可能比较低？
		for (auto backEdge : backEdges) {
			Circle circle;
			int end = backEdge.first;
			int begin = backEdge.second;
			circle.cir_blks.insert(end);
			circle.cir_blks.insert(begin);
			circle.cir_begin = begin;
			queue<int> work;
			work.push(end);
			while (!work.empty()) {
				int tmp = work.front();
				work.pop();
				auto preds = blocks.at(tmp).pred;
				for (auto pred : preds) {
					if (pred != begin && circle.cir_blks.find(pred) == circle.cir_blks.end()) {
						work.push(pred);
						circle.cir_blks.insert(pred);
					}
				}
			}
			for (auto one : circle.cir_blks) {
				auto succs = blocks.at(one).succeeds;
				for (auto suc : succs) {
					if (circle.cir_blks.find(suc) == circle.cir_blks.end()) {
						circle.cir_outs.insert(one);
					}
				}
			}
			func2circles.at(i).push_back(circle);
		}
	}
	printCircle();
}

//================================================
//达到定义链和 使用定义链
//================================================

void SSA::mark_invariant() {
	for (int i = 1; i < blockCore.size(); i++) {
		auto& blocks = blockCore.at(i);
		auto& udchain = func2udChains.at(i);
		try {
			//处理函数内部的所有循环
			for (auto circle : func2circles.at(i)) {
				//第一遍标记运算对象为常数和定值点在循环外的
				for (auto idx : circle.cir_blks) {
					auto& ir = blocks.at(idx).Ir;
					for (int j = 0; j < ir.size(); j++) { //这里判断每条指令的操作数是否为常数或者定值在循环外面
						auto& instr = ir.at(j);
						auto op = instr.getCodetype();
						auto res = instr.getResult();
						auto ope1 = instr.getOperand1();
						auto ope2 = instr.getOperand2();
						switch (op) {
						case LOAD: {
							if (ope2 == "para" || ope2 == "array") {	//取数组地址，一定是不变式
								instr.setInvariant();
							}
							else {	//取变量的值，看变量的定义位置def是否在循环外
								if (!isGlobal(ope1)) {
									auto def = udchain.getDef(Node(idx, j, ope1), ope1);
									if (def.var != "" && circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
										instr.setInvariant();
									}
								}
							}
							break; } 
						case STORE: {	//先不考虑全局变量和局部变量的区别
							if (isNumber(res)) {	//赋值是常数，直接设置为不变式
								instr.setInvariant();
							}
							else {	//赋值为变量，查看变量的定义
								auto def = udchain.getDef(Node(idx, j, res), res);
								if (def.var != "" && circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							break; }
						case ADD: case SUB: case DIV: case MUL: case REM:
						case AND: case OR: case NOT: case EQL:
						case NEQ: case SGT: case SGE: case SLT: case SLE: {
							if (isNumber(ope1)) {
								auto def = udchain.getDef(Node(idx, j, ope2), ope2);
								if (def.var != "" && circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							else if (isNumber(ope2)) {
								auto def = udchain.getDef(Node(idx, j, ope1), ope1);
								if (def.var != "" && circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							else {	//操作数全是变量		//如果是全局变量的use那么没有定义怎么办？
								auto def1 = udchain.getDef(Node(idx, j, ope1), ope1);
								auto def2 = udchain.getDef(Node(idx, j, ope2), ope2);
								if (def1.var != "" && def2.var != "" 
									&& circle.cir_blks.find(def1.bIdx) == circle.cir_blks.end() 
									&& circle.cir_blks.find(def2.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							break; }
						case STOREARR: case LOADARR: {
							if (!isNumber(ope2)) {
								auto def = udchain.getDef(Node(idx, j, ope2), ope2);
								if (def.var != "" && circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							break;
						}
						case RET: case PUSH: case BR: {
							if (isTmp(ope1)) {
								auto def = udchain.getDef(Node(idx, j, ope1), ope1);
								if (def.var != "" && circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							break; }
						default:
							break;
						}
					}
				}
				//处理第一遍未标记的代码
				for (auto idx : circle.cir_blks) {
					auto& ir = blocks.at(idx).Ir;
					for (int j = 0; j < ir.size(); j++) { //这里判断每条指令的操作数是否为常数或者定值在循环外面
						auto& instr = ir.at(j);
						if (instr.getInvariant() == 1) {
							continue;
						}
						auto op = instr.getCodetype();
						auto res = instr.getResult();
						auto ope1 = instr.getOperand1();
						auto ope2 = instr.getOperand2();
						switch (op) {
						case LOAD: {
							//取变量的值，看变量的定义位置def是否在循环外
							if (!isGlobal(ope1)) {	//不是全局变量
								auto def = udchain.getDef(Node(idx, j, ope1), ope1);
								if (def.var != "" && circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
								else {
									if (blocks.at(def.bIdx).Ir.at(def.lIdx).getInvariant() == 1) {	//定值点被标记了
										instr.setInvariant();
									}
								}
							}
							break; }
						case STORE: {	//先不考虑全局变量和局部变量的区别
							//赋值为变量，查看变量的定义
							auto def = udchain.getDef(Node(idx, j, res), res);
							if (def.var != "" && circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
								instr.setInvariant();
							}
							else {
								if (blocks.at(def.bIdx).Ir.at(def.lIdx).getInvariant() == 1) {	//定值点被标记了
									instr.setInvariant();
								}
							}
							break; }
						case ADD: case SUB: case DIV: case MUL: case REM:
						case AND: case OR: case NOT: case EQL:
						case NEQ: case SGT: case SGE: case SLT: case SLE: {
							if (isNumber(ope1)) {
								auto def = udchain.getDef(Node(idx, j, ope2), ope2);
								if (def.var != "" && circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
								else {
									if (blocks.at(def.bIdx).Ir.at(def.lIdx).getInvariant() == 1) {	//定值点被标记了
										instr.setInvariant();
									}
								}
							}
							else if (isNumber(ope2)) {
								auto def = udchain.getDef(Node(idx, j, ope1), ope1);
								if (def.var != "" && circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
								else {
									if (blocks.at(def.bIdx).Ir.at(def.lIdx).getInvariant() == 1) {	//定值点被标记了
										instr.setInvariant();
									}
								}
							}
							else {	//操作数全是变量
								auto def1 = udchain.getDef(Node(idx, j, ope1), ope1);
								auto def2 = udchain.getDef(Node(idx, j, ope2), ope2);
								if (def1.var != "" && def2.var != "" ) {	//定义都有意义
									if (circle.cir_blks.find(def1.bIdx) == circle.cir_blks.end() 
										&& blocks.at(def2.bIdx).Ir.at(def2.lIdx).getInvariant() == 1) {	//ope1定义在循环外，ope2定义是不变量
										instr.setInvariant();
									}
									else if (circle.cir_blks.find(def2.bIdx) == circle.cir_blks.end()
										&& blocks.at(def1.bIdx).Ir.at(def1.lIdx).getInvariant() == 1) {	//ope2定义在循环外，ope1定义是不变量
										instr.setInvariant();
									}
									else if (blocks.at(def1.bIdx).Ir.at(def1.lIdx).getInvariant() == 1 
										&& blocks.at(def2.bIdx).Ir.at(def2.lIdx).getInvariant() == 1) {	//ope1和ope2定义均是不变量
										instr.setInvariant();
									}
								}
							}
							break; }
						case STOREARR: case LOADARR: {
							if (!isNumber(ope2)) {
								auto def = udchain.getDef(Node(idx, j, ope2), ope2);
								if (def.var != "" && circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
								else {
									if (blocks.at(def.bIdx).Ir.at(def.lIdx).getInvariant() == 1) {	//定值点被标记了
										instr.setInvariant();
									}
								}
							}
						}
						case RET: case PUSH: case BR: {
							if (isTmp(ope1)) {
								auto def = udchain.getDef(Node(idx, j, ope1), ope1);
								if (def.var != "" && circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
								else {
									if (blocks.at(def.bIdx).Ir.at(def.lIdx).getInvariant() == 1) {	//定值点被标记了
										instr.setInvariant();
									}
								}
							}
							break; }
						default:
							break;
						}
					}
				}
			}
		}
		catch (exception e) {
			WARN_MSG("wrong in mark invariant! maybe the circle find is wrong!");
		}
	}
}

//遍历每一条指令s: A = B op C | A = B
//a)
//循环不变式外提条件1：所在节点是所有出口结点的必经结点
bool SSA::condition1(set<int> outBlk, int instrBlk, int func) {
	for (auto one : outBlk) {
		if (blockCore.at(func).at(one).domin.find(instrBlk) == blockCore.at(func).at(one).domin.end()) {
			return false;
		}
	}
	return true;
}
//循环不变式外提条件2：循环中没有A的其他定制语句，SSA格式天然满足？？？
//循环不变式外提条件3：循环对于A的引用，只有s对于A的定值能够到达，SSA格式天然满足？？？

//b)
//循环不变式外提条件1：变量离开循环后不活跃
bool SSA::condition2(set<int> outBlk, string var, int func) {
	for (auto one : outBlk) {
		if (blockCore.at(func).at(one).out.find(var) != blockCore.at(func).at(one).out.end()) {
			return false;
		}
	}
	return true;
}

//循环不变式外提条件2：循环中没有A的其他定制语句，SSA格式天然满足？？？
//循环不变式外提条件3：循环对于A的引用，只有s对于A的定值能够到达，SSA格式天然满足？？？


void SSA::code_outside() {
	//外提代码
	for (int i = 1; i < blockCore.size(); i++) {
		auto& blocks = blockCore.at(i);
		try {
			//处理函数内部的所有循环
			for (auto circle : func2circles.at(i)) {
				//第一遍标记运算对象为常数和定值点在循环外的
				
				for (auto idx : circle.cir_blks) {
					auto& ir = blocks.at(idx).Ir;
					int pos = 0;
					for (int j = 0; j < ir.size(); j++) { //这里判断每条指令的操作数是否为常数或者定值在循环外面
						auto& instr = ir.at(j);
						if (instr.getInvariant() == 1) {	//不变式代码
							//外提到循环开始基本块的label前面
							if (condition1(circle.cir_outs, idx, i) || condition2(circle.cir_outs, instr.getResult(), i)) {
								auto tmp = instr;
								instr.setCodeOut();
								blocks.at(circle.cir_begin).Ir.insert(blocks.at(circle.cir_begin).Ir.begin() + pos++, tmp);
								j++;
							}
						}
					}
				}
			}
		}
		catch (exception e) {
			WARN_MSG("code outside wrong!!");
		}
	}
	//删除标记为outcode的代码
	for (int i = 1; i < blockCore.size(); i++) {
		auto& blocks = blockCore.at(i);
		try {
			for (auto& blk : blocks) {
				auto& ir = blk.Ir;
				for (auto it = ir.begin(); it != ir.end();) {
					if (it->getCodeOut() == 1) {	//被外提的代码，删除
						it = ir.erase(it);
						continue;
					}
					it++;
				}
			}
		}
		catch (exception e) {
			WARN_MSG("delete code out wrong!!");
		}
	}
}


//void SSA::strength_reduction()
//{
//}
//
//void SSA::protocol_variable_deletion()
//{
//}
