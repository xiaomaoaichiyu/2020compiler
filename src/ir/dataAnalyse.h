#ifndef _DATA_ANALYSE_H_
#define _DATA_ANALYSE_H_

#include <fstream>
#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <string>
#include <map>
#include "../ir/ssa.h"
#include "../util/meow.h"

using namespace std;

//记录基本块和指令位置
class Node {
public:
	string var;
	int bIdx, lIdx;
	Node() {}
	Node(int blk_idx, int line_idx, string var) : var(var), bIdx(blk_idx), lIdx(line_idx) {}
	string display() const {
		return FORMAT("({}, <{}, {}>)", var, bIdx, lIdx);
	}

	friend bool operator==(const Node& lhs, const Node& rhs) {
		return tie(lhs.bIdx, lhs.lIdx) == tie(rhs.bIdx, rhs.lIdx);
	}

	friend bool operator!=(const Node& lhs, const Node& rhs) {
		return !(lhs == rhs);
	}

	friend bool operator<(const Node& lhs, const Node& rhs) {
		return tie(lhs.bIdx, lhs.lIdx) < tie(rhs.bIdx, rhs.lIdx);
	}

	friend bool operator<=(const Node& lhs, const Node& rhs) {
		return !(rhs < lhs);
	}

	friend bool operator>(const Node& lhs, const Node& rhs) {
		return rhs < lhs;
	}

	friend bool operator>=(const Node& lhs, const Node& rhs) {
		return !(lhs < rhs);
	}
};

//===============================
// 使用-定义链 use-define chain
//===============================

class UDchain {
	vector<basicBlock> CFG;
	vector<set<Node>> in, out, gen, kill;					//记录每个基本块的集合
	map<string, set<Node>> var2defs;						//记录每个变量的定义点
	map<Node, string> def2var;								//每个定义点只会定义一个变量

	map<pair<string, Node>, Node> chains;
public:
	UDchain() {}
	UDchain(vector<basicBlock>& cfg) : CFG(cfg) {
		chains.clear();
		init();
		count_gen();
		count_in_and_out();
		count_UDchain();
	}
	void init();
	void count_gen();						//根据CFG计算每个基本块的gen集合、kill集合
	void iterate_in_out();					//迭代计算一次in、out集合
	void count_in_and_out();				//根据gen、kill计算in，out
	void count_UDchain();					//计算整个流程图的使用-定义链
	void printUDchain(ofstream& ud);
	Node getDef(Node use, string var) {
		return chains[pair<string, Node>(var, use)];
	}
private:
	bool find_var_def(set<Node> container, string var);			//寻找变量的定义
	void erase_defs(set<Node> container, string var);			//删除集合中的var的定义
	void add_A_UDChain(string var, const Node& use);			//添加一条使用-定义链
	bool checkPhi(string var, int blkNum);											//检查具有多个定义的使用use是不是phi定义的变量
};

#endif //_DATA_ANALYSE_H_