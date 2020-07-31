﻿#include "ssa.h"
#include <regex>
#include <algorithm>
#include <set>
#include <map>

using namespace std;

// 判断是否是临时变量
// 可用于通过判断br语句的result操作数选出相应的标签名称
// br tmpReg label1 label2
// br label
bool SSA::ifTempVariable(string name) {
	regex r("%\\d+");
	return regex_match(name, r);
}

// 判断字符串是否是数字
bool SSA::ifDigit(string name) {
	regex r("(\\+|\\-)?\\d+");
	return regex_match(name, r);
}

// 判断字符串是否是全局变量
bool SSA::ifGlobalVariable(string name) {
	regex r("@.+");
	return regex_match(name, r);
}

// 判断是否是局部变量
bool SSA::ifLocalVariable(string name) {
	regex r("%.+");
	return regex_match(name, r) && (name.find(".") == string::npos) && !ifTempVariable(name);
}

// 判断是否是虚拟寄存器
bool SSA::ifVR(string name) {
	regex r("VR\\d+");
	return regex_match(name, r);
}

// 在一个函数的所有中间代码中找到某个标签的位置，即它是第几条中间代码
// @result: 0, 1, 2, ..., size-1(size代表该函数总的中间代码条数)
// @exception: -1(表示没有找到，lzh在调用该函数时正常情况下不应出现该情况)
int lookForLabel(vector<CodeItem> v, string label) {
	int size = v.size();
	int i;
	for (i = 0; i < size; i++) {
		if (v[i].getCodetype() == LABEL && v[i].getResult() == label) return i;
	}
	return -1;
}

// 用于查找某个函数中第一条语句序号是key的基本块，即找出哪个基本块是以上面函数查到的label标签为第一条语句
// @para key: 传入的是上面lookForLabel函数查出的label标签对应的中间代码序号
// @result: 0, 1, 2, ..., size-1(size代表基本块的总数量)
// exception: -1(表示没有找到，lzh在调用该函数时正常情况下不应出现该情况)
int locBlockForLabel(vector<int> v, int key) {
	int size = v.size();
	for (int i = 0; i < size; i++) if (v[i] == key) return i;
	cout << "locBlockForLabel Wrong!" << endl; 
	return -1;
}

// 判断标签类型
// 无需处理
bool ifNotToDeal(irCodeType ct) {
	switch (ct)
	{
	case ALLOC: case GLOBAL: case LABEL: case DEFINE: case NOTE:
		return true;
	default:
		return false;
	}
}
// result为定义变量
bool ifResultDef(irCodeType ct) {
	switch (ct)
	{
	case ADD: case SUB: case MUL: case DIV: case REM:
	case AND: case OR: case NOT: case EQL: case NEQ: case SGT: case SGE: case SLT: case SLE:
	case LOAD: case LOADARR:
	case RET:
	case PUSH:
	case BR:
	case PARA:
	case CALL:
		return true;
	default:
		return false;
	}
}
// op1为定义变量
bool ifOp1Def(irCodeType ct) {
	switch (ct)
	{
	case STORE: case STOREARR: case POP: 
	case GETREG: case LEA: case MOV:	// 新增对刘阳LIR中间代码支持
		return true;
	default:
		return false;
	}
}

// 找到基本块的每个起始语句
void SSA::find_primary_statement() {
	int size1 = codetotal.size();
	int i, j, k;
	// 申请空间
	for (i = 0; i < size1; i++) {
		vector<int> v; blockOrigin.push_back(v);
	}
	// 遍历每个函数
	for (i = 1; i < size1; i++)
	{
		vector<CodeItem> v = codetotal[i];
		int size2 = v.size();
		// tmp用于保存所有基本块的起始语句: 初始化 0 代表entry，无实际含义；初始化 1 代表函数第一条中间代码
		vector<int> tmp = { 0, 1 };
		// 遍历该函数内的所有中间代码，找出基本块的第一条语句，注意在push时要加1，因为代表第几条中间代码，1,2,3...
		for (j = 0; j < size2; j++) {
			CodeItem c = v[j];
			if (c.getCodetype() == BR) {			// 跳转指令
				if (ifTempVariable(c.getOperand1()) || ifVR(c.getOperand1())) {
					k = lookForLabel(v, c.getResult());
					if (find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
					k = lookForLabel(v, c.getOperand2());
					if (find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
				}
				else if (ifDigit(c.getOperand1())) {	/* debug1: result可能是常数，e.g br 1 if_... else_... */
					// 这个问题通过 simplify_br() 函数解决，转换为无条件跳转指令
					k = -1;
					cout << "Wrong" << endl;
				}
				else {
					k = lookForLabel(v, c.getOperand1());
					if (find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
				}
				// 分支指令的下一条指令
				k = j + 1;
				if (find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
			}
			// 返回指令
			else if (c.getCodetype() == RET) {
				k = j + 1;		// 分支指令的下一条指令
				if (k + 1 <= size2 && find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
			}
		}
		tmp.push_back(size2 + 1);		// exit块
		sort(tmp.begin(), tmp.end());	// 对所有入口语句从小到大排序
		blockOrigin[i] = tmp;
	}
}

// 为每个函数划分基本块
void SSA::divide_basic_block() {
	int size1 = codetotal.size();
	int i, j, k;
	// 申请空间
	for (i = 0; i < size1; i++) {
		vector<basicBlock> v; blockCore.push_back(v);
	}
	// 遍历每个函数
	for (i = 1; i < size1; i++)
	{
		vector<int> v = blockOrigin[i];
		// tmp用于保存该函数下的所有基本块
		vector<basicBlock> tmp;
		// entry块
		basicBlock bb1(0, {}, {}, {});		// 序号为0，起始和终止语句均为0
		// 改：basicBlock bb1(0, 0, 0, {}, {});
		tmp.push_back(bb1);
		int size2 = v.size();
		// 函数中间代码划分的基本块
		for (j = 1; j < size2 - 1; j++) {
			vector<CodeItem> tv;
			for (k = v[j]; k < v[j + 1]; k++) {
				tv.push_back(codetotal[i][k - 1]);
			}
			// 改：basicBlock bb(j, v[j], v[j + 1] - 1, {}, {});	// 序号1, 2, 3...，起始语句为当前的入口语句，到下一条入口语句的前一条为止
			basicBlock bb(j, tv, {}, {});
			tmp.push_back(bb);
		}
		// exit块
		// 改：basicBlock bb2(j, v[j], v[j], {}, {});	// 前序节点无需初始化，后序节点为空
		basicBlock bb2(j, {}, {}, {});
		tmp.push_back(bb2);
		blockCore[i] = tmp;
	}
}

// 建立基本块间的前序和后序关系
void SSA::build_pred_and_succeeds() {
	int size1 = blockCore.size();
	int i, j, k;
	for (i = 1; i < size1; i++) {
		int size2 = blockCore[i].size();
		vector<int> v = blockOrigin[i];
		vector<basicBlock> tmp = blockCore[i];
		// 初始化entry块的后序节点为1，第1个基本块的前序节点为entry块即0
		tmp[0].succeeds.insert(1);
		tmp[1].pred.insert(0);
		// 建立关系
		for (j = 1; j < size2 - 1; j++) {
			if (!tmp[j].Ir.empty()) {
				CodeItem ci = tmp[j].Ir.back();
				if (ci.getCodetype() == BR) {
					// 跳转指令
					if (ifTempVariable(ci.getOperand1()) || ifVR(ci.getOperand1())) {
						// step1: 找到跳转到标签所在的基本块序号
						// cout << "step1";
						k = lookForLabel(codetotal[i], ci.getResult());
						k = locBlockForLabel(v, k + 1);
						// step2: 插入当前基本块的后序节点
						// cout << "step2";
						tmp[j].succeeds.insert(k);
						// step3: 为跳转到的基本块前序节点插入当前基本块的序号
						// cout << "step3";
						tmp[k].pred.insert(j);
						// step1: 找到跳转到标签所在的基本块序号
						// cout << "step4";
						k = lookForLabel(codetotal[i], ci.getOperand2());
						k = locBlockForLabel(v, k + 1);
						// step2: 插入当前基本块的后序节点
						// cout << "step5";
						tmp[j].succeeds.insert(k);
						// step3: 为跳转到的基本块前序节点插入当前基本块的序号
						// cout << "step6" << endl;
						tmp[k].pred.insert(j);
					}
					else {
						// step1: 找到跳转到标签所在的基本块序号
						k = lookForLabel(codetotal[i], ci.getOperand1());
						k = locBlockForLabel(v, k + 1);
						// step2: 插入当前基本块的后序节点
						tmp[j].succeeds.insert(k);
						// step3: 为跳转到的基本块前序节点插入当前基本块的序号
						tmp[k].pred.insert(j);
					}
				}
				else if (ci.getCodetype() == RET) {
					// 返回指令，该基本块跳转到exit块
					k = size2 - 1;
					tmp[j].succeeds.insert(k);
					tmp[k].pred.insert(j);
				}
				else {
					// 该基本块正常结束，顺序执行到下一个基本块（即顺序执行下一条代码语句）
					k = j + 1;
					tmp[j].succeeds.insert(k);
					tmp[k].pred.insert(j);
				}
			}
			else {
				// 该基本块正常结束，顺序执行到下一个基本块（即顺序执行下一条代码语句）
				k = j + 1;
				tmp[j].succeeds.insert(k);
				tmp[k].pred.insert(j);
			}
		}
		blockCore[i] = tmp;
	}
}

// 确定每个基本块的必经关系，参见《高级编译器设计与实现》P132 Dom_Comp算法
void SSA::build_dom_tree() {
	int size1 = blockCore.size();
	int i, j, k;
	for (i = 1; i < size1; i++)
	{
		// 进入到每个函数中
		int size2 = blockCore[i].size();
		vector<int> N;
		// 新建N集合，包含所有的基本块序号（节点）
		for (j = 0; j < size2; j++) N.push_back(j);	// 该函数下基本块的数量
		// Domin(r) := {r}
		blockCore[i][0].domin.insert(0);
		// for each n except r: Domin(n) := N
		for (j = 1; j < size2; j++)
				blockCore[i][j].domin.insert(N.begin(), N.end());
		bool change = true;
		while (change) {
			change = false;
			// for each n excpt r
			for (j = 1; j < size2; j++) {
				// if (blockCore[i][j].pred.empty()) continue;	// 若该基本块不可达则不进行分析
				// T := N
				set<int> T(N.begin(), N.end());
				for (set<int>::iterator iter = blockCore[i][j].pred.begin(); iter != blockCore[i][j].pred.end(); iter++) {
					set<int> tmp;
					set_intersection(T.begin(), T.end(), blockCore[i][*iter].domin.begin(), blockCore[i][*iter].domin.end(), inserter(tmp, tmp.begin()));
					T = tmp;
				}
				set<int> D = T;
				D.insert(j);
				// D ≠ Domin(n) case1
				if (D.size() != blockCore[i][j].domin.size()) {
					change = true;
					blockCore[i][j].domin = D;
				}
				else {
					for (set<int>::iterator iter1 = D.begin(), iter2 = blockCore[i][j].domin.begin(); iter1 != D.end(); iter1++, iter2++) {
						if (*iter1 != *iter2) { // D ≠ Domin(n) case2
							change = true;
							blockCore[i][j].domin = D;
							break;
						}
					}
				}
			}
		}
	}
}

// 确定每个基本块的直接必经关系，参见《高级编译器设计与实现》P134 Idom_Comp算法
void SSA::build_idom_tree() {
	int size1 = blockCore.size();
	int i, j, k;
	for (i = 1; i < size1; i++)
	{
		// foreach n
		// tmp(n) := Domin(n) - {n}
		int size2 = blockCore[i].size();
		for (j = 0; j < size2; j++) {
			set<int> tmp1, tmp2, tmp3;
			tmp1.insert(j);
			set_difference(blockCore[i][j].domin.begin(), blockCore[i][j].domin.end(), tmp1.begin(), tmp1.end(), inserter(tmp2, tmp2.begin()));;
			blockCore[i][j].tmpIdom = tmp2;
			blockCore[i][j].idom = tmp3;	// 申请idom空间
		}
		// foreach n
		for (j = 1; j < size2; j++) {
			// foreach s
			set<int> tmp;
			for (set<int>::iterator iter1 = blockCore[i][j].tmpIdom.begin(); iter1 != blockCore[i][j].tmpIdom.end(); iter1++) {
				if (tmp.find(*iter1) != tmp.end()) continue;
				set<int> tmp1, tmp2, tmp3;
				tmp1.insert(*iter1);
				set_difference(blockCore[i][j].tmpIdom.begin(), blockCore[i][j].tmpIdom.end(), tmp1.begin(), tmp1.end(), inserter(tmp2, tmp2.begin()));
				set_difference(tmp2.begin(), tmp2.end(), tmp.begin(), tmp.end(), inserter(tmp3, tmp3.begin()));
				// foreach t
				for (set<int>::iterator iter2 = tmp3.begin(); iter2 != tmp3.end(); iter2++) {
					if (blockCore[i][*iter1].tmpIdom.find(*iter2) != blockCore[i][*iter1].tmpIdom.end()) {
						tmp.insert(*iter2);
					}
				}
			}
			set<int> tmp4;
			set_difference(blockCore[i][j].tmpIdom.begin(), blockCore[i][j].tmpIdom.end(), tmp.begin(), tmp.end(), inserter(tmp4, tmp4.begin()));;
			blockCore[i][j].tmpIdom = tmp4;
		}
		// foreach n
		for (j = 1; j < size2; j++)
			if (blockCore[i][j].tmpIdom.empty())
				continue;
			else if (blockCore[i][j].tmpIdom.size() > 1)
				cout << i << " " << j << " " << "idom > 1" << endl;
			else
				blockCore[i][j].idom.insert(*(blockCore[i][j].tmpIdom.begin()));
	}
}

// 根据直接必经节点找到必经节点树中每个节点的后序节点
void SSA::build_reverse_idom_tree() {
	int size1 = blockCore.size();
	int i, j, k;
	for (i = 1; i < size1; i++)
	{
		int size2 = blockCore[i].size();
		for (j = 0; j < size2; j++) {
			set<int> s;
			// 如果该节点是某个节点的直接必经节点，则把它加入到那个节点的reverse_idom中
			for (k = 0; k < size2; k++) if (blockCore[i][k].idom.find(j) != blockCore[i][k].idom.end()) s.insert(k);
			blockCore[i][j].reverse_idom = s;
		}
	}
}

// 后序遍历必经节点树的递归函数
void SSA::post_order(int funNum, int node) {
	basicBlock bb = blockCore[funNum][node];
	for (set<int>::iterator iter = bb.reverse_idom.begin(); iter != bb.reverse_idom.end(); iter++) {
		post_order(funNum, *iter);
	}
	postOrder[funNum].push_back(node);
}

// 后序遍历必经节点树
void SSA::build_post_order() {
	int size1 = blockCore.size();
	int i;
	// 申请空间
	for (i = 0; i < size1; i++) { vector<int> v; postOrder.push_back(v); }
	// 为每个函数确定后序遍历序列
	for (i = 1; i < size1; i++) post_order(i, 0);
}

// 前序遍历必经节点树的递归函数
void SSA::pre_order(int funNum, int node) {
	preOrder[funNum].push_back(node);
	basicBlock bb = blockCore[funNum][node];
	for (set<int>::iterator iter = bb.reverse_idom.begin(); iter != bb.reverse_idom.end(); iter++) {
		pre_order(funNum, *iter);
	}
}

// 前序遍历必经节点树
void SSA::build_pre_order() {
	int size1 = blockCore.size();
	int i;
	// 申请空间
	for (i = 0; i < size1; i++) { vector<int> v; preOrder.push_back(v); }
	// 为每个函数确定前序遍历序列
	for (i = 1; i < size1; i++) pre_order(i, 0);
}

// 计算流图必经边界，参考《高级编译器设计与实现》 P185 Dom_Front算法
void SSA::build_dom_frontier() {
	int i, j, k;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {
		int size2 = postOrder[i].size();
		for (j = 0; j < size2; j++) {
			int p_i = postOrder[i][j];
			set<int> df;
			for (set<int>::iterator iter = blockCore[i][p_i].succeeds.begin(); iter != blockCore[i][p_i].succeeds.end(); iter++) {
				if (blockCore[i][p_i].reverse_idom.find(*iter) == blockCore[i][p_i].reverse_idom.end()) df.insert(*iter); // 注意IDOM的含义
			}
			for (set<int>::iterator iter1 = blockCore[i][p_i].reverse_idom.begin(); iter1 != blockCore[i][p_i].reverse_idom.end(); iter1++) {
				for (set<int>::iterator iter2 = blockCore[i][*iter1].df.begin(); iter2 != blockCore[i][*iter1].df.end(); iter2++) {
					if (blockCore[i][p_i].reverse_idom.find(*iter2) == blockCore[i][p_i].reverse_idom.end()) df.insert(*iter2);	// 注意IDOM的含义
				}
			}
			blockCore[i][p_i].df = df;
		}
	}
}

// 在基本块的use链中插入变量名称
void SSA::use_insert(int funNum, int blkNum, string varName) {
	if (varName == "") return;
	if (blockCore[funNum][blkNum].def.find(varName) != blockCore[funNum][blkNum].def.end()) return;	// use变量的定义为使用前未被定义的变量
	if (ifLocalVariable(varName) || ifTempVariable(varName) || ifVR(varName)) blockCore[funNum][blkNum].use.insert(varName);	// 不加入全局变量
}

// 在基本块的def链中插入变量名称
void SSA::def_insert(int funNum, int blkNum, string varName) {
	if (varName == "") return;
	if (blockCore[funNum][blkNum].use.find(varName) != blockCore[funNum][blkNum].use.end()) return;
	if (ifLocalVariable(varName) || ifTempVariable(varName) || ifVR(varName)) blockCore[funNum][blkNum].def.insert(varName);	// 不加入全局变量
}

// 在基本块的def2链中插入变量名称
void SSA::def2_insert(int funNum, int blkNum, string varName) {
	if (varName == "") return;
	// if (blockCore[funNum][blkNum].use.find(varName) != blockCore[funNum][blkNum].use.end()) return;
	if (ifLocalVariable(varName) || ifTempVariable(varName) || ifVR(varName)) blockCore[funNum][blkNum].def2.insert(varName);	// 不加入全局变量
}

// 计算ud链，即分析每个基本块的use和def变量
void SSA::build_def_use_chain() {
	int i, j, k;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {
		int size2 = blockCore[i].size();
		for (j = 0; j < size2; j++) {	// 申请空间
			set<string> s1; blockCore[i][j].use = s1;
			set<string> s2; blockCore[i][j].def = s2;
		}
		for (j = 1; j < size2; j++) {		// 遍历基本块
			// 改：for (k = blockCore[i][j].start; k <= blockCore[i][j].end; k++) {
			for (vector<CodeItem>::iterator iter = blockCore[i][j].Ir.begin(); iter != blockCore[i][j].Ir.end(); iter++) {
				// if (k - 1 < 0 || k - 1 >= codetotal[i].size()) continue;
				// CodeItem ci = codetotal[i][k - 1];
				CodeItem ci = *iter;
				if (ifResultDef(ci.getCodetype())) {
					// result为def，op1和op2为use
					use_insert(i, j, ci.getOperand2());
					use_insert(i, j, ci.getOperand1());
					def_insert(i, j, ci.getResult());
					def2_insert(i, j, ci.getResult());
				}
				else if (ifOp1Def(ci.getCodetype())) {
					use_insert(i, j, ci.getResult());
					use_insert(i, j, ci.getOperand2());
					def_insert(i, j, ci.getOperand1());
					def2_insert(i, j, ci.getOperand1());
				}
				else if (ifNotToDeal(ci.getCodetype())) {
					;
				}
				else {
					cout << "codetype" << ci.getCodetype() << endl;
				}
			}
		}
	}
}

// 活跃变量分析，生成in、out集合
void SSA::active_var_analyse() {
	int i, j, k;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {
		int size2 = blockCore[i].size();
		for (j = 0; j < size2; j++) {	// 申请空间
			set<string> s1; blockCore[i][j].in = s1;
			set<string> s2; blockCore[i][j].out = s2;
		}
		bool update = true;
		while (update) {
			update = false;
			for (j = 0; j < size2; j++) {
				set<string> tmp;
				for (set<int>::iterator iter = blockCore[i][j].succeeds.begin(); iter != blockCore[i][j].succeeds.end(); iter++)
					set_union(tmp.begin(), tmp.end(), blockCore[i][*iter].in.begin(), blockCore[i][*iter].in.end(), inserter(tmp, tmp.begin()));
				blockCore[i][j].out = tmp;
				set_difference(blockCore[i][j].out.begin(), blockCore[i][j].out.end(), blockCore[i][j].def.begin(), blockCore[i][j].def.end(), inserter(tmp, tmp.begin()));
				set_union(blockCore[i][j].use.begin(), blockCore[i][j].use.end(), tmp.begin(), tmp.end(), inserter(tmp, tmp.begin()));
				if (tmp.size() != blockCore[i][j].in.size()) {
					update = true;
					blockCore[i][j].in = tmp;
				}
				else {
					for (set<string>::iterator iter1 = tmp.begin(), iter2 = blockCore[i][j].in.begin(); iter1 != tmp.end(); iter1++, iter2++)
						if (*iter1 != *iter2) {
							update = true;
							blockCore[i][j].in = tmp;
							break;
						}
				}
			}
		}

	}
}

set<int> SSA::DF_Set(int funNum, set<int> s) {
	set<int> D;
	for (set<int>::iterator iter = s.begin(); iter != s.end(); iter++) {
		set<int> tmp;
		set_union(D.begin(), D.end(), blockCore[funNum][*iter].df.begin(), blockCore[funNum][*iter].df.end(), inserter(tmp, tmp.begin()));
		D = tmp;
	}
	return D;
}

// 计算函数内每个局部变量对应的迭代必经边界，用于\phi函数的插入，参考《高级编译器设计与实现》P186
void SSA::build_var_chain() {
	int i, j, k;
	int size1 = total.size();
	// 申请空间
	for (i = 0; i < size1; i++) {
		vector<varStruct> v;
		varChain.push_back(v);
	}
	// 添加变量
	for (i = 1; i < size1; i++) {
		int size2 = total[i].size();
		for (j = 0; j < size2; j++) {
			symbolTable st = total[i][j];
			if ((st.getForm() == VARIABLE || st.getForm() == PARAMETER) && st.getDimension() == 0) {
				// 不处理数组
				set<int> s;
				varChain[i].push_back(varStruct(st.getName(), s));
			}
		}
	}
	// 确定迭代必经边界
	// size1 = blockCore[i].size();		
	// 实际上，codetotal、total、blockCore等的一维大小应该都相同
	for (i = 1; i < size1; i++) {
		int size2 = varChain[i].size();
		for (j = 0; j < size2; j++) {	// 遍历该函数内的每个变量
			int size3 = blockCore[i].size();
			// 初始化寻找
			for (k = 1; k < size3; k++)
				if (blockCore[i][k].def2.find(varChain[i][j].name) != blockCore[i][k].def2.end())
					varChain[i][j].blockNums.insert(k);
			varChain[i][j].blockNums.insert(0);			// entry块
			bool change = true;
			set<int> S = varChain[i][j].blockNums;
			set<int> DFP = DF_Set(i, S);
			while (change) {
				change = false;
				set<int> tmp;
				set_union(S.begin(), S.end(), DFP.begin(), DFP.end(), inserter(tmp, tmp.begin()));
				set<int> D = DF_Set(i, tmp);
				if (D.size() != DFP.size()) {
					DFP = D;
					change = true;
				}
				else {
					for (set<int>::iterator iter1 = D.begin(), iter2 = DFP.begin(); iter1 != D.end(); iter1++, iter2++)
						if (*iter1 != *iter2) {
							DFP = D;
							change = true;
							break;
						}
				}
			}
			varChain[i][j].blockNums = DFP;
		}
	}
}

// 查找该节点开始对应的name变量的基本块位置
int SSA::phi_loc_block(int funNum, int blkNum, string name, vector<bool> visited, int insertBlk) {
	if (visited[blkNum] || blkNum == insertBlk) return 0;
	else visited[blkNum] = true;
	// 如果该基本块中定义了name变量
	if (blockCore[funNum][blkNum].def2.find(name) != blockCore[funNum][blkNum].def2.end()) 
		return blkNum;
	// 如果该基本块要插入的\phi函数中包含该变量，也算该基本块使用了该变量
	set<int> s3;  varStruct vs(name, s3);
	for (int i = 0; i < varChain[funNum].size(); i++) 
		if (varChain[funNum][i].name.compare(vs.name) == 0) {
			vs = varChain[funNum][i]; break;
		}
	if (vs.blockNums.find(blkNum) != vs.blockNums.end()) return blkNum;
	// 在该基本块的前驱节点中再继续找
	set<int> pred = blockCore[funNum][blkNum].pred;
	for (set<int>::iterator iter = pred.begin(); iter != pred.end(); iter++) {
		int t = phi_loc_block(funNum, *iter, name, visited, insertBlk);
		if (t != 0) { return t; }
	}
	return 0;
}

// 在需要添加基本块的开始添加\phi函数
void SSA::add_phi_function() {
	int i, j, k;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {
		int size2 = blockCore[i].size();
		// 申请空间
		for (j = 0; j < size2; j++) { vector<phiFun> v; blockCore[i][j].phi = v; }
		int size3 = varChain[i].size();
		// 遍历这个函数需要\phi函数的变量
		for (j = 0; j < size3; j++) {
			// 处理每一个添加phi的变量
			bool update = true;
			set<int> tmp_diff;
			while (update) {
				update = false;
				varStruct vs = varChain[i][j];
				set<int> tmp_ans;
				set_difference(vs.blockNums.begin(), vs.blockNums.end(), tmp_diff.begin(), tmp_diff.end(), inserter(tmp_ans, tmp_ans.begin()));
				varChain[i][j].blockNums = tmp_ans;
				vs = varChain[i][j];
				// 在每个需要插入\phi函数的基本块插入\phi函数
				for (set<int>::iterator iter = vs.blockNums.begin(); iter != vs.blockNums.end(); iter++) {
					// 初始化要插入的\phi函数
					vector<int> s1; vector<string> s2;  phiFun pf(vs.name, vs.name, s1, s2);
					// 在每个前序节点中查找该变量的基本块位置
					for (set<int>::iterator iter1 = blockCore[i][*iter].pred.begin(); iter1 != blockCore[i][*iter].pred.end(); iter1++) {
						// 记录该基本块是否被访问过
						vector<bool> visited;
						for (k = 0; k < size2; k++) visited.push_back(false);
						int t = phi_loc_block(i, *iter1, vs.name, visited, *iter);
						if (t != 0 && find(pf.blockNums.begin(), pf.blockNums.end(), t) == pf.blockNums.end())
							pf.blockNums.push_back(t);
					}
					if (!blockCore[i][*iter].phi.empty() && blockCore[i][*iter].phi.back().primaryName.compare(pf.primaryName) == 0)
						blockCore[i][*iter].phi.pop_back();
					if (pf.blockNums.size() > 1) {
					// 在基本块中插入
						blockCore[i][*iter].phi.push_back(pf);
					}
					else {
						// 若\phi函数参数只有一个值则不用插入 
						update = true;
						tmp_diff.insert(*iter);
					}
				}
			}
		}	
	}
}

// 变量重命名
void SSA::renameVar() {
	int i, j, k;
	int size1 = blockCore.size();
	map<string, int> varPool;
	for (i = 1; i < size1; i++) {
		// 建立该函数的变量池，同时指定变量的下标
		// map<string, int> varPool;
		int size2 = total[i].size();
		for (j = 0; j < size2; j++)
			if ((total[i][j].getForm() == VARIABLE || total[i][j].getForm() == PARAMETER) && total[i][j].getDimension() == 0 && varPool.find(total[i][j].getName()) == varPool.end())
				varPool[total[i][j].getName()] = 0;
		// 记录每个基本块中最后出现的变量名
		map<string, map<int, string>> lastVarName;	// <varName, <blkNum, varName_SSA>>
		for (map<string, int>::iterator iter = varPool.begin(); iter != varPool.end(); iter++) { map<int, string> m;  lastVarName[iter->first] = m; }
		// 以基本块为单位遍历, j 是基本块编号
		int size3 = blockCore[i].size();
		for (j = 1; j < size3; j++) {
			// 1. 首先更新基本块开始的 \phi函数定义
			int size4 = blockCore[i][j].phi.size();
			for (k = 0; k < size4; k++) {
				phiFun pf = blockCore[i][j].phi[k];
				string tmp = pf.name + "^" + to_string(varPool[pf.name]);
				blockCore[i][j].phi[k].name = tmp;
				varPool[pf.name] = varPool[pf.name] + 1;
				lastVarName[pf.name][j] = tmp;
			}
			// 2. 更新中间代码中新定义的变量
			int size5 = blockCore[i][j].Ir.size();
			for (k = 0; k < size5; k++) {
				CodeItem ci = blockCore[i][j].Ir[k];
				if (ifNotToDeal(ci.getCodetype())) {
					;
				}
				else if (ifOp1Def(ci.getCodetype())) {
					// op1是新定义的变量，result和op2是使用的变量
					if (varPool.find(ci.getOperand1()) != varPool.end()) {
						string tmp = ci.getOperand1() + "^" + to_string(varPool[ci.getOperand1()]);
						CodeItem nci(ci.getCodetype(), ci.getResult(), tmp, ci.getOperand2());
						blockCore[i][j].Ir[k] = nci;
						varPool[ci.getOperand1()] = varPool[ci.getOperand1()] + 1;
						lastVarName[ci.getOperand1()][j] = tmp;
					}
				}
				else if (ifResultDef(ci.getCodetype())) {
					// result是新定义的变量，op1和op2是使用的变量
					if (varPool.find(ci.getResult()) != varPool.end()) {
						string tmp = ci.getResult() + "^" + to_string(varPool[ci.getResult()]);
						CodeItem nci(ci.getCodetype(), tmp, ci.getOperand1(), ci.getOperand2());
						blockCore[i][j].Ir[k] = nci;
						varPool[ci.getResult()] = varPool[ci.getResult()] + 1;
						lastVarName[ci.getResult()][j] = tmp;
					}
				}
			}
		}
		// 以基本块为单位遍历, j 是基本块编号
		bool update = true;
		while (update) {
			update = false;
			for (j = 1; j < size3; j++) {
				// 3. 更新phi的使用
				for (k = 0; k < blockCore[i][j].phi.size(); k++) {
					phiFun pf = blockCore[i][j].phi[k];
					blockCore[i][j].phi[k].subIndexs.clear();
					for (vector<int>::iterator iter = pf.blockNums.begin(); iter != pf.blockNums.end(); iter++)
						if (lastVarName[pf.primaryName].find(*iter) != lastVarName[pf.primaryName].end())
							blockCore[i][j].phi[k].subIndexs.push_back(lastVarName[pf.primaryName][*iter]);
				}
				// 4. 删除无用的phi函数
				for (k = 0; k < blockCore[i][j].phi.size(); k++) {
					phiFun pf = blockCore[i][j].phi[k];
					if (pf.subIndexs.size() <= 1) {	// 如果对应的选择只有一个则不用添加phi函数
						blockCore[i][j].phi.erase(blockCore[i][j].phi.begin() + k);
						if (lastVarName[pf.primaryName].find(j) != lastVarName[pf.primaryName].end() &&
							lastVarName[pf.primaryName][j].compare(pf.name) == 0)
							lastVarName[pf.primaryName].erase(j);
						k--;
						update = true;
						cout << "delete" << pf.primaryName << " " << pf.name;
						cout << " " << i << " " << j;
						for (vector<int>::iterator iter = pf.blockNums.begin(); iter != pf.blockNums.end(); iter++) cout << " " << *iter;
						for (vector<string>::iterator iter = pf.subIndexs.begin(); iter != pf.subIndexs.end(); iter++) cout << " " << *iter;
						cout << endl;
						continue;
					}
				}
			}
		}
		// 以基本块为单位遍历, j 是基本块编号
		for (j = 1; j < size3; j++) {
			int size6 = blockCore[i][j].Ir.size();
			// 记录每个变量在基本块中出现的次序
			map<string, vector<string>> varSequence;
			for (map<string, int>::iterator iter = varPool.begin(); iter != varPool.end(); iter++) {
				vector<string> v;
				varSequence[iter->first] = v;
			}
			for (vector<phiFun>::iterator iter = blockCore[i][j].phi.begin(); iter != blockCore[i][j].phi.end(); iter++)
				varSequence[iter->primaryName].push_back(iter->name);
			// 5. 更新中间代码中的使用
			for (k = 0; k < size6; k++) {
				CodeItem ci = blockCore[i][j].Ir[k];
				if (ifNotToDeal(ci.getCodetype())) {
					;
				}
				else if (ifOp1Def(ci.getCodetype())) {
					// op1是新定义的变量，result和op2是使用的变量
					varSequence[deleteSuffix(ci.getOperand1())].push_back(ci.getOperand1());
					// 更新result的变量名
					ci = blockCore[i][j].Ir[k];
					if (varPool.find(ci.getResult()) != varPool.end()) {
						string tmp = ci.getResult();
						if (varSequence.find(ci.getResult()) != varSequence.end() && !varSequence[ci.getResult()].empty())
							tmp = varSequence[ci.getResult()].back();
						else {
							vector<int> queue;
							for (set<int>::iterator iter1 = blockCore[i][j].pred.begin(); iter1 != blockCore[i][j].pred.end(); iter1++) queue.push_back(*iter1);
							int flag = 0;
							for (int p = 0; p < queue.size(); p++) {
								if (lastVarName[ci.getResult()].find(queue[p]) != lastVarName[ci.getResult()].end()) { tmp = lastVarName[ci.getResult()][queue[p]]; flag++;  break; }
								for (set<int>::iterator iter2 = blockCore[i][queue[p]].pred.begin(); iter2 != blockCore[i][queue[p]].pred.end(); iter2++)
									if (find(queue.begin(), queue.end(), *iter2) == queue.end()) queue.push_back(*iter2);
							}
							if (flag == 0) cout << "Wrong! Empty! result = " << "\t" << ci.getResult() << "\t" << ci.getContent() << endl;
						}
						CodeItem nci(ci.getCodetype(), tmp, ci.getOperand1(), ci.getOperand2());
						blockCore[i][j].Ir[k] = nci;
						varSequence[ci.getResult()].push_back(tmp);
					}
					// 更新op2的变量名
					ci = blockCore[i][j].Ir[k];
					if (varPool.find(ci.getOperand2()) != varPool.end()) {
						string tmp = ci.getOperand2();
						if (lastVarName[ci.getOperand2()].find(j) != lastVarName[ci.getOperand2()].end()) tmp = lastVarName[ci.getOperand2()][j];
						else {
							vector<int> queue;
							for (set<int>::iterator iter1 = blockCore[i][j].pred.begin(); iter1 != blockCore[i][j].pred.end(); iter1++) queue.push_back(*iter1);
							int flag = 0;
							for (int p = 0; p < queue.size(); p++) {
								if (lastVarName[ci.getOperand2()].find(queue[p]) != lastVarName[ci.getOperand2()].end()) { tmp = lastVarName[ci.getOperand2()][queue[p]]; flag++; break; }
								for (set<int>::iterator iter2 = blockCore[i][queue[p]].pred.begin(); iter2 != blockCore[i][queue[p]].pred.end(); iter2++)
									if (find(queue.begin(), queue.end(), *iter2) == queue.end()) queue.push_back(*iter2);
							}
							if (flag == 0) cout << "Wrong! Empty! op2 = " << "\t" << ci.getOperand2() << "\t" << ci.getContent() << endl;
						}
						CodeItem nci(ci.getCodetype(), ci.getResult(), ci.getOperand1(), tmp);
						blockCore[i][j].Ir[k] = nci;
						varSequence[ci.getOperand2()].push_back(tmp);
					}
				}
				else if (ifResultDef(ci.getCodetype())) {
					// result是新定义的变量，op1和op2是使用的变量
					varSequence[deleteSuffix(ci.getResult())].push_back(ci.getResult());
					// 更新op1的变量名
					ci = blockCore[i][j].Ir[k];
					if (varPool.find(ci.getOperand1()) != varPool.end()) {
						string tmp = ci.getOperand1();
						if (varSequence.find(ci.getOperand1()) != varSequence.end() && !varSequence[ci.getOperand1()].empty())
							tmp = varSequence[ci.getOperand1()].back();
						else {
							vector<int> queue;
							for (set<int>::iterator iter1 = blockCore[i][j].pred.begin(); iter1 != blockCore[i][j].pred.end(); iter1++) queue.push_back(*iter1);
							int flag = 0;
							for (int p = 0; p < queue.size(); p++) {
								if (lastVarName[ci.getOperand1()].find(queue[p]) != lastVarName[ci.getOperand1()].end()) { tmp = lastVarName[ci.getOperand1()][queue[p]]; flag++;  break; }
								for (set<int>::iterator iter2 = blockCore[i][queue[p]].pred.begin(); iter2 != blockCore[i][queue[p]].pred.end(); iter2++)
									if (find(queue.begin(), queue.end(), *iter2) == queue.end()) queue.push_back(*iter2);
							}
							if (flag == 0) cout << "Wrong! Empty! op1 = " << "\t" << ci.getOperand1() << "\t" << ci.getContent() << endl;
						}
						CodeItem nci(ci.getCodetype(), ci.getResult(), tmp, ci.getOperand2());
						blockCore[i][j].Ir[k] = nci;
						varSequence[ci.getOperand1()].push_back(tmp);
					}
					// 更新op2的变量名
					ci = blockCore[i][j].Ir[k];
					if (varPool.find(ci.getOperand2()) != varPool.end()) {
						string tmp = ci.getOperand2();
						if (lastVarName[ci.getOperand2()].find(j) != lastVarName[ci.getOperand2()].end()) tmp = lastVarName[ci.getOperand2()][j];
						else {
							vector<int> queue;
							for (set<int>::iterator iter1 = blockCore[i][j].pred.begin(); iter1 != blockCore[i][j].pred.end(); iter1++) queue.push_back(*iter1);
							int flag = 0;
							for (int p = 0; p < queue.size(); p++) {
								if (lastVarName[ci.getOperand2()].find(queue[p]) != lastVarName[ci.getOperand2()].end()) { tmp = lastVarName[ci.getOperand2()][queue[p]]; flag++; break; }
								for (set<int>::iterator iter2 = blockCore[i][queue[p]].pred.begin(); iter2 != blockCore[i][queue[p]].pred.end(); iter2++)
									if (find(queue.begin(), queue.end(), *iter2) == queue.end()) queue.push_back(*iter2);
							}
							if (flag == 0) cout << "Wrong! Empty! op2_2 = " << "\t" << ci.getOperand2() << "\t" << ci.getContent() << endl;
						}
						CodeItem nci(ci.getCodetype(), ci.getResult(), ci.getOperand1(), tmp);
						blockCore[i][j].Ir[k] = nci;
						varSequence[ci.getOperand2()].push_back(tmp);
					}
				}
			}
		}
	}
}

// 删除后缀，如输入参数为% a ^ 1，返回 % a; 若不含^ 则直接返回；
string SSA::deleteSuffix(string name) {
	if (name.find("^") != string::npos)
		name.erase(name.begin() + name.find("^"), name.end());
	return name;
}

// 将SSA变量带下标的中间代码恢复为正常中间代码，即做完优化后去掉下标
void SSA::rename_back() {
	int i, j, k;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {	// 遍历函数
		int size2 = blockCore[i].size();
		for (j = 1; j < size2 - 1; j++) {	// 遍历基本块，跳过entry和exit块
			int size3 = blockCore[i][j].Ir.size();
			for (k = 0; k < size3; k++) {	// 遍历基本块中的中间代码并进行修改
				CodeItem ci = blockCore[i][j].Ir[k];
				string result = deleteSuffix(ci.getResult());
				string op1 = deleteSuffix(ci.getOperand1());
				string op2 = deleteSuffix(ci.getOperand2());
				CodeItem nci(ci.getCodetype(), result, op1, op2);
				blockCore[i][j].Ir[k] = nci;
			}
		}
	}
	// 将中间代码存回codetotal
	//for (int i = 1; i < size1; i++) {		// 遍历函数
	//	codetotal[i].clear();
	//	int size2 = blockCore[i].size();
	//	for (int j = 1; j < size2 - 1; j++) {	// 遍历基本块，跳过entry和exit块
	//		codetotal[i].insert(codetotal[i].end(), blockCore[i][j].Ir.begin(), blockCore[i][j].Ir.end());
	//	}
	//}
}

void SSA::turn_back_codetotal() {
	// 将中间代码存回codetotal
	for (int i = 1; i < blockCore.size(); i++) {		// 遍历函数
		codetotal[i].clear();
		int size2 = blockCore[i].size();
		for (int j = 1; j < size2 - 1; j++) {	// 遍历基本块，跳过entry和exit块
			codetotal[i].insert(codetotal[i].end(), blockCore[i][j].Ir.begin(), blockCore[i][j].Ir.end());
		}
	}
}

// 处理\phi函数
void SSA::deal_phi_function() {
	int i, j, k, m, n;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {	// 遍历函数，跳过全局定义
		int size2 = blockCore[i].size();
		for (j = 1; j < size2 - 1; j++) {	// 遍历基本块，跳过entry块和exit块
			for (k = 0; k < blockCore[i][j].phi.size(); k++) {
				phiFun pf = blockCore[i][j].phi[k];
				if (pf.subIndexs.size() <= 1) {	// 如果对应的选择只有一个则不用添加phi函数
					cout << pf.primaryName << " " << pf.name << " " << pf.subIndexs.back();
					for (vector<int>::iterator iter10 = pf.blockNums.begin(); iter10 != pf.blockNums.end(); iter10++)
						cout << " " << *iter10;
					cout << endl;
					blockCore[i][j].phi.erase(blockCore[i][j].phi.begin() + k);
					k--;
					continue;
				}
				for (m = 0; m < pf.blockNums.size(); m++) {
					// e.g. x3 = \phi(x1, x2)
					// 在x1和x2的基本块末尾分别添加x3 = x1; x3 = x2; 赋值语句
					// x3 = x1;的赋值语句转换为中间代码为 load %0, x1; store %0, x3; 两条语句
					int tTempIndex = func2tmpIndex[i];
					string tTemp = "%" + to_string(tTempIndex);
					func2tmpIndex[i] = tTempIndex + 1;
					CodeItem ci1(LOAD, tTemp, pf.subIndexs[m], "");
					CodeItem ci2(STORE, tTemp, pf.name, "");
					if (!blockCore[i][pf.blockNums[m]].Ir.empty() && 
						(blockCore[i][pf.blockNums[m]].Ir.back().getCodetype() == BR || blockCore[i][pf.blockNums[m]].Ir.back().getCodetype() == RET)) {
						// 如果要插入的基本块最后一条中间代码是跳转指令，则插在它的前面
						CodeItem ci3 = blockCore[i][pf.blockNums[m]].Ir.back();
						blockCore[i][pf.blockNums[m]].Ir.pop_back();
						blockCore[i][pf.blockNums[m]].Ir.push_back(ci1);
						blockCore[i][pf.blockNums[m]].Ir.push_back(ci2);
						blockCore[i][pf.blockNums[m]].Ir.push_back(ci3);
					}
					else {	// 否则插到基本块的最后
						blockCore[i][pf.blockNums[m]].Ir.push_back(ci1);
						blockCore[i][pf.blockNums[m]].Ir.push_back(ci2);
					}
				}
			}
		}
	}
}

// 消除无法到达基本块
void SSA::simplify_basic_block() {
	int i, j, k;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {
		bool update = true;
		while (update) {
			update = false;
			vector<basicBlock> v = blockCore[i];
			int size2 = v.size();for (j = 1; j < size2; j++) if (v[j].pred.empty()) { update = true; break; }
			if (update) {
				v.erase(v.begin() + j);
				int size3 = v.size();
				for (k = 0; k < size3; k++) {
					// 修改序号
					if (v[k].number > j) v[k].number = v[k].number - 1;
					// 修改pred
					set<int> pred;
					for (set<int>::iterator iter = v[k].pred.begin(); iter != v[k].pred.end(); iter++)
						if (*iter < j) pred.insert(*iter);
						else if (*iter > j) pred.insert((*iter) - 1);
						else continue;
					v[k].pred = pred;
					// 修改succeeds
					set<int> succeeds;
					for (set<int>::iterator iter = v[k].succeeds.begin(); iter != v[k].succeeds.end(); iter++)
						if (*iter < j) succeeds.insert(*iter);
						else if (*iter > j) succeeds.insert((*iter) - 1);
						else continue;
					v[k].succeeds = succeeds;
				}
			}
			blockCore[i] = v;
		}
	}
}

// 将phi函数加入到中间代码
void SSA::add_phi_to_Ir() {
	int i, j, k;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {	// 遍历函数
		int size2 = blockCore[i].size();
		for (j = 1; j < size2; j++) {	// 遍历基本块
			int size3 = blockCore[i][j].phi.size();
			for (int k = 0; k < size3; k++) {	// 遍历phi函数
				phiFun phi = blockCore[i][j].phi[k];
				CodeItem ci(PHI, phi.name, "", "");
				blockCore[i][j].Ir.insert(blockCore[i][j].Ir.begin(), ci);
			}
		}
	}
}

// 删除中间代码中的phi
void SSA::delete_Ir_phi() {
	int i, j, k;
	int size1 = blockCore.size();
	for (i = 1; i < size1; i++) {	// 遍历函数
		int size2 = blockCore[i].size();
		for (j = 1; j < size2; j++) {	// 遍历基本块
			auto& ir = blockCore[i][j].Ir;
			for (auto it = ir.begin(); it != ir.end();) {	// 遍历中间代码
				if (it->getCodetype() == PHI) {
					it = ir.erase(it);
					continue;
				}
				it++;
			}
		}
	}
}

// 入口函数
void SSA::generate() {

	// 简化条件判断为常值的跳转指令
	simplify_br();

	// 在睿轩生成的中间代码上做优化
	pre_optimize();

	// 计算每个基本块的起始语句
	find_primary_statement();
	// 为每个函数划分基本块
	divide_basic_block();
	// 建立基本块间的前序和后序关系
	build_pred_and_succeeds();
	// 消除无法到达基本块
	simplify_basic_block();

	// 确定每个基本块的必经关系，参见《高级编译器设计与实现》P132 Dom_Comp算法
	build_dom_tree();

	// 确定每个基本块的直接必经关系，参见《高级编译器设计与实现》P134 Idom_Comp算法
	build_idom_tree();

	// 根据直接必经节点找到必经节点树中每个节点的后序节点
	build_reverse_idom_tree();

	// 后序遍历必经节点树
	build_post_order();

	// 前序遍历必经节点树
	build_pre_order();

	// 计算流图必经边界，参考《高级编译器设计与实现》 P185 Dom_Front算法
	build_dom_frontier();

	// 计算ud链，即分析每个基本块的use和def变量
	build_def_use_chain();

	// 活跃变量分析，生成in、out集合
	active_var_analyse();

	// 计算函数内每个局部变量对应的迭代必经边界，用于\phi函数的插入，参考《高级编译器设计与实现》P186
	build_var_chain();

	// 在需要添加基本块的开始添加\phi函数
	add_phi_function();
	// 变量重命名
	renameVar();
	
	// 处理\phi函数
	deal_phi_function();

	// 优化
	ssa_optimize();

	// 测试输出上面各个函数
	Test_SSA();

	// 恢复变量命名
	rename_back();

	//复写传播
	// copy_propagation();

	//将SSA格式代码转换到codetotal格式
	turn_back_codetotal();

	// 恢复为之前中间代码形式后再做一次无用代码删除
	pre_optimize();

	// 输出中间代码
	TestIrCode("ir2.txt");

}

void SSA::generate_activeAnalyse() {

	// 简化条件判断为常值的跳转指令
	simplify_br();

	// 在睿轩生成的中间代码上做优化
	pre_optimize();

	// 计算每个基本块的起始语句
	find_primary_statement();
	// 为每个函数划分基本块
	divide_basic_block();
	// 建立基本块间的前序和后序关系
	build_pred_and_succeeds();
	// 消除无法到达基本块
	simplify_basic_block();

	// 计算ud链，即分析每个基本块的use和def变量
	build_def_use_chain();

	// 活跃变量分析，生成in、out集合
	active_var_analyse();
}

