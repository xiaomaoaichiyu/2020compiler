#include "ssa.h"
#include <regex>
#include <algorithm>
#include <set>

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
			set<int> tmp1, tmp2;
			tmp1.insert(j);
			set_difference(blockCore[i][j].domin.begin(), blockCore[i][j].domin.end(), tmp1.begin(), tmp1.end(), inserter(tmp2, tmp2.begin()));;
			blockCore[i][j].tmpIdom = tmp2;
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
		for (j = 1; j < size2; j++) blockCore[i][j].idom.insert(*(blockCore[i][j].tmpIdom.begin()));
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

// 入口函数
void SSA::generate() {
	divide_basic_block();
	Test_Divide_Basic_Block();
	build_dom_tree();
	Test_Build_Dom_Tree();
	build_idom_tree();
	Test_Build_Idom_Tree();
	build_reverse_idom_tree();
	Test_Build_Reverse_Idom_Tree();
	build_post_order();
	Test_Build_Post_Order();
	build_pre_order();
	Test_Build_Pre_Order();
	build_dom_frontier();
	Test_Build_Dom_Frontier();
}

// 测试函数：输出所有的基本块信息
void SSA::Test_Divide_Basic_Block() {
	cout << "---------------- basic block -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		cout << i << endl;
		int size2 = v[i].size();
		// 依次输出该函数下所有的基本块信息
		cout << "基本块编号" << "\t" << "入口中间代码" << "\t" << "结束中间代码" << "\t" << "前序节点" << "\t" << "后序节点" << "\t" << endl;
		for (int j = 0; j < size2; j++) {
			cout << v[i][j].number << "\t";	// 基本块编号
			cout << v[i][j].start << "\t";		// 入口中间代码
			cout << v[i][j].end << "\t";			// 结束中间代码
			set<int>::iterator iter;
			 // 即可以跳转到该基本块的基本块序号
			cout << "{ ";
			for (iter = v[i][j].pred.begin(); iter != v[i][j].pred.end(); iter++) cout << *iter << " ";
			cout << "}" << "\t";
			 // 即通过该基本块可以跳转到的基本块序号
			cout << "{ ";
			for (iter = v[i][j].succeeds.begin(); iter != v[i][j].succeeds.end(); iter++) cout << *iter << " ";
			cout << "}" << endl;
		}
	}
}

// 测试函数：输出构建的必经节点
void SSA::Test_Build_Dom_Tree() {
	cout << "---------------- dom tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		cout << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			cout << "该基本块编号: " << v[i][j].number << "\t";
			cout << "该基本块的必经节点: {  ";
			for (set<int>::iterator iter = v[i][j].domin.begin(); iter != v[i][j].domin.end(); iter++) cout << *iter << " ";
			cout << "}" << endl;
		}
	}
}

// 测试函数：输出直接必经节点
void SSA::Test_Build_Idom_Tree() {
	cout << "---------------- idom tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		cout << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			cout << "该基本块的编号: " << v[i][j].number << "\t";
			cout << "该基本块的直接必经节点: {  ";
			for (set<int>::iterator iter = v[i][j].idom.begin(); iter != v[i][j].idom.end(); iter++) cout << *iter << " ";
			cout << "}" << endl;
		}
	}
}

// 测试函数：输出反向必经节点
void SSA::Test_Build_Reverse_Idom_Tree() {
	cout << "---------------- reverse idom tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		cout << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			cout << "该基本块的编号: " << v[i][j].number << "\t";
			cout << "该基本块反向直接必经节点: {  ";
			for (set<int>::iterator iter = v[i][j].reverse_idom.begin(); iter != v[i][j].reverse_idom.end(); iter++) cout << *iter << " ";
			cout << "}" << endl;
		}
	}
}

// 测试函数：输出后序遍历序列
void SSA::Test_Build_Post_Order() {
	cout << "---------------- post order -----------------" << endl;
	vector<vector<int>> v = postOrder;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		cout << i << endl;
		int size2 = v[i].size();
		cout << "后序遍历序列" << "\t";
		for (int j = 0; j < size2; j++) cout << v[i][j] << " ";
		cout << endl;
	}
}

// 测试函数：输出前序遍历序列
void SSA::Test_Build_Pre_Order() {
	cout << "---------------- pre order -----------------" << endl;
	vector<vector<int>> v = preOrder;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		cout << i << endl;
		int size2 = v[i].size();
		cout << "前序遍历序列" << "\t";
		for (int j = 0; j < size2; j++) cout << v[i][j] << " ";
		cout << endl;
	}
}

// 测试函数：输出必经边界
void SSA::Test_Build_Dom_Frontier() {
	cout << "---------------- df tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		cout << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			cout << "该基本块的编号: " << v[i][j].number << "\t";
			cout << "该基本块的必经边界: {  ";
			for (set<int>::iterator iter = v[i][j].df.begin(); iter != v[i][j].df.end(); iter++) cout << *iter << " ";
			cout << "}" << endl;
		}
	}
}