#include "ssa.h"
#include "../front/symboltable.h"
#include "../front/syntax.h"
#include "../util/meow.h"
#include <queue>
#include <unordered_map>

//==========================================================

// 在睿轩生成的中间代码做优化
void SSA::pre_optimize() {
	// 简化条件判断为常值的跳转指令
	simplify_br();
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
	//build_def_use_chain();
	// 重新进行活跃变量分析
	//active_var_analyse();
	// 死代码删除
	//delete_dead_codes();
	// 函数内联
	//judge_inline_function();
	//inline_function();

	//循环优化
	//back_edge();

	count_use_def_chain();
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

//每个函数的循环，已连续的基本块构成；
vector<vector<set<int>>> circles;

void SSA::back_edge() {
	for (int i = 1; i < blockCore.size(); i++) {
		//每一个函数的基本块
		auto blocks = blockCore.at(i);
		//存储找到的回边
		vector<pair<int, int>> backEdges;
		for (auto blk : blocks) {
			int num = blk.number;
			auto succs = blk.succeeds;
			auto doms = blk.domin;
			for (auto suc : succs) {
				//后继结点是必经节点说明是一条回边
				if (doms.find(suc) != doms.end()) {
					backEdges.push_back(pair<int, int>(num, suc));
					if (suc > num) {
						WARN_MSG("出了问题！");
					}
				}
			}
		}

		vector<set<int>> xunhuan;
		//查找循环的基本块
		for (auto backEdge : backEdges) {
			set<int> one;
			int end = backEdge.first;
			int begin = backEdge.second;
			one.insert(end);
			one.insert(begin);

			queue<int> work;
			work.push(end);
			while (!work.empty()) {
				int tmp = work.front();
				work.pop();
				auto preds = blocks.at(tmp).pred;
				for (auto pred : preds) {
					if (pred != begin && one.find(pred) == one.end()) {
						work.push(pred);
						one.insert(pred);
					}
				}
			}
			xunhuan.push_back(one);
		}
		circles.push_back(xunhuan);
	}
}

//================================================
//达到定义链和 使用定义链
//================================================

namespace ly {
	using Defines = vector<pair<int, int>>;
	using Uses = vector<pair<int, int>>;
	using Use = pair<int, int>;
	using Define = pair<int, int>;
	using In = vector<pair<int, int>>;
	using Out = vector<pair<int, int>>;
	using Pos = pair<int, int>;
}

using namespace ly;

void addDef(map<string, Defines>& container, string var, Define def) {
	if (container.find(var) != container.end()) {
		container[var].push_back(def);
	}
	else {
		container[var] = Defines();
		container[var].push_back(def);
	}
}

void addUse(map<string, Uses>& container, string var, Use use) {
	if (container.find(var) != container.end()) {
		container[var].push_back(use);
	}
	else {
		container[var] = Defines();
		container[var].push_back(use);
	}
}		

struct Block {
	map<Pos, string> pos2Def;			//每一条指令只会给一个变量定值。有位置<block, index> -> Def_var
	map<string, Defines> var2defs;
	map<string, Uses> var2uses;
	map<string, Define> in;	//记录变量在基本块开始位置的——定值
	map<string, Define> ou;	//记录变量在基本块结束位置的——定值
	map<string, Define> kill;	//记录基本块的kill定值
	map<string, Define> gen;	//记录基本块的生成定值
	set<string> genVar;			//记录基本块生成的变量名
};

vector<Block> blk;

void printBlk() {
	if (TIJIAO) {
		int i = 0;
		for (auto one : blk) {
			string vars = FORMAT("B{} gen: ", i);
			for (auto tmp : one.genVar) {
				vars += tmp + "  ";
			}
			i++;
			cout << vars << endl;
		}
		cout << endl;
	}
}

//计算每个基本块的gen，kill使用另外的方法处理
void count_gen_of_block(vector<basicBlock>& blocks) {
	for (int j = 0; j < blocks.size(); j++) {
		blk.push_back(Block());
		for (int k = 0; k < blocks.at(j).Ir.size(); k++) {
			auto instr = blocks.at(j).Ir.at(k);
			auto res = instr.getResult();
			auto ope1 = instr.getOperand1();
			auto ope2 = instr.getOperand2();
			switch (instr.getCodetype())
			{
			case ADD: case SUB: case MUL: case DIV: case REM:
			case AND: case OR:
			case EQL: case NEQ: case SLT: case SLE: case SGT: case SGE: {
				addDef(blk[j].var2defs, res, Define(j, k));	
				addUse(blk[j].var2uses, ope1, Use(j, k));
				addUse(blk[j].var2uses, ope2, Use(j, k));
				blk[j].gen[res] = Define(j, k); blk[j].genVar.insert(res);
				break;
			} case NOT: {
				addDef(blk[j].var2defs, res, Define(j, k));	
				addUse(blk[j].var2uses, ope1, Use(j, k));
				blk[j].gen[res] = Define(j, k); blk[j].genVar.insert(res);
				break;
			} case LOAD: {	//load的变量使用？
				if (ope2 != "array" && ope2 != "para") {
					addUse(blk[j].var2uses, ope1, Use(j, k));
				}
				addDef(blk[j].var2defs, res, Define(j, k));
				blk[j].gen[res] = Define(j, k); blk[j].genVar.insert(res);
				break;
			} case STORE: {
				addUse(blk[j].var2uses, res, Use(j, k));
				addDef(blk[j].var2defs, ope1, Define(j, k));
				blk[j].gen[ope1] = Define(j, k); blk[j].genVar.insert(ope1);
				break;
			} case LOADARR: {
				if (!isNumber(ope2)) {
					addUse(blk[j].var2uses, ope2, Use(j, k));
				}
				addDef(blk[j].var2defs, res, Define(j, k));
				blk[j].gen[res] = Define(j, k); blk[j].genVar.insert(res);
				break;
			}case STOREARR: {
				if (!isNumber(ope2)) {
					addUse(blk[j].var2uses, ope2, Use(j, k));
				}
				addUse(blk[j].var2uses, res, Use(j, k));
				break;
			}case PUSH: {
				addUse(blk[j].var2uses, ope1, Use(j, k));
				break;
			}case BR: {
				if (isTmp(ope1)) {
					addUse(blk[j].var2uses, ope1, Use(j, k));
				}
				break;
			}case CALL: {
				if (isTmp(res)) {
					addDef(blk[j].var2defs, res, Define(j, k));
					blk[j].gen[res] = Define(j, k); blk[j].genVar.insert(res);
				}
				break;
			}case RET: {
				if (isTmp(ope1)) {
					addUse(blk[j].var2uses, ope1, Use(j, k));
				}
				break;
			}
			default:
				break;
			}
		}
	}
}

void SSA::count_use_def_chain() {
	for (int i = 1; i < blockCore.size(); i++) {
		auto blocks = blockCore.at(i);
		count_gen_of_block(blocks);
		printBlk();
		for (int j = 0; j < blocks.size(); j++) {
			auto block = blocks.at(j);

			while (true) {


				if () { //如果in out集不在发生变化
					break;
				}
			}
		}
		//计算到达-定义的in out	
	}
}

void SSA::mark_invariant() {
	/*for (int i = 1; i < blockCore.size(); i++) {
		auto FuncCircle = circles.at(i-1);
		for (auto circle : FuncCircle) {
			int mark = 1;
			while (mark) {
				for () {

				}
			}
		}
	}*/
}

void SSA::code_outside() {
	//计算不变式

	//外提代码
}

void SSA::strength_reduction()
{
}

void SSA::protocol_variable_deletion()
{
}
