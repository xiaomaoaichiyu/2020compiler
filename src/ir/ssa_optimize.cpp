﻿#include "ssa.h"
#include "../front/symboltable.h"
#include "../front/syntax.h"
#include "../util/meow.h"
#include "../ir/dataAnalyse.h"

#include <queue>
#include <unordered_map>
#include <algorithm>
#include <regex>
#include <iterator>

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
	simpify_duplicate_assign();
}
void SSA::simpify_duplicate_assign() {
	int i, j, k;
	int size1 = codetotal.size();
	for (i = 1; i < size1; i++) {
		for (j = 0; j < codetotal[i].size() - 1; j++) {
			CodeItem ci1 = codetotal[i][j];
			CodeItem ci2 = codetotal[i][j + 1];
			if (ci1.getCodetype() == ci2.getCodetype()
				&& ci1.getResult().compare(ci2.getResult()) == 0
				&& ci1.getOperand1().compare(ci2.getOperand1()) == 0
				&& ci1.getOperand2().compare(ci2.getOperand2()) == 0) {
				codetotal[i].erase(codetotal[i].begin() + j);
				j--;
			}
		}
	}
}

ActiveAnalyse ly_act;

void SSA::get_avtiveAnalyse_result() {
	ly_act.func2in.push_back(map<int, set<string>>());
	ly_act.func2out.push_back(map<int, set<string>>());
	ly_act.func2def.push_back(map<int, set<string>>());
	ly_act.func2use.push_back(map<int, set<string>>());
	for (int i = 1; i < blockCore.size(); i++) {
		auto blocks = blockCore.at(i);
		map<int, set<string>> inTmp;
		map<int, set<string>> outTmp;
		map<int, set<string>> defTmp;
		map<int, set<string>> useTmp;
		for (auto blk : blocks) {
			inTmp[blk.number] = blk.in;
			outTmp[blk.number] = blk.out;
			defTmp[blk.number] = blk.def;
			useTmp[blk.number] = blk.use;
		}
		ly_act.func2in.push_back(inTmp);
		ly_act.func2out.push_back(outTmp);
		ly_act.func2def.push_back(defTmp);
		ly_act.func2use.push_back(useTmp);
	}
}

void ActiveAnalyse::print_ly_act() {
	if (TIJIAO) {
		ofstream ly_out("activeAnalyse.txt");
		for (int i = 1; i < ly_act.func2in.size(); i++) {
			ly_out << "function_" << i << endl;
			auto inTmp = ly_act.func2in.at(i);
			auto outTmp = ly_act.func2out.at(i);
			auto defTmp = ly_act.func2def.at(i);
			auto useTmp = ly_act.func2use.at(i);
			int size = inTmp.size();
			for (int j = 0; j < size; j++) {
				ly_out << "B" << j << ":\n\tin:  {";
				for (auto var : inTmp[j]) {
					ly_out << var << " ";
				}
				ly_out << "}\n\tout: {";
				for (auto var : outTmp[j]) {
					ly_out << var << " ";
				}
				ly_out << "}\n\tdef: {";
				for (auto var : defTmp[j]) {
					ly_out << var << " ";
				}
				ly_out << "}\n\tuse: {";
				for (auto var : useTmp[j]) {
					ly_out << var << " ";
				}
				ly_out << "}\n";
			}
			ly_out << endl << endl;
		}
	}
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

	if (0) { // 关闭循环优化
	// 将phi函数加入到中间代码
		add_phi_to_Ir();

		//循环优化
		count_UDchains();		//计算使用-定义链
		back_edge();			//循环优化

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
	for (i = 1; i < size1; i++) {	// 遍历函数
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

string SSA::getNewInsertAddr(int funNum, string name) {
	inlineArrayName[funNum].insert(name);
	return name;
} 

string SSA::getNewInsertLabel(int funNum, string name) {
	string ans = name + "." + to_string(labelIndex[funNum]);
	labelIndex[funNum]++;
	return ans;
}

string SSA::getNewFunEndLabel(int funNum, string name) {
	string ans = "%function." + name.substr(1) + ".end" + to_string(funCallCount[funNum]);
	funCallCount[funNum]++;
	return ans;
}

string SSA::getFunEndLabel(int funNum, string name) {
	return ("%function." + name.substr(1) + ".end" + to_string(funCallCount[funNum]));
}

string SSA::getRetValName(string name) {
	return ("%Ret*" + name.substr(1));
}

// 函数内联
void SSA::inline_function() {
	int i, j;
	int size1 = blockCore.size();
	// init & 建立函数名与对应序号的对应关系
	for (i = 0; i < size1; i++) {
		addrIndex.push_back(0);
		labelIndex.push_back(0);
		set<string> s; inlineArrayName.push_back(s);
		funCallCount.push_back(0);
		if (i == 0) continue;
		string funName = codetotal[i][0].getResult();	// define
		funName2Num[funName] = i;
		funNum2Name[i] = funName;
	}
	// 函数内联主体部分
	for (i = 1; i < size1; i++) {	// 遍历函数
		vector<CodeItem> newInsertAllocIr;	// 要在该函数开始添加的alloc中间代码
		set<string> newInsertAllocName;		// 要在该函数开始添加的alloc变量名
		vector<symbolTable> newInsertSymbolTable;	// 要在该函数符号表中插入的
		bool step1 = true, step2 = true, step3 = true, step4 = true, step5 = true, step6 = true, debug = false;	// debug
		/*
		 step1: 判断调用函数是否可以内联
		 step2: 处理参数及传参指令，alloc
		 step3. 插入调用函数的除函数和参数声明之外的指令
		 step4. 添加函数调用结束标签
		 step5. 删除call指令
		 step6. 添加alloc指令到函数开始位置，更新符号表
		 */
		for (j = 1; j < codetotal[i].size(); j++) {
			CodeItem ci = codetotal[i][j];
			/* step0: 找到函数调用 */
			if (ci.getCodetype() == NOTE && ci.getOperand1().compare("func") == 0 && ci.getOperand2().compare("begin") == 0
				&& funName2Num.find(ci.getResult()) != funName2Num.end()) {
				string funName = ci.getResult();				// 函数名
				int funNum = funName2Num[funName];	// 该函数对应的序号
				symbolTable funSt;									// 函数对应的符号表结构
				set<int> alreadyNeilian;				//丛睿轩添加的...	记录当前以内联的符号
				if (step1) {	/* step1: 判断调用函数是否可以内联 */
					for (int iter1 = 1; iter1 < total.size(); iter1++) {
						if (total[iter1][0].getName().compare(funName) == 0) {
							funSt = total[iter1][0];	// 正常情况下一定会有这一步
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
					//else if (alreadyNeilian.find(funNum)==alreadyNeilian.end() && total[funNum].size()+total[i].size()>10) {		//临时变量总数不超过8个就内联,否则不内敛(丛睿轩添加的...)  
					/*else if (alreadyNeilian.find(funNum) == alreadyNeilian.end() && codetotal[funNum].size()>200) {
						continue;		//第一个条件说明当前函数没被内联过
					}
					else if (codetotal[i].size() > 750) {		//丛睿轩添加的....
						continue;
					}*/
					else if (codetotal[i].size() + codetotal[funNum].size() > 800) {	// 内联后行数超过800
						continue;
					}
					else if (alreadyNeilian.find(funNum) == alreadyNeilian.end() && varName2St[i].size() + varName2St[funNum].size() > 10) {	// 内联后变量个数超过10个Orz
						continue;
					}
					else {
						if (debug) cout << "在函数 " << funNum2Name[i] << " 中内联函数 " << funName << endl;
					}
				}
				alreadyNeilian.insert(funNum);		//丛睿轩添加的
				if (debug) cout << "step1 done." << endl;
				int paraNum = funSt.getparaLength();	// 参数个数
				vector<string> paraTable;					// 参数列表
				if (step2) {	/* step2: 处理参数及传参指令 */
					for (int k = 1; k < total[funNum].size(); k++) {
						symbolTable st = total[funNum][k];
						if (st.getForm() == PARAMETER) {	// 从符号表中获得该函数下的所有参数名
							string paraName = st.getDimension() == 0 ? st.getName() : getNewInsertAddr(i, st.getName());
							if (newInsertAllocName.find(paraName) == newInsertAllocName.end()) {
								newInsertAllocName.insert(paraName);
								// 将要内联函数的形参全部转换成alloc指令
								CodeItem nci(ALLOC, paraName, "", "1");
								newInsertAllocIr.insert(newInsertAllocIr.begin(), nci);
								// 插入到符号表
								symbolTable nst(VARIABLE, INT, paraName, 0, 0);
								newInsertSymbolTable.insert(newInsertSymbolTable.begin() ,nst);
							}
							paraTable.push_back(paraName);
						}
						else break;
					}
					if (paraTable.size() != paraNum) {
						cout << "函数内联不应该发生的情况：参数表和符号表中函数对应的参数数量不一致. " << endl;
						break;
					}
					//cout << ci.getContent() << endl;
					CodeItem nci(ci.getCodetype(), ci.getResult(), "inline", ci.getOperand2());
					codetotal[i][j] = nci;
					j++;	// 跳过note指令，后面应跟传参指令
					for (int iter2 = paraNum; iter2 > 0; j++) {	// 中间代码倒序，而符号表顺序
						ci = codetotal[i][j];
						if (ci.getCodetype() == PUSH) {
							CodeItem nci(STORE, ci.getOperand1(), paraTable[iter2 - 1], "");
							codetotal[i][j] = nci;
							iter2--;
						}
						else { continue; }
					}
				}
				if (debug) cout << "step2 done." << endl;
				ci = codetotal[i][j];
				if (step3) {	/* step3. 插入调用函数的除函数和参数声明之外的指令 */
					map<string, string> label2NewLabel;	// 调用函数内部重命名后的标签名
					for (int k = 1; k <= paraNum; k++) {
						ci = codetotal[funNum][k];
						if (ci.getCodetype() != PARA || (ci.getOperand1().compare("int*") != 0 && ci.getResult().compare(paraTable[k - 1]) != 0)) {
							cout << "函数内联不应该发生的情况：函数定义的参数与参数表不一致. " << endl; break;
						}
					}
					for (int k = paraNum + 1; k < codetotal[funNum].size(); k++) {
						ci = codetotal[funNum][k];
						if (ci.getCodetype() == ALLOC) {	// 处理要内联函数中的alloc指令，对于一个函数内多次内联同一个函数的情况不要加重了
							string varName = ci.getResult();
							if (newInsertAllocName.find(varName) == newInsertAllocName.end()) {
								newInsertAllocName.insert(varName);
								newInsertAllocIr.push_back(ci);
								symbolTable nst;
								for (vector<symbolTable>::iterator iter = total[funNum].begin(); iter != total[funNum].end(); iter++)
									if (iter->getName().compare(varName) == 0) {
										nst = *iter; break;
									}
								if (nst.getName().compare(varName) == 0) {	// 插入到符号表
									newInsertSymbolTable.push_back(nst);
								}
								else {
									cout << "函数内联不应该发生的情况：被内联函数的局部变量在符号表中没有找到. " << endl; break;
								}
							}
						}
						else if (ci.getCodetype() == BR) {
							if (ifTempVariable(ci.getOperand1()) || ifVR(ci.getOperand1())) {	// 有条件跳转
								string newLabel1, newLabel2;
								if (label2NewLabel.find(ci.getResult()) == label2NewLabel.end()) {
									newLabel1 = getNewInsertLabel(i, ci.getResult());
									label2NewLabel[ci.getResult()] = newLabel1;
								}
								else {
									newLabel1 = label2NewLabel[ci.getResult()];
								}
								if (label2NewLabel.find(ci.getOperand2()) == label2NewLabel.end()) {
									newLabel2 = getNewInsertLabel(i, ci.getOperand2());
									label2NewLabel[ci.getOperand2()] = newLabel2;
								}
								else {
									newLabel2 = label2NewLabel[ci.getOperand2()];
								}
								CodeItem nci(BR, newLabel1, ci.getOperand1(), newLabel2);
								codetotal[i].insert(codetotal[i].begin() + j, nci);
								j++;
							}
							else {
								string newLabel;
								if (label2NewLabel.find(ci.getOperand1()) == label2NewLabel.end()) {
									newLabel = getNewInsertLabel(i, ci.getOperand1());
									label2NewLabel[ci.getOperand1()] = newLabel;
								}
								else {
									newLabel = label2NewLabel[ci.getOperand1()];
								}
								CodeItem nci(BR, ci.getResult(), newLabel, ci.getOperand2());
								codetotal[i].insert(codetotal[i].begin() + j, nci);
								j++;
							}
						}
						else if (ci.getCodetype() == LABEL) {
							string newLabel;
							if (label2NewLabel.find(ci.getResult()) == label2NewLabel.end()) {
								newLabel = getNewInsertLabel(i, ci.getResult());
								label2NewLabel[ci.getResult()] = newLabel;
							}
							else {
								newLabel = label2NewLabel[ci.getResult()];
							}
							CodeItem nci(LABEL, newLabel, "", "");
							codetotal[i].insert(codetotal[i].begin() + j, nci);
							j++;
						}
						else if (ci.getCodetype() == RET) {
							if (funSt.getValuetype() == INT && ci.getOperand2().compare("int") == 0) {	// 有返回值函数
								CodeItem nci(STORE, ci.getOperand1(), getRetValName(funName), "");
								codetotal[i].insert(codetotal[i].begin() + j, nci);
								j++;
							}
							else if (funSt.getValuetype() == VOID && ci.getOperand2().compare("void") == 0) {	// 无返回值函数
								
							}
							else { cout << "函数内联不应该发生的情况：返回值不合法的函数定义. " << endl; break; }
							CodeItem nci(BR, "", getFunEndLabel(funNum, funName), "");
							codetotal[i].insert(codetotal[i].begin() + j, nci);
							j++;
						}
						else {
							codetotal[i].insert(codetotal[i].begin() + j, ci);
							j++;
						}
					}
				}
				if (debug) cout << "step3 done." << endl;
				if (step4) {	/* step4.添加函数调用结束标签 */
					CodeItem nci(LABEL, getNewFunEndLabel(funNum, funName), "", "");
					codetotal[i].insert(codetotal[i].begin() + j, nci);
					j++;
				}
				if (debug) cout << "step4 done." << endl;
				if (step5) {	/* step5. 删除call指令 */
					ci = codetotal[i][j];
					if (ci.getCodetype() != CALL || ci.getOperand1().compare(funName) != 0) {
						cout << "函数内联不应该发生的情况：压完参数后没有跟call函数指令. " << endl; break;
					}
					if (funSt.getValuetype() == INT) {
						string retValName = getRetValName(funName);
						if (newInsertAllocName.find(retValName) == newInsertAllocName.end()) {
							newInsertAllocName.insert(retValName);
							CodeItem nci(ALLOC, retValName, "", "1");
							newInsertAllocIr.push_back(nci);
							symbolTable nst(VARIABLE, INT, retValName, 0, 0);
							newInsertSymbolTable.push_back(nst);
						}
						if (ifTempVariable(ci.getResult())) {
							CodeItem nci(LOAD, ci.getResult(), getRetValName(funName), "");
							codetotal[i][j] = nci;
						}
						else { cout << "函数内联不应该发生的情况：返回值定义和调用不统一. " << endl; break; }
					}
					else {
						codetotal[i].erase(codetotal[i].begin() + j);
						j--;
					}
				}
				if (debug) cout << "step5 done." << endl;
				j++;
				ci = codetotal[i][j];
				if (ci.getCodetype() == NOTE && 
					ci.getResult().compare(funName) == 0 && ci.getOperand1().compare("func") == 0) {
					CodeItem nci(ci.getCodetype(),ci.getResult(), "inline", ci.getOperand2());
					codetotal[i][j] = nci;
				}
				else {
					cout << "函数内联不应该发生的情况：调用函数之后没有note结束标签. " << endl;
				}
			}
		}
		if (step6) {	/* step6.添加alloc指令到函数开始位置，更新符号表 */
			for (j = 1; j < codetotal[i].size(); j++) 
				if (codetotal[i][j].getCodetype() != PARA && codetotal[i][j].getCodetype() != ALLOC) break;
			codetotal[i].insert(codetotal[i].begin() + j, newInsertAllocIr.begin(), newInsertAllocIr.end());
			total[i].insert(total[i].end(), newInsertSymbolTable.begin(), newInsertSymbolTable.end());
		}
		if (debug) cout << "step6 done." << endl;
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
				for (k = size3 - 1; k >= 0; k--) {	// 遍历中间代码
					CodeItem ci = blockCore[i][j].Ir[k];
					switch (ci.getCodetype())
					{
					case ADD: case SUB: case MUL: case DIV: case REM:
					case AND: case OR: case NOT: case EQL: case NEQ: case SGT: case SGE: case SLT: case SLE:
					case LOAD: case LOADARR:
						if (tmpout.find(ci.getResult()) == tmpout.end()) {
							update = true;
							blockCore[i][j].Ir.erase(blockCore[i][j].Ir.begin() + k);
						}
						else {
							if (ifGlobalVariable(ci.getOperand1()) || ifLocalVariable(ci.getOperand1()) || ifTempVariable(ci.getOperand1())) 
								tmpout.insert(ci.getOperand1());
							if (ifGlobalVariable(ci.getOperand2()) || ifLocalVariable(ci.getOperand2()) || ifTempVariable(ci.getOperand2()))
								tmpout.insert(ci.getOperand2());
							tmpout.erase(ci.getResult());
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
							tmpout.erase(ci.getOperand1());
						}
						break;
					case STOREARR:		// 数组不敢删除
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

//数组
map<string, map<string, string>> arr2value;
//array offset value
//数组

//记录被赋值的全局数组
set<string> globalChanged;

//标记没有被定义过的全局变量，用于常量传播
void markGloablVar(vector<vector<basicBlock>>& funcs) {
	for (int i = 1; i < funcs.size(); i++) {
		for (int j = 0; j < funcs.at(i).size(); j++) {
			for (int k = 0; k < funcs.at(i).at(j).Ir.size(); k++) {
				auto instr = funcs.at(i).at(j).Ir.at(k);
				//全局数组被赋值，就不能被常量传播
				if (instr.getCodetype() == STOREARR && isGlobal(instr.getOperand1())) {
					globalChanged.insert(instr.getOperand1());
				}
				if (instr.getCodetype() == LOAD && instr.getOperand2() == "array" && isGlobal(instr.getOperand1())) {
					globalChanged.insert(instr.getOperand1());
				}
				if (instr.getCodetype() == STORE && isGlobal(instr.getOperand1())) {
					globalChanged.insert(instr.getOperand1());
				}
			}
		}
	}
}

void SSA::const_propagation() {
	markGloablVar(blockCore);
	for (int i = 1; i < blockCore.size(); i++) {	//遍历每个函数
		auto& blocks = blockCore.at(i);
		for (int j = 0; j < blocks.size(); j++) {	//遍历每个基本的ir
			auto& ir = blocks.at(j).Ir;
			var2value.clear(); arr2value.clear();	//初始化
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
					//局部数组赋值为常数，偏移为常数
					if (isNumber(instr.getResult()) && isNumber(instr.getOperand2())) {
						if (arr2value.find(ope1) != arr2value.end()) {	//已经有这个数组
							arr2value[ope1][instr.getOperand2()] = instr.getResult();
						}
						else {
							arr2value[ope1] = map<string, string>();
							arr2value[ope1][instr.getOperand2()] = instr.getResult();
						}
					}
					else {	//如果有一个不是常数，那么就释放全部存的常数值
						arr2value.clear();
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
					if (isNumber(instr.getOperand2())) {
						if (arr2value[ope1].find(instr.getOperand2()) != arr2value[ope1].end()) {
							var2value[res] = arr2value[ope1].find(instr.getOperand2())->second;
						}
					}
					//没有改赋值过的全局数组
					if (isGlobal(ope1) && globalChanged.find(ope1) == globalChanged.end()) {
						if (isNumber(instr.getOperand2())) {
							for (int k = 0; k < total.at(0).size(); k++) {
								if (total.at(0).at(k).getName() == ope1) {
									var2value[res] = I2A(total.at(0).at(k).getIntValue()[A2I(instr.getOperand2()) / 4]);
								}
							}
						}
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
							instr.setInstr("", instr.getResult(), "");	//如果结果==0，就直接跳转到res对应的标签
						}
						else {
							instr.setInstr("", instr.getOperand2(), "");	//如果结果!=0，直接跳转到ope2对应的标签
						}
					}
					break;
				}case CALL: {	//数组清空，数组作为地址其值可以被改变
					arr2value.clear();
				}
				default:
					break;
				}
			}
		}
	}
}



///====================================================================

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
					if (isTmp(res) && !isGlobal(ope1)) {	//变量赋值，非立即数赋值
						if (var2copy.find(res) != var2copy.end()) {	//如果赋值的中间变量对应某个变量，那么被赋值的变量也对应那个变量
							var2copy[ope1] = var2copy[res];
						}
						releaseCopy(ope1);
					}
					break;
				}
				case LOAD: {
					if (ope2 != "para" && ope2 != "array" && !isGlobal(ope1)) {	//加载变量的值
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
		//输出当前函数的所有循环
		//print
		for (auto circle : func2circles.at(i)) {
			mark_invariant(i, circle);				//确定不变式
			ofstream ly1("xunhuan1.txt");
			printCircleIr(this->blockCore, ly1);
			code_outside(i, circle);				//不变式外提
			ofstream ly2("xunhuan2.txt");
			printCircleIr(this->blockCore, ly2);
		}
	}
	//printCircle();
}

//================================================
//达到定义链和 使用定义链
//================================================

void SSA::mark_invariant(int funcNum, Circle& circle) {
	auto& blocks = blockCore.at(funcNum);
	auto& udchain = func2udChains.at(funcNum);
	try {
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
							if (def.var != "" && blocks.at(def.bIdx).Ir.at(def.lIdx).getInvariant() == 1) {	//定值点被标记了
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
						if (def.var != "" && blocks.at(def.bIdx).Ir.at(def.lIdx).getInvariant() == 1) {	//定值点被标记了
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
							if (def.var != "" && blocks.at(def.bIdx).Ir.at(def.lIdx).getInvariant() == 1) {	//定值点被标记了
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
							if (def.var != "" && blocks.at(def.bIdx).Ir.at(def.lIdx).getInvariant() == 1) {	//定值点被标记了
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
							if (def.var != "" && blocks.at(def.bIdx).Ir.at(def.lIdx).getInvariant() == 1) {	//定值点被标记了
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
							if (def.var != "" && blocks.at(def.bIdx).Ir.at(def.lIdx).getInvariant() == 1) {	//定值点被标记了
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
	catch (exception e) {
		WARN_MSG("wrong in mark invariant! maybe the circle find is wrong!");
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


void SSA::code_outside(int funcNum, Circle& circle) {
	//外提代码
	auto& blocks = blockCore.at(funcNum);
	try {		
		for (auto idx : circle.cir_blks) {
			auto& ir = blocks.at(idx).Ir;
			int pos = 0;
			for (int j = 0; j < ir.size(); j++) { //这里判断每条指令的操作数是否为常数或者定值在循环外面
				auto& instr = ir.at(j);
				if (instr.getInvariant() == 1) {	//不变式代码
					//外提到循环开始基本块的label前面
					if (condition1(circle.cir_outs, idx, funcNum) || condition2(circle.cir_outs, instr.getResult(), funcNum)) {
						auto tmp = instr;
						instr.setCodeOut();
						blocks.at(circle.cir_begin).Ir.insert(blocks.at(circle.cir_begin).Ir.begin() + pos++, tmp);
						j++;
					}
				}
			}
		}
	}
	catch (exception e) {
		WARN_MSG("code outside wrong!!");
	}
	//删除标记为outcode的代码
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


void SSA::optimize_arrayinit() {
	int i, j, k;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {	// 函数
		j = 1;	// 基本块
		if (blockCore[i][j].Ir.empty())	continue;
		map<string, int> arraySize;	// 初始化数组size大小
		map<string, vector<int>> arrayNotInit;	// 不用初始化的元素
		map<string, vector<int>> arrayInit;		// 必须初始化的元素
		for (k = 0; k < blockCore[i][j].Ir.size(); k++) {
			CodeItem ci = blockCore[i][j].Ir[k];
			if (ci.getCodetype() == ARRAYINIT) {
				arraySize[ci.getOperand1()] = str2int(ci.getOperand2());
				arrayNotInit[ci.getOperand1()] = vector<int>();
				arrayInit[ci.getOperand1()] = vector<int>();
			}
			else if (ci.getCodetype() == STOREARR && ifDigit(ci.getOperand2())) {
				int index = str2int(ci.getOperand2()) / 4;
				if (arrayNotInit.find(ci.getOperand1()) != arrayNotInit.end() &&
					find(arrayNotInit[ci.getOperand1()].begin(), arrayNotInit[ci.getOperand1()].end(), index) == arrayNotInit[ci.getOperand1()].end())
					arrayNotInit[ci.getOperand1()].push_back(index);
			}
			else if (ci.getCodetype() == LOADARR) {
				if (!ifDigit(ci.getOperand2())) {	// 不确定要用数组的那个元素，整个数组必须初始化
					if (arraySize.find(ci.getOperand1()) != arraySize.end()) {
						arraySize.erase(ci.getOperand1());
						arrayNotInit.erase(ci.getOperand1());
						arrayInit.erase(ci.getOperand1());
					}
				}
				else {
					int index = str2int(ci.getOperand2()) / 4;
					if (arrayInit.find(ci.getOperand1()) != arrayInit.end() &&
						find(arrayNotInit[ci.getOperand1()].begin(), arrayNotInit[ci.getOperand1()].end(), index) == arrayNotInit[ci.getOperand1()].end())
						// 之前未显式定义，必须初始化的元素
						arrayInit[ci.getOperand1()].push_back(index);
				}
			}
		}
		if (!arrayNotInit.empty()) {
			for (map<string, vector<int>>::iterator iter = arrayNotInit.begin(); iter != arrayNotInit.end(); iter++) {
				if (!(iter->second).empty())
					sort((iter->second).begin(), (iter->second).end());
			}
		}
		if (!arrayInit.empty()) {
			for (map<string, vector<int>>::iterator iter = arrayInit.begin(); iter != arrayInit.end(); iter++) {
				if (!(iter->second).empty())
					sort((iter->second).begin(), (iter->second).end());
			}
		}
		for (k = 0; k < blockCore[i][j].Ir.size(); k++) {
			CodeItem ci = blockCore[i][j].Ir[k];
			if (ci.getCodetype() == ARRAYINIT) {
				if (arraySize.find(ci.getOperand1()) == arraySize.end()) continue;
				vector<int> v = arrayNotInit[ci.getOperand1()];
				if (!v.empty() && v[v.size() - 1] - v[0] + 1 == v.size()) {
					vector<int> v2 = arrayInit[ci.getOperand1()];
					bool update = true;
					if (!v.empty()) {
						for (vector<int>::iterator iter = v2.begin(); iter != v2.end(); iter++) {
							if (find(v.begin(), v.end(), *iter) != v.end()) {
								// 必须初始化的元素在无需初始化的列表里面
								update = false; break;
							}
						}
					}
					if (!update) continue;
					string arrayName = ci.getOperand1();
					int firstMember = v[0];
					int lastMember = v[v.size() - 1];
					if (firstMember == 0) {
						int initSize = arraySize[arrayName] - v.size();
						if (initSize > 1) {
							CodeItem nci(ARRAYINIT, "0", arrayName, to_string(initSize), to_string(lastMember + 1));
							blockCore[i][j].Ir[k] = nci;
						}
						else if (initSize == 1) {
							CodeItem nci(STOREARR, "0", arrayName, to_string(4 * (lastMember + 1)));
							blockCore[i][j].Ir[k] = nci;
						}
						else {
							blockCore[i][j].Ir.erase(blockCore[i][j].Ir.begin() + k);
							k--;
						}
					}
					else if (lastMember == arraySize[arrayName] - 1) {
						int initSize = arraySize[arrayName] - v.size();
						if (initSize > 1) {
							CodeItem nci(ARRAYINIT, "0", arrayName, to_string(initSize), "0");
							blockCore[i][j].Ir[k] = nci;
						}
						else if (initSize == 1) {
							CodeItem nci(STOREARR, "0", arrayName, "0");
							blockCore[i][j].Ir[k] = nci;
						}
						else {
							blockCore[i][j].Ir.erase(blockCore[i][j].Ir.begin() + k);
							k--;
						}
					}
					else {
						int initSize1 = firstMember + 1;
						if (initSize1 > 1) {
							CodeItem nci(ARRAYINIT, "0", arrayName, to_string(initSize1), "0");
							blockCore[i][j].Ir[k] = nci;
						}
						else if (initSize1 == 1) {
							CodeItem nci(STOREARR, "0", arrayName, "0");
							blockCore[i][j].Ir[k] = nci;
						}
						else cout << "arrayinit优化：error1" << endl;
						int initSize2 = arraySize[arrayName] - 1 - lastMember;
						k++;
						if (initSize2 > 1) {
							CodeItem nci(ARRAYINIT, "0", arrayName, to_string(initSize2), to_string(lastMember + 1));
							blockCore[i][j].Ir.insert(blockCore[i][j].Ir.begin() + k, nci);
						}
						else if (initSize2 == 1) {
							CodeItem nci(STOREARR, "0", arrayName, to_string(4 * (lastMember + 1)));
							blockCore[i][j].Ir.insert(blockCore[i][j].Ir.begin() + k, nci);
						}
						else cout << "arrayinit优化：error2" << endl;
					}
				}
			}
		}
	}
}
 //从这开始都是丛睿轩加的
bool allowFuncname(string name)		//call的函数必须是自定义函数且该函数内部不能出现数组、全局变量、函数调用(后两个可通过非@可判断)
{
	if (name == "@printf" || name == "@_sysy_starttime" || name == "@_sysy_stoptime" || name == "@putarray"
		|| name == "@putch" || name == "@putint" || name == "@getint" || name == "@getarray" || name == "@getch") {
		return false;
	}
	int i = 0;
	for (i = 1; i < total.size(); i++) {
		if (total[i][0].getName() == name) {
			break;
		}
	}
	int j;
	for (j = 1; j < codetotal[i].size(); j++) {
		if (codetotal[i][j].getCodetype() == LOADARR || codetotal[i][j].getCodetype() == STOREARR) {
			return false;
		}
		if (codetotal[i][j].getResult()[0] == '@') {
			return false;
		}
		if (codetotal[i][j].getOperand1()[0] == '@') {
			return false;
		}
		if (codetotal[i][j].getOperand2()[0] == '@') {
			return false;
		}
	}
	return true;
}
bool judgeTemp(string a)
{
	if (a[0] == '%' && isdigit(a[1])) {
		return true;
	}
	return false;
}
string numtoString(int a)
{
	stringstream trans;          //数字和字符串相互转化渠道
	trans << a;
	return trans.str();
}
void SSA::optimize_delete_same_exp()
{
	int o, p, q;
	int i, j, k;
	int times = 0;
	for ( o = 1; o < blockCore.size(); o++) { // 遍历函数
		for (p = 1; p < blockCore[o].size() - 1; p++) { // 遍历该函数的基本块，跳过entry和exit块
			int get;
			for (q = 0,get=0; q < blockCore[o][p].Ir.size(); q++) { // 遍历基本块中的中间代码
				if (blockCore[o][p].Ir[q].getCodetype() == CALL && allowFuncname(blockCore[o][p].Ir[q].getOperand1())==false) {
					get = 1;
					break;   //调用非法函数直接不删
				}
			}
			if (get == 1) {
				continue;
			}
			else {		//公共表达式删除
				//从此开始找相同表达式
				set<string> varName;
				map<string, int>  record;		//记录load或loadarr型变量出现次数
				map<string, string> newVarName;	//记录新的变量名
				map<string, int> maxLine;		//记录变量最先出现store对应行数
				record.clear();
				maxLine.clear();
				newVarName.clear();
				for (i = 0; i < blockCore[o][p].Ir.size(); i++) {		//保证从noWhileLabel+1 到最后一定是顺序执行的
					if (blockCore[o][p].Ir[i].getCodetype() == STORE || blockCore[o][p].Ir[i].getCodetype() == STOREARR) {
						if (!maxLine.count(blockCore[o][p].Ir[i].getOperand1()) > 0) {
							maxLine[blockCore[o][p].Ir[i].getOperand1()] = i; //涉及此类变量求出下限
						}
					}
					if (blockCore[o][p].Ir[i].getCodetype() == LOAD || blockCore[o][p].Ir[i].getCodetype() == LOADARR) {
						varName.insert(blockCore[o][p].Ir[i].getOperand1());				//涉及此类变量先记录下来，存在表达式相同的可能
						record[blockCore[o][p].Ir[i].getOperand1()] = record[blockCore[o][p].Ir[i].getOperand1()] + 1;
					}
				}
				while (true) {
					int flag = 0;
					for (string str : varName) {		//该变量至少load2次
						if (record[str] <= 1) {
							varName.erase(str);
							flag = 1;
							break;
						}
					}
					if (flag == 0) {
						break;
					}
				}
				if (varName.size() == 0) {
					continue;			//没有变量符合公共表达式删除，直接退出
				}
				int j, i1, j1, i2, j2;
				int jian = 0;
				newVarName.clear();
				for (string str : varName) {
					if (!maxLine.count(str) > 0) {
						maxLine[str] = blockCore[o][p].Ir.size();
					}
					else {
						maxLine[str] = maxLine[str] - jian;
					}
					for (i = 0; i < maxLine[str]; i++) {		//保证从noWhileLabel+1 到最后一定是顺序执行的
						if (blockCore[o][p].Ir[i].getCodetype() == LOAD || blockCore[o][p].Ir[i].getCodetype() == LOADARR) {
							if (str == blockCore[o][p].Ir[i].getOperand1()) {		//找到该变量第一次出现的位置
								j = i;
								break;
							}
						}
					}
					for (i = i + 1; i < maxLine[str]; i++) {
						if (blockCore[o][p].Ir[i].getCodetype() == LOAD || blockCore[o][p].Ir[i].getCodetype() == LOADARR) {
							if (str == blockCore[o][p].Ir[i].getOperand1()) {		//找到该变量第二次出现的位置
								j1 = j; i1 = i; j2 = j; i2 = i;
								while (j1 > 0 && blockCore[o][p].Ir[j1].isequal(blockCore[o][p].Ir[i1]) && j < i1) {
									j1--;		i1--;
								}
								while (j2 < i && blockCore[o][p].Ir[j2].isequal(blockCore[o][p].Ir[i2]) && j2 < i1) {
									j2++;		i2++;
								}		//上下搜索公共子表达式
								j2 = j2 - 1; i2 = i2 - 1; j1 = j1 + 1; i1 = i1 + 1;
								int zj = j2;;
								while (blockCore[o][p].Ir[zj].getCodetype() == NOTE) {
									zj--;//NOTE的res并不是临时寄存器，产生错误
								}
								int zi = i2;
								while (blockCore[o][p].Ir[zi].getCodetype() == NOTE) {
									zi--;//NOTE的res并不是临时寄存器，产生错误
								}
								if (j2 - j1 < 10 || judgeTemp(blockCore[o][p].Ir[zj].getResult()) == false || judgeTemp(blockCore[o][p].Ir[zi].getResult()) == false) {
									continue;	//公共子表达式要大于6条而且最后一条的res字段应该是临时变量
								}
								else {		//可以删除了
									string xiabiao = numtoString(j1) + "-" + numtoString(j2);
									if (!(newVarName.count(xiabiao) > 0)) {		//第一次找到公共子表达式
										string replace = "%Rep+"+xiabiao+"-" + numtoString(times++) + "+" + total[o][0].getName();
										newVarName[xiabiao] = replace;
										string newTemp;
										int i4 = i1;
										while (i1 < i2) {		//找一个可用的临时变量
											if (judgeTemp(blockCore[o][p].Ir[i1].getResult())) {
												newTemp = blockCore[o][p].Ir[i1].getResult(); break;
											}
											if (judgeTemp(blockCore[o][p].Ir[i1].getOperand1())) {
												newTemp = blockCore[o][p].Ir[i1].getOperand1(); break;
											}
											if (judgeTemp(blockCore[o][p].Ir[i1].getOperand2())) {
												newTemp = blockCore[o][p].Ir[i1].getOperand2(); break;
											}
											i1++;
										}
										i1 = i4;
										//同时往j2处插入新的中间代码
										string oldTemp = blockCore[o][p].Ir[zj].getResult();
										blockCore[o][p].Ir[zj].setResult(newTemp);	//改成新的
										CodeItem citem1 = CodeItem(STORE, newTemp, newVarName[xiabiao], "");	//赋值单值
										CodeItem citem2 = CodeItem(LOAD, oldTemp, newVarName[xiabiao], "");
										blockCore[o][p].Ir.insert(blockCore[o][p].Ir.begin() + j2 + 1, citem1);
										blockCore[o][p].Ir.insert(blockCore[o][p].Ir.begin() + j2 + 2, citem2);
										i2 = i2 + 2; i1 = i1 + 2; maxLine[str] = maxLine[str] + 2; jian = jian - 2; zi = zi + 2;
									}
									//用zi是因为NOTE的res并不是临时寄存器，产生错误
									CodeItem citem = CodeItem(LOAD, blockCore[o][p].Ir[zi].getResult(), newVarName[xiabiao], "");
									int i3 = i1;
									while (i1 <= i2) {		//删除公共子表达式
										blockCore[o][p].Ir.erase(blockCore[o][p].Ir.begin() + i3);
										i1++;
									}
									blockCore[o][p].Ir.insert(blockCore[o][p].Ir.begin() + i3, citem);	//生成新的子表达式
									maxLine[str] = maxLine[str] - (i2 - i3);
									jian = jian + (i2 - i3);
									for (; i < maxLine[str]; i++) {	//找到一个基础上继续找相同的
										if (blockCore[o][p].Ir[i].isequal(blockCore[o][p].Ir[j1])) {
											int i5 = i; int j5 = j1;
											while (j5 <= j2) {
												if (!blockCore[o][p].Ir[i5].isequal(blockCore[o][p].Ir[j5])) {
													break;
												}
												j5++;  i5++;
											}
											j5 = j5 - 1; i5 = i5 - 1;
											if (j5 == j2) {	//找到相同的
												int zzz = i5;
												while (blockCore[o][p].Ir[zzz].getCodetype() == NOTE) {	
													zzz--;//NOTE的res并不是临时寄存器，产生错误
												}
												CodeItem citem = CodeItem(LOAD, blockCore[o][p].Ir[zzz].getResult(), newVarName[xiabiao], "");
												int i6 = i5 - (j2 - j1);
												int i7 = i6;
												while (i6 <= i5) {		//删除公共子表达式
													blockCore[o][p].Ir.erase(blockCore[o][p].Ir.begin() + i7);
													i6++;
												}
												blockCore[o][p].Ir.insert(blockCore[o][p].Ir.begin() + i7, citem);	//生成新的子表达式
												maxLine[str] = maxLine[str] - (j2 - j1);
												jian = jian + (i2 - i3);
												i = i6;
											}
											else {
												continue;
											}
										}
									}
									i = i3;		//恢复循环变量值
								}
							}
						}
					}
				}
				//最后插入符号表同时生成ALLOC代码
				map <  string, string>::iterator iter;
				for (iter = newVarName.begin(); iter != newVarName.end(); iter++)
				{
					symbolTable item = symbolTable(VARIABLE, INT, iter->second, 0, 0);		//此时插入符号表要带@
					total[o].push_back(item);
					CodeItem citem = CodeItem(ALLOC, iter->second, "0", "1");
					for (k = 1; k < blockCore[o][1].Ir.size(); k++) {   //define 、 para 、alloc在第一个基本块
						if (blockCore[o][1].Ir[k].getCodetype() == DEFINE || blockCore[o][1].Ir[k].getCodetype() == PARA || blockCore[o][1].Ir[k].getCodetype() == ALLOC) {
							continue;
						}
						else {
							break;
						}
					}
					blockCore[o][1].Ir.insert(blockCore[o][1].Ir.begin() + k, citem);
				}
			}
		}
	}
	/*
	for (o = 1; o < blockCore.size(); o++) { // 遍历函数
		for (p = 1; p < blockCore[o].size() - 1; p++) { // 遍历该函数的基本块，跳过entry和exit块
			for (q = 0; q < blockCore[o][p].Ir.size(); q++) { // 遍历基本块中的中间代码
				cout << blockCore[o][p].Ir[q].getContent() << endl;
			}
		}
		cout << "\n" << endl;
	}*/
}

void SSA::delete_dead_codes_2() {
	int i, j, k;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {
		for (j = 2; j < blockCore[i].size() - 1; j++) {
			if (blockCore[i][j].pred.size() > 1 ||
				blockCore[i][j].succeeds.size() > 1 ||
				blockCore[i][j].succeeds.find(blockCore[i].size() - 1) != blockCore[i][j].succeeds.end()) continue;
			bool res = true;
			int size3 = blockCore[i][j].Ir.size();
			for (k = 0; k < size3; k++) {
				CodeItem ci = blockCore[i][j].Ir[k];
				if (ci.getCodetype() == STOREARR) {
					res = false; break;
				}
				if (ci.getCodetype() == CALL) {
					res = false; break;
				}
				if (ifOp1Def(ci.getCodetype())) {
					if (ifGlobalVariable(ci.getOperand1())) {
						res = false; break;
					}
				}
				if (ifResultDef(ci.getCodetype())) {
					if (ifGlobalVariable(ci.getResult())) {
						res = false; break;
					}
				}
			}
			if (!res) continue;
			set<string> tmpDef, tmpDef2, tmpUse, tmpIn, tmpOut;
			for (set<string>::iterator iter = blockCore[i][j].def.begin(); iter != blockCore[i][j].def.end(); iter++)
				if (!ifTempVariable(*iter)) tmpDef.insert(*iter);
			for (set<string>::iterator iter = blockCore[i][j].def2.begin(); iter != blockCore[i][j].def2.end(); iter++)
				if (!ifTempVariable(*iter)) tmpDef2.insert(*iter);
			for (set<string>::iterator iter = blockCore[i][j].use.begin(); iter != blockCore[i][j].use.end(); iter++)
				if (!ifTempVariable(*iter)) tmpUse.insert(*iter);
			for (set<string>::iterator iter = blockCore[i][j].in.begin(); iter != blockCore[i][j].in.end(); iter++)
				if (!ifTempVariable(*iter)) tmpIn.insert(*iter);
			for (set<string>::iterator iter = blockCore[i][j].out.begin(); iter != blockCore[i][j].out.end(); iter++)
				if (!ifTempVariable(*iter)) tmpOut.insert(*iter);
			set<string> tmp1;
			set_intersection(tmpDef.begin(), tmpDef.end(), tmpOut.begin(), tmpOut.end(), inserter(tmp1, tmp1.begin()));
			if (!tmp1.empty()) continue;
			// if (tmpUse.empty()) {
			if (tmpDef2.empty()) {
				CodeItem ci1 = blockCore[i][j].Ir[0];
				CodeItem ci2 = blockCore[i][j].Ir[blockCore[i][j].Ir.size() - 1];
				blockCore[i][j].Ir.clear();
				if (ci1.getCodetype() == LABEL) blockCore[i][j].Ir.push_back(ci1);
				if (ci2.getCodetype() == BR) blockCore[i][j].Ir.push_back(ci2);
				build_def_use_chain();
				active_var_analyse();
				continue;
			}
			//for (set<string>::iterator iter = tmpUse.begin(); iter != tmpUse.end(); iter++) {
			for (set<string>::iterator iter = tmpDef2.begin(); iter != tmpDef2.end(); iter++) {
				for (k = 1; k < blockCore[i].size() - 1; k++)
					if (j == k) continue;
					else if (blockCore[i][k].use.find(*iter) != blockCore[i][k].use.end()) {
						// cout << *iter << i << " " << j << " " << k << endl;
						res = false; break;
					}
				if (!res) break;
			}
			if (res) {
				CodeItem ci1 = blockCore[i][j].Ir[0];
				CodeItem ci2 = blockCore[i][j].Ir[blockCore[i][j].Ir.size() - 1];
				blockCore[i][j].Ir.clear();
				if (ci1.getCodetype() == LABEL) blockCore[i][j].Ir.push_back(ci1);
				if (ci2.getCodetype() == BR) blockCore[i][j].Ir.push_back(ci2);
				build_def_use_chain();
				active_var_analyse();
				continue;
			}
		}
	}
}

void SSA::optimize_br_label() {
	int i, j, k;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {
		int size2 = blockCore[i].size();
		for (j = 2; j < size2 - 1; j++) {
			CodeItem ci1 = blockCore[i][j].Ir[0];
			CodeItem ci2 = blockCore[i][j].Ir[blockCore[i][j].Ir.size() - 1];
			if (blockCore[i][j].Ir.size() == 2 && ci1.getCodetype() == LABEL && ci2.getCodetype() == BR) {
				if (!ifTempVariable(ci2.getOperand1()) && blockCore[i][j - 1].Ir.back().getCodetype() == BR) {
					if (ifTempVariable(blockCore[i][j - 1].Ir.back().getOperand1())) {
						if (ci1.getResult().compare(blockCore[i][j - 1].Ir.back().getResult()) == 0) {
							CodeItem tci = blockCore[i][j - 1].Ir.back();
							CodeItem tnci(BR, ci2.getOperand1(), tci.getOperand1(), tci.getOperand2());
							blockCore[i][j - 1].Ir[blockCore[i][j - 1].Ir.size() - 1] = tnci;
						}
						if (ci1.getResult().compare(blockCore[i][j - 1].Ir.back().getOperand2()) == 0) {
							CodeItem tci = blockCore[i][j - 1].Ir.back();
							CodeItem tnci(BR, tci.getResult(), tci.getOperand1(), ci2.getOperand1());
							blockCore[i][j - 1].Ir[blockCore[i][j - 1].Ir.size() - 1] = tnci;
						}
					}
					else {
						if (ci1.getResult().compare(blockCore[i][j - 1].Ir.back().getOperand1()) == 0) {
							CodeItem tci = blockCore[i][j - 1].Ir.back();
							CodeItem tnci(BR, "", ci2.getOperand1(), "");
							blockCore[i][j - 1].Ir[blockCore[i][j - 1].Ir.size() - 1] = tnci;
						}
					}
				}
			}
		}
	}
	build_def_use_chain();
	active_var_analyse();
}
