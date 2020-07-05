#include "ssa.h"
#include <regex>
#include <algorithm>
#include <set>
#include <fstream>

using namespace std;
ofstream debug_ssa;

// 用于输出的显示效果
string standardOutput(string str)
{
	while (str.size() < 15) {
		str = str + " ";
	}
	return str;
}

// 判断是否是临时变量
// 可用于通过判断br语句的result操作数选出相应的标签名称
// br tmpReg label1 label2
// br label
bool ifTempVariable(string name) {
	regex r("%\\d+");
	return regex_match(name, r);
}

// 判断字符串是否是数字
bool ifDigit(string name) {
	regex r("(\\+|\\-)?\\d+");
	return regex_match(name, r);
}

// 判断字符串是否是全局变量
bool ifGlobalVariable(string name) {
	regex r("@.+");
	return regex_match(name, r);
}

// 判断是否是局部变量
bool ifLocalVariable(string name) {
	regex r("%.+");
	return regex_match(name, r);
}

// 在一个函数的所有中间代码中找到某个标签的位置，即它是第几条中间代码
// @result: 0, 1, 2, ..., size-1(size代表该函数总的中间代码条数), -1(表示没有找到，lzh在调用该函数时正常情况下不应出现该情况)
int lookForLabel(vector<CodeItem> v, string label) {
	int size = v.size();
	int i;
	for (i = 0; i < size; i++) {
		if (v[i].getCodetype() == LABEL && v[i].getResult() == label) return i;
	}
	return -1;
}

// @para key: 传入的是上面lookForLabel函数查出的label标签对应的中间代码序号
// 用于查找某个函数中第一条语句序号是key的基本块，即找出哪个基本块是以上面函数查到的label标签为第一条语句
int locBlockForLabel(vector<int> v, int key) {
	int size = v.size();
	for (int i = 0; i < size; i++) if (v[i] == key) return i;
	return -1;
}

// 为每个函数划分基本块
void SSA::divide_basic_block() {
	int size1 = codetotal.size();
	int i, j, k;
	// 跳过全局定义
	vector<int> v0 = {}; blockOrigin.push_back(v0);
	// 函数定义
	for (i = 1; i < size1; i++)
	{
		vector<CodeItem> v = codetotal[i];
		int size2 = v.size();
		vector<int> tmp = { 0, 1 };		// tmp用于保存所有基本块的起始语句：0代表entry；1代表函数第一条中间代码
		for (j = 0; j < size2; j++) {	// 第几条中间代码(1, 2, 3...)
			CodeItem c = v[j];
			if (c.getCodetype() == BR) {			// 跳转指令
				if (ifTempVariable(c.getResult())) {
					k = lookForLabel(v, c.getOperand1());
					if (find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1); // 第几条中间代码，从1开始，因此push进去的要加1，后面同理
					k = lookForLabel(v, c.getOperand2());
					if (find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
				}
				else {
					k = lookForLabel(v, c.getResult());
					if (find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
				}
				k = j + 1;		// 分支指令的下一条指令
				if (find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
			}
			else if (c.getCodetype() == RET) { // 返回指令
				k = j + 1;		// 分支指令的下一条指令
				if (k + 1 <= size2 && find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
			}
		}
		tmp.push_back(size2 + 1);		// exit块
		sort(tmp.begin(), tmp.end());	// 对所有入口语句从小到大排序
		blockOrigin.push_back(tmp);
	}

	// 跳过全局定义
	vector<basicBlock> vv0; blockCore.push_back(vv0);
	// 函数定义
	for (i = 1; i < size1; i++)
	{
		// 申请空间
		vector<int> v = blockOrigin[i];
		vector<basicBlock> tmp;
		// entry块
		basicBlock bb1(0, 0, 0, {},  {});		// 序号为0，起始和终止语句均为0
		tmp.push_back(bb1);
		int size2 = v.size();
		// 函数中间代码划分的基本块
		for (j = 1; j < size2 - 1; j++) {
			basicBlock bb(j, v[j], v[j + 1] - 1, {}, {});	// 序号1, 2, 3...，起始语句为当前的入口语句，到下一条入口语句的前一条为止
			tmp.push_back(bb);
		}
		// 初始化entry块的后序节点为1，第1个基本块的前序节点为entry块即0
		tmp[0].succeeds.insert(0);
		tmp[1].pred.insert(0);
		// exit块
		basicBlock bb2(j, v[j], v[j], {}, {});	// 前序节点无需初始化，后序节点为空
		tmp.push_back(bb2);
		// 建立关系
		for (j = 1; j < size2 - 1; j++) {
			if (codetotal[i][tmp[j].end - 1].getCodetype() == BR) {	// 跳转指令
				// 找到跳转到标签所在的基本块序号，插入当前基本块的后序节点，同时为跳转到的基本块前序节点插入当前基本块的序号
				if (ifTempVariable(codetotal[i][tmp[j].end - 1].getResult())) {
					k = lookForLabel(codetotal[i], codetotal[i][tmp[j].end - 1].getOperand1());
					k = locBlockForLabel(v, k + 1);
					tmp[j].succeeds.insert(k);
					tmp[k].pred.insert(j);
					k = lookForLabel(codetotal[i], codetotal[i][tmp[j].end - 1].getOperand2());
					k = locBlockForLabel(v, k + 1);
					tmp[j].succeeds.insert(k);
					tmp[k].pred.insert(j);
				}
				else {
					k = lookForLabel(codetotal[i], codetotal[i][tmp[j].end - 1].getResult());
					k = locBlockForLabel(v, k + 1);
					tmp[j].succeeds.insert(k);
					tmp[k].pred.insert(j);
				}
			}
			else if (codetotal[i][tmp[j].end - 1].getCodetype() == RET) {	// 返回指令
				// 该基本块跳转到exit块
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
		blockCore.push_back(tmp); 
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
		for (j = 1; j < size2; j++) blockCore[i][j].domin.insert(N.begin(), N.end());
		bool change;
		do {
			change = false;
			// for each n excpt r
			for (j = 1; j < size2; j++) {
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
		} while (change);
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
			blockCore[i][j].idom = tmp3;
		}
		// foreach n
		for (j = 1; j < size2; j++) {
			// foreach s
			set<int> tmp;
			for (set<int>::iterator iter1 = blockCore[i][j].tmpIdom.begin(); iter1 != blockCore[i][j].tmpIdom.end(); iter1++) {
				set<int> tmp1, tmp2;
				tmp1.insert(*iter1);
				set_difference(blockCore[i][j].tmpIdom.begin(), blockCore[i][j].tmpIdom.end(), tmp1.begin(), tmp1.end(), inserter(tmp2, tmp2.begin()));
				// foreach t
				for (set<int>::iterator iter2 = tmp2.begin(); iter2 != tmp2.end(); iter2++) {
					if (blockCore[i][*iter1].tmpIdom.find(*iter2) != blockCore[i][*iter1].tmpIdom.end()) {
						tmp.insert(*iter2);
					}
				}
			}
			set<int> tmp3;
			set_difference(blockCore[i][j].tmpIdom.begin(), blockCore[i][j].tmpIdom.end(), tmp.begin(), tmp.end(), inserter(tmp3, tmp3.begin()));;
			blockCore[i][j].tmpIdom = tmp3;
		}
		// foreach n
		for (j = 1; j < size2; j++) 
			if (!blockCore[i][j].tmpIdom.empty()) 
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
		int size2 = preOrder[i].size();
		for (j = 0; j < size2; j++) {
			int p_i = preOrder[i][j];
			set<int> df;
			for (set<int>::iterator iter = blockCore[i][p_i].succeeds.begin(); iter != blockCore[i][p_i].succeeds.end(); iter++) {
				if (blockCore[i][p_i].idom.find(*iter) != blockCore[i][p_i].idom.end()) df.insert(*iter);
			}
			for (set<int>::iterator iter1 = blockCore[i][p_i].idom.begin(); iter1 != blockCore[i][p_i].idom.end(); iter1++) {
				for (set<int>::iterator iter2 = blockCore[i][*iter1].df.begin(); iter2 != blockCore[i][*iter1].df.end(); iter2++) {
					if (blockCore[i][p_i].idom.find(*iter2) != blockCore[i][p_i].idom.end()) df.insert(*iter2);
				}
			}
			blockCore[i][p_i].df = df;
		}
	}
}

// 在基本块的use链中插入变量名称
void SSA::use_insert(int funNum, int blkNum, string varName) {
	if (varName == "") return;
	// if (ifTempVariable(varName)) return;							// 不加入临时变量
	// if (ifDigit(varName)) return;										// 前端可能直接做好部分优化，操作数这里可能是常数
	if (ifLocalVariable(varName)) blockCore[funNum][blkNum].use.insert(varName);	// 不加入全局变量
}

// 在基本块的def链中插入变量名称
void SSA::def_insert(int funNum, int blkNum, string varName) {
	if (varName == "") return;
	// if (ifTempVariable(varName)) return;							// 不加入临时变量
	// if (ifDigit(varName)) return;										// 前端可能直接做好部分优化，操作数这里可能是常数
	if(ifLocalVariable(varName)) blockCore[funNum][blkNum].def.insert(varName);	// 不加入全局变量
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
			for (k = blockCore[i][j].start; k <= blockCore[i][j].end; k++) {
				if (k - 1 < 0 || k - 1 >= codetotal[i].size()) continue;
				CodeItem ci = codetotal[i][k - 1];
				switch (ci.getCodetype())
				{
				case ADD: case SUB: case MUL: case DIV: case REM:
					use_insert(i, j, ci.getOperand1());
					use_insert(i, j, ci.getOperand2());
					def_insert(i, j, ci.getResult());
					break;
				case AND: case OR: case NOT: case EQL: case NEQ: case SGT: case SGE: case SLT: case SLE:
					use_insert(i, j, ci.getOperand1());
					use_insert(i, j, ci.getOperand2());
					def_insert(i, j, ci.getResult());
					break;
				case ALLOC: case GLOBAL:
					break;
				case STORE: case STOREARR:
					use_insert(i, j, ci.getResult());
					def_insert(i, j, ci.getOperand1());			// 变量名或者数组名
					break;
				case LOAD: case LOADARR:
					use_insert(i, j, ci.getOperand1());			// 变量名或者数组名
					def_insert(i, j, ci.getResult());
					break;
				case RET:
					use_insert(i, j, ci.getResult());			// 函数返回值
					break;
				case PUSH:
					use_insert(i, j, ci.getResult());
					break;
				case POP:
					def_insert(i, j, ci.getResult());
					break;
				case BR:
					/* 特殊判断，只添加中间变量 */
					if (ifTempVariable(ci.getResult())) use_insert(i, j, ci.getResult());
					break;
				/* MOV在中间代码中应该还没有
				case MOV:
					break; */
				case CALL: case LABEL: case DEFINE: case PARA:
					break;
				default:
					break;
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

// 入口函数
void SSA::generate() {
	debug_ssa.open("debug_ssa.txt");
	/* Test_* 函数用于对每个函数进行测试，输出相关信息 */
	// 为每个函数划分基本块
	divide_basic_block();							
	Test_Divide_Basic_Block();
	// 确定每个基本块的必经关系，参见《高级编译器设计与实现》P132 Dom_Comp算法
	build_dom_tree();				
	Test_Build_Dom_Tree();
	// 确定每个基本块的直接必经关系，参见《高级编译器设计与实现》P134 Idom_Comp算法
	build_idom_tree();								
	Test_Build_Idom_Tree();
	// 根据直接必经节点找到必经节点树中每个节点的后序节点
	build_reverse_idom_tree();				
	Test_Build_Reverse_Idom_Tree();
	// 后序遍历必经节点树
	build_post_order();								
	Test_Build_Post_Order();
	// 前序遍历必经节点树
	build_pre_order();			
	Test_Build_Pre_Order();
	// 计算流图必经边界，参考《高级编译器设计与实现》 P185 Dom_Front算法
	build_dom_frontier();
	Test_Build_Dom_Frontier();
	// 计算ud链，即分析每个基本块的use和def变量
	build_def_use_chain();					
	Test_Build_Def_Use_Chain();
	// 活跃变量分析，生成in、out集合
	active_var_analyse();							
	Test_Active_Var_Analyse();
	debug_ssa.close();
}

// 测试函数：输出所有的基本块信息
void SSA::Test_Divide_Basic_Block() {
	debug_ssa << "---------------- basic block -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		// 依次输出该函数下所有的基本块信息
		debug_ssa << "基本块编号" << "\t\t" << "入口中间代码" << "\t\t" << "结束中间代码" << "\t\t" << "前序节点" << "\t\t" << "后序节点" << "\t\t" << endl;
		for (int j = 0; j < size2; j++) {
			debug_ssa << v[i][j].number << "\t\t";	// 基本块编号
			debug_ssa << v[i][j].start << "\t\t";		// 入口中间代码
			debug_ssa << v[i][j].end << "\t\t";		// 结束中间代码
			set<int>::iterator iter;
			 // 即可以跳转到该基本块的基本块序号
			debug_ssa << "{ ";
			for (iter = v[i][j].pred.begin(); iter != v[i][j].pred.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << "\t\t";
			 // 即通过该基本块可以跳转到的基本块序号
			debug_ssa << "{ ";
			for (iter = v[i][j].succeeds.begin(); iter != v[i][j].succeeds.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

// 测试函数：输出构建的必经节点
void SSA::Test_Build_Dom_Tree() {
	debug_ssa << "---------------- dom tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "该基本块编号: " << v[i][j].number << "\t\t";
			debug_ssa << "该基本块的必经节点: {  ";
			for (set<int>::iterator iter = v[i][j].domin.begin(); iter != v[i][j].domin.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

// 测试函数：输出直接必经节点
void SSA::Test_Build_Idom_Tree() {
	debug_ssa << "---------------- idom tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "该基本块的编号: " << v[i][j].number << "\t\t";
			debug_ssa << "该基本块的直接必经节点: {  ";
			for (set<int>::iterator iter = v[i][j].idom.begin(); iter != v[i][j].idom.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

// 测试函数：输出反向必经节点
void SSA::Test_Build_Reverse_Idom_Tree() {
	debug_ssa << "---------------- reverse idom tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "该基本块的编号: " << v[i][j].number << "\t\t";
			debug_ssa << "该基本块反向直接必经节点: {  ";
			for (set<int>::iterator iter = v[i][j].reverse_idom.begin(); iter != v[i][j].reverse_idom.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

// 测试函数：输出后序遍历序列
void SSA::Test_Build_Post_Order() {
	debug_ssa << "---------------- post order -----------------" << endl;
	vector<vector<int>> v = postOrder;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		debug_ssa << "后序遍历序列" << "\t\t";
		for (int j = 0; j < size2; j++) debug_ssa << v[i][j] << "\t\t";
		debug_ssa << endl;
	}
}

// 测试函数：输出前序遍历序列
void SSA::Test_Build_Pre_Order() {
	debug_ssa << "---------------- pre order -----------------" << endl;
	vector<vector<int>> v = preOrder;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		debug_ssa << "前序遍历序列" << "\t\t";
		for (int j = 0; j < size2; j++) debug_ssa << v[i][j] << "\t\t";
		debug_ssa << endl;
	}
}

// 测试函数：输出必经边界
void SSA::Test_Build_Dom_Frontier() {
	debug_ssa << "---------------- df tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "该基本块的编号: " << v[i][j].number << "\t\t";
			debug_ssa << "该基本块的必经边界: {  ";
			for (set<int>::iterator iter = v[i][j].df.begin(); iter != v[i][j].df.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

void SSA::Test_Build_Def_Use_Chain() {
	debug_ssa << "---------------- def-use chain -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "该基本块的编号: " << v[i][j].number << endl;
			debug_ssa << "该基本块的def变量: {  ";
			for (set<string>::iterator iter = v[i][j].def.begin(); iter != v[i][j].def.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
			debug_ssa << "该基本块的use变量: {  ";
			for (set<string>::iterator iter = v[i][j].use.begin(); iter != v[i][j].use.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

void SSA::Test_Active_Var_Analyse() {
	debug_ssa << "---------------- active var analyse -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "该基本块的编号: " << v[i][j].number << endl;
			debug_ssa << "该基本块的in集合: {  ";
			for (set<string>::iterator iter = v[i][j].in.begin(); iter != v[i][j].in.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
			debug_ssa << "该基本块的out集合: {  ";
			for (set<string>::iterator iter = v[i][j].out.begin(); iter != v[i][j].out.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}