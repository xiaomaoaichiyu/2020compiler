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
	std::set<int> tmpIdom;			// 计算直接必经节点算法中需要的数据结构
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
	std::vector<std::vector<CodeItem>> codetotal;
	std::vector<std::vector<int>> blockOrigin;
	std::vector<std::vector<basicBlock>> blockCore;
	void divide_basic_block();
	void build_dom_tree();
	void build_idom_tree();
public:
	SSA(std::vector<std::vector<CodeItem>> codetotal) {
		this->codetotal = codetotal;
	}
	void generate();
};