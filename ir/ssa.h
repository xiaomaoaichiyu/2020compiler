#pragma once
#include <iostream>
#include <vector>
#include <set>
#include "intermediatecode.h"

class basicBlock {
public:
	int number;							// 基本块的编号
	int start;								// 基本块的起始中间代码
	int end;								// 基本块的结束中间代码
	std::set<int> pred;				// 前驱
	std::set<int> succeeds;			// 后继节点
	std::set<int> domin;				// 必经节点
	std::set<int> idom;				// 直接必经节点
	std::set<int> reverse_idom;	// 直接必经节点反
	std::set<int> tmpIdom;			// 计算直接必经节点算法中需要的数据结构
	std::set<int> df;					// 必经边界
	basicBlock(int number, int start, int end, std::set<int> pred, std::set<int> succeeds) {
		this->number = number;
		this->start = start;
		this->end = end;
		this->pred = pred;
		this->succeeds = succeeds;
	}
};

class SSA {
private:
	std::vector<std::vector<CodeItem>> codetotal;	// 睿轩前端传过来的中间代码
	std::vector<std::vector<int>> blockOrigin;			// 每个基本块的第一条中间代码
	std::vector<std::vector<basicBlock>> blockCore;	// 每个基本块结构
	std::vector<std::vector<int>> postOrder;				// 必经节点数的后序遍历序列
	std::vector<std::vector<int>> preOrder;				// 必经节点数的前序遍历序列
	void divide_basic_block();									// 划分基本块
	void build_dom_tree();										// 建立必经节点关系
	void build_idom_tree();										// 建立直接必经关系
	void build_reverse_idom_tree();							// 直接必经节点的反关系
	void post_order(int funNum, int node);
	void build_post_order();										// 后序遍历必经节点树
	void pre_order(int funNum, int node);
	void build_pre_order();										// 前序遍历必经节点树
	void build_dom_frontier();									// 计算流图必经边界
	void build_def_use_chain();								// 计算ud链
	// 测试专用函数
	void Test_Divide_Basic_Block();	
	void Test_Build_Dom_Tree();
	void Test_Build_Idom_Tree();
	void Test_Build_Reverse_Idom_Tree();
	void Test_Build_Post_Order();
	void Test_Build_Pre_Order();
	void Test_Build_Dom_Frontier();
public:
	SSA(std::vector<std::vector<CodeItem>> codetotal) {
		this->codetotal = codetotal;
	}
	void generate();
};