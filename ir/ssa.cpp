﻿#include "ssa.h"
#include <regex>
#include <algorithm>
#include <set>

using namespace std;

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

// 
int locBlockForLabel(vector<int> v, int key) {
	int size = v.size();
	for (int i = 0; i < size; i++) if (v[i] == key) return i;
	return -1;
}

// 测试函数：输出所有的基本块信息
void Test_Divide_Basic_Block(vector<vector<basicBlock>> v) {
	cout << "---------------- basic block -----------------" << endl;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		cout << i << endl;
		int size2 = v[i].size();
		// 依次输出该函数下所有的基本块信息
		for (int j = 0; j < size2; j++) { 
			cout << "基本块编号: " << v[i][j].number << endl;
			cout << "该基本块的第一条中间代码" << v[i][j].start << "\t" << endl;
			cout << "该基本块的最后一条中间代码" << v[i][j].end << "\t" << endl;
			set<int>::iterator iter;
			cout << "该基本块的前序节点，即可以跳转到该基本块的基本块序号: { ";
			for (iter = v[i][j].pred.begin(); iter != v[i][j].pred.end(); iter++) cout << *iter << " ";
			cout << "}" << endl;
			cout << "该基本块的后序节点，即通过该基本块可以跳转到的基本块序号: { ";
			for (iter = v[i][j].succeeds.begin(); iter != v[i][j].succeeds.end(); iter++) cout << *iter << " ";
			cout << "}" << endl;
		}
	}
}

void Test_Build_Dom_Tree(vector<vector<basicBlock>> v) {
	cout << "---------------- dom tree -----------------" << endl;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		cout << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			cout << v[i][j].number;
			cout << "{  ";
			for (set<int>::iterator iter = v[i][j].domin.begin(); iter != v[i][j].domin.end(); iter++) cout << *iter << " ";
			cout << "}" << endl;
		}
	}
}

void Test_Build_Idom_Tree(vector<vector<basicBlock>> v) {
	cout << "---------------- idom tree -----------------" << endl;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		cout << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			cout << v[i][j].number;
			cout << "{  ";
			for (set<int>::iterator iter = v[i][j].idom.begin(); iter != v[i][j].idom.end(); iter++) cout << *iter << " ";
			cout << "}" << endl;
		}
	}
}

void Test_Build_Reverse_Idom_Tree(vector<vector<basicBlock>> v) {
	cout << "---------------- reverse idom tree -----------------" << endl;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		cout << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			cout << v[i][j].number;
			cout << "{  ";
			for (set<int>::iterator iter = v[i][j].reverse_idom.begin(); iter != v[i][j].reverse_idom.end(); iter++) cout << *iter << " ";
			cout << "}" << endl;
		}
	}
}

void Test_Build_Post_Order(vector<vector<int>> v) {
	cout << "---------------- post order -----------------" << endl;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		cout << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) cout << v[i][j] << " ";
		cout << endl;
	}
}

void Test_Build_Pre_Order(vector<vector<int>> v) {
	cout << "---------------- pre order -----------------" << endl;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		cout << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) cout << v[i][j] << " ";
		cout << endl;
	}
}

void Test_Build_Dom_Frontier(vector<vector<basicBlock>> v) {
	cout << "---------------- df tree -----------------" << endl;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		cout << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			cout << v[i][j].number;
			cout << "{  ";
			for (set<int>::iterator iter = v[i][j].df.begin(); iter != v[i][j].df.end(); iter++) cout << *iter << " ";
			cout << "}" << endl;
		}
	}
}

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
		vector<int> tmp = { 0, 1 };		// entry and 第一条代码
		for (j = 0; j < size2; j++) {	// 第几条中间代码(1, 2, 3...)
			CodeItem c = v[j];
			if (c.getCodetype() == BR) {	// 跳转指令
				if (ifTempVariable(c.getResult())) {
					k = lookForLabel(v, c.getOperand1());
					if (find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
					k = lookForLabel(v, c.getOperand2());
					if (find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
				}
				else {
					k = lookForLabel(v, c.getResult());
					if (find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
				}
				k = j + 1;
				if (find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
			}
			else if (c.getCodetype() == RET) { // 返回指令
				k = j + 1;
				if (k + 1 <= size2 && find(tmp.begin(), tmp.end(), k + 1) == tmp.end()) tmp.push_back(k + 1);
			}
		}
		tmp.push_back(size2 + 1);	// exit块
		sort(tmp.begin(), tmp.end());
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
		basicBlock bb1(0, 0, 0, {},  { 1 });		// entry块
		tmp.push_back(bb1);
		int size2 = v.size();
		for (j = 1; j < size2 - 1; j++) {
			basicBlock bb(j, v[j], v[j + 1] - 1, {}, {});
			tmp.push_back(bb);
		}
		tmp[1].pred.insert(0);
		basicBlock bb2(j, v[j], v[j], {}, {});	// exit块
		tmp.push_back(bb2);
		// 建立关系
		for (j = 1; j < size2 - 1; j++) {
			if (codetotal[i][tmp[j].end - 1].getCodetype() == BR) {	// 跳转指令
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
				k = size2 - 1;	// exit块 
				tmp[j].succeeds.insert(k);
				tmp[k].pred.insert(j);
			}
			else {
				k = j + 1;
				tmp[j].succeeds.insert(k);
				tmp[k].pred.insert(j);
			}
		}
		/*
		// 删除不可达基础块(这一部分其实可以放到前端来做)
		for (j = 1; j < tmp.size(); j++) if (tmp[j].pred.empty()) { tmp.erase(tmp.begin() + j); j--; }*/
		blockCore.push_back(tmp); 
	}
}

void SSA::build_dom_tree() {
	int size1 = blockCore.size();
	int i, j, k;
	for (i = 1; i < size1; i++) 
	{
		int size2 = blockCore[i].size();
		vector<int> N;
		for (j = 0; j < size2; j++) N.push_back(j);	// 该函数下基本块的数量
		blockCore[i][0].domin.insert(0);
		for (j = 1; j < size2; j++) blockCore[i][j].domin.insert(N.begin(), N.end());
		bool change;
		do {
			change = false;
			for (j = 1; j < size2; j++) {
				set<int> T(N.begin(), N.end());
				for (set<int>::iterator iter = blockCore[i][j].pred.begin(); iter != blockCore[i][j].pred.end(); iter++) {
					set<int> tmp;
					set_intersection(T.begin(), T.end(), blockCore[i][*iter].domin.begin(), blockCore[i][*iter].domin.end(), inserter(tmp, tmp.begin()));
					T = tmp;
				}
				set<int> D = T;
				D.insert(j);
				if (D.size() != blockCore[i][j].domin.size()) {
					change = true;
					blockCore[i][j].domin = D;
				}
				else {
					for (set<int>::iterator iter1 = D.begin(), iter2 = blockCore[i][j].domin.begin(); iter1 != D.end(); iter1++, iter2++) {
						if (*iter1 != *iter2) {
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

void SSA::build_reverse_idom_tree() {
	int size1 = blockCore.size();
	int i, j, k;
	for (i = 1; i < size1; i++)
	{
		int size2 = blockCore[i].size();
		for (j = 0; j < size2; j++) {
			set<int> s;
			for (k = 0; k < size2; k++) if (blockCore[i][k].idom.find(j) != blockCore[i][k].idom.end()) s.insert(k);
			blockCore[i][j].reverse_idom = s;
		}
	}
}

void SSA::post_order(int funNum, int node) {
	basicBlock bb = blockCore[funNum][node];
	for (set<int>::iterator iter = bb.reverse_idom.begin(); iter != bb.reverse_idom.end(); iter++) {
		post_order(funNum, *iter);
	}
	postOrder[funNum].push_back(node);
}

void SSA::build_post_order() {
	int size1 = blockCore.size();
	int i;
	for (i = 0; i < size1; i++) { vector<int> v; postOrder.push_back(v); }
	for (i = 1; i < size1; i++) post_order(i, 0);
}

void SSA::pre_order(int funNum, int node) {
	preOrder[funNum].push_back(node);
	basicBlock bb = blockCore[funNum][node];
	for (set<int>::iterator iter = bb.reverse_idom.begin(); iter != bb.reverse_idom.end(); iter++) {
		pre_order(funNum, *iter);
	}
}

void SSA::build_pre_order() {
	int size1 = blockCore.size();
	int i;
	for (i = 0; i < size1; i++) { vector<int> v; preOrder.push_back(v); }
	for (i = 1; i < size1; i++) pre_order(i, 0);
}

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

void SSA::generate() {
	divide_basic_block();
	Test_Divide_Basic_Block(blockCore);
	build_dom_tree();
	Test_Build_Dom_Tree(blockCore);
	build_idom_tree();
	Test_Build_Idom_Tree(blockCore);
	build_reverse_idom_tree();
	Test_Build_Reverse_Idom_Tree(blockCore);
	build_post_order();
	Test_Build_Post_Order(postOrder);
	build_pre_order();
	Test_Build_Pre_Order(preOrder);
	build_dom_frontier();
	Test_Build_Dom_Frontier(blockCore);
}