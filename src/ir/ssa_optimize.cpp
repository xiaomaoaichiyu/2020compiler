#include "ssa.h"
#include "../front/symboltable.h"
#include "../front/syntax.h"
#include "../util/meow.h"
#include "../ir/dataAnalyse.h"

#include <queue>
#include <unordered_map>

//==========================================================

// 在睿轩生成的中间代码做优化
void SSA::pre_optimize() {
	// 简化load和store指令相邻的指令
	load_and_store();
	// 简化加减0、乘除模1这样的指令
	simplify_add_minus_multi_div_mod();
	// 简化紧邻的跳转
	simplify_br_label();
}

//ssa形式上的优化
void SSA::ssa_optimize() {
	// 重新计算use-def关系
	build_def_use_chain();
	// 重新进行活跃变量分析
	active_var_analyse();
	// 死代码删除
	delete_dead_codes();
	// 重新计算use-def关系
	build_def_use_chain();
	// 重新进行活跃变量分析
	active_var_analyse();
	// 函数内联
	//judge_inline_function();
	//inline_function();

	// 将phi函数加入到中间代码
	add_phi_to_Ir();

	count_UDchains();
	//循环优化
	back_edge();
	mark_invariant();
	
	// 删除中间代码中的phi
	delete_Ir_phi();

	//count_use_def_chain();
}

//========================================================

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

//============================================================
// 循环优化：不变式外提、强度削弱、归纳变量删除
//============================================================

class Circle {
public:
	set<int> cir_blks;	//循环的基本块结点
	set<int> cir_outs;	//循环的退出结点
	Circle(set<int>& blks) : cir_blks(blks) {}
};

//每个函数的循环，已连续的基本块构成；
vector<Circle> circles;

void printCircle() {
	if (TIJIAO) {
		for (auto circle : circles) {
			cout << "circle's blocks: { ";
			for (auto blk : circle.cir_blks) {
				cout << "B" << blk << " ";
			}
			cout << "}	Quit block in circle: { ";
			for (auto blk : circle.cir_outs) {
				cout << "B" << blk << " ";
			}
			cout << "}" << endl;
		}
		cout << endl;
	}
}

vector<UDchain> func2udChains;

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
	for (int i = 1; i < blockCore.size(); i++) {
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

		set<int> circle;
		set<int> cir_out;
		//查找循环的基本块，每个回边对应着一个循环，后续应该合并某些循环，不合并效率可能比较低？
		for (auto backEdge : backEdges) {
			circle.clear();
			int end = backEdge.first;
			int begin = backEdge.second;
			circle.insert(end);
			circle.insert(begin);

			queue<int> work;
			work.push(end);
			while (!work.empty()) {
				int tmp = work.front();
				work.pop();
				auto preds = blocks.at(tmp).pred;
				for (auto pred : preds) {
					if (pred != begin && circle.find(pred) == circle.end()) {
						work.push(pred);
						circle.insert(pred);
					}
				}
			}
			cir_out.clear();
			for (auto one : circle) {
				auto succs = blocks.at(one).succeeds;
				for (auto suc : succs) {
					if (circle.find(suc) == circle.end()) {
						cir_out.insert(one);
					}
				}
			}
			Circle tmp(circle);
			tmp.cir_outs = cir_out;
			circles.push_back(tmp);
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
			for (auto circle : circles) {
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
								auto def = udchain.getDef(Node(idx, j, ope1), ope1);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							break; } 
						case STORE: {	//先不考虑全局变量和局部变量的区别
							if (isNumber(res)) {	//赋值是常数，直接设置为不变式
								instr.setInvariant();
							}
							else {	//赋值为变量，查看变量的定义
								auto def = udchain.getDef(Node(idx, j, res), res);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							break; }
						case ADD: case SUB: case DIV: case MUL: case REM:
						case AND: case OR: case NOT: case EQL:
						case NEQ: case SGT: case SGE: case SLT: case SLE: {
							if (isNumber(ope1)) {
								auto def = udchain.getDef(Node(idx, j, ope2), ope2);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							else if (isNumber(ope2)) {
								auto def = udchain.getDef(Node(idx, j, ope1), ope1);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							else {	//操作数全是变量
								auto def = udchain.getDef(Node(idx, j, ope1), ope1);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
								def = udchain.getDef(Node(idx, j, ope2), ope2);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							break; }
						case STOREARR: case LOADARR: {
							if (!isNumber(ope2)) {
								auto def = udchain.getDef(Node(idx, j, ope2), ope2);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
						}
						case RET: case PUSH: case BR: {
							if (isTmp(ope1)) {
								auto def = udchain.getDef(Node(idx, j, ope1), ope1);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
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
								auto def = udchain.getDef(Node(idx, j, ope1), ope1);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							break; }
						case STORE: {	//先不考虑全局变量和局部变量的区别
							if (isNumber(res)) {	//赋值是常数，直接设置为不变式
								instr.setInvariant();
							}
							else {	//赋值为变量，查看变量的定义
								auto def = udchain.getDef(Node(idx, j, res), res);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							break; }
						case ADD: case SUB: case DIV: case MUL: case REM:
						case AND: case OR: case NOT: case EQL:
						case NEQ: case SGT: case SGE: case SLT: case SLE: {
							if (isNumber(ope1)) {
								auto def = udchain.getDef(Node(idx, j, ope2), ope2);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							else if (isNumber(ope2)) {
								auto def = udchain.getDef(Node(idx, j, ope1), ope1);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							else {	//操作数全是变量
								auto def = udchain.getDef(Node(idx, j, ope1), ope1);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
								def = udchain.getDef(Node(idx, j, ope2), ope2);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
							break; }
						case STOREARR: case LOADARR: {
							if (!isNumber(ope2)) {
								auto def = udchain.getDef(Node(idx, j, ope2), ope2);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
								}
							}
						}
						case RET: case PUSH: case BR: {
							if (isTmp(ope1)) {
								auto def = udchain.getDef(Node(idx, j, ope1), ope1);
								if (circle.cir_blks.find(def.bIdx) == circle.cir_blks.end()) {
									instr.setInvariant();
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
	for (int i = 1; i < blockCore.size(); i++) {
		for (int j = 0; j < blockCore.at(i).size(); j++) {
			auto ir = blockCore.at(i).at(j).Ir;
			for (auto instr : ir) {
				cout << instr.getContent() << endl;
			}
		}
	}
}
//
//void SSA::code_outside() {
//	//计算不变式
//
//	//外提代码
//}
//
//void SSA::strength_reduction()
//{
//}
//
//void SSA::protocol_variable_deletion()
//{
//}
