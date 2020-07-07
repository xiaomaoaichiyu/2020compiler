#include "ssa.h"
#include <regex>
#include <algorithm>
#include <set>
#include <fstream>

using namespace std;

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

// 判断是否是局部变量，包含临时变量
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
				else if (ifDigit(c.getResult())) {	/* debug1: result可能是常数 */
					// 这个问题通过 simplify_br() 函数解决
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
		bool change = true;
		while (change) {
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
	int size1 = symtotal.size();
	// 申请空间
	for (i = 0; i < size1; i++) {
		vector<varStruct> v;
		varChain.push_back(v);
	}
	// 添加变量
	for (i = 1; i < size1; i++) {
		int size2 = symtotal[i].size();
		for (j = 0; j < size2; j++) {
			symbolTable st = symtotal[i][j];
			if (st.getForm() == VARIABLE) {
				set<int> s;
				varChain[i].push_back(varStruct(st.getName(), s));
			}
		}
	}
	// 确定迭代必经边界
	// size1 = blockCore[i].size();		
	// 实际上，codetotal、symtotal、blockCore等的一维大小应该都相同
	for (i = 1; i < size1; i++) {
		int size2 = varChain[i].size();
		for (j = 0; j < size2; j++) {
			int size3 = blockCore[i].size();
			// 初始化寻找
			for (k = 1; k < size3; k++)
				if (blockCore[i][k].def.find(varChain[i][j].name) != blockCore[i][k].def.end())
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
						}
				}
			}
			varChain[i][j].blockNums = DFP;
		}
	}
}

void SSA::renameVar() {

}

// 简化条件判断为常值的跳转指令
void SSA::simplify_br() {
	int i, j, k;
	int size1 = codetotal.size();
	for (i = 1; i < size1; i++) {
		int size2 = codetotal[i].size();
		for (j = 0; j < size2; j++) {
			CodeItem ci = codetotal[i][j];
			if (ci.getCodetype() == BR && ifDigit(ci.getResult())) {
				// CodeItem nci(BR, "", "", "");
				if (ci.getResult().compare("0") == 0) { 
					CodeItem nci(BR, ci.getOperand1(), "", "");
					codetotal[i].erase(codetotal[i].begin() + j);
					codetotal[i].insert(codetotal[i].begin() + j, nci);
					// nci.setResult(ci.getOperand1());
				}
				else { 
					CodeItem nci(BR, ci.getOperand2(), "", "");
					codetotal[i].erase(codetotal[i].begin() + j);
					codetotal[i].insert(codetotal[i].begin() + j, nci);
					// nci.setResult(ci.getOperand2());
				}
			}
		}
	}
}

// 入口函数
void SSA::generate() {

	// 简化条件判断为常值的跳转指令
	simplify_br();

	// 为每个函数划分基本块
	divide_basic_block();							
	
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

	// 测试输出上面各个函数
	Test_SSA();
}