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

//��¼�������ָ��λ��
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
// ʹ��-������ use-define chain
//===============================

class UDchain {
	vector<basicBlock> CFG;
	vector<set<Node>> in, out, gen, kill;					//��¼ÿ��������ļ���
	map<string, set<Node>> var2defs;						//��¼ÿ�������Ķ����
	map<Node, string> def2var;								//ÿ�������ֻ�ᶨ��һ������

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
	void count_gen();						//����CFG����ÿ���������gen���ϡ�kill����
	void iterate_in_out();					//��������һ��in��out����
	void count_in_and_out();				//����gen��kill����in��out
	void count_UDchain();					//������������ͼ��ʹ��-������
	void printUDchain(ofstream& ud);
	Node getDef(Node use, string var) {
		return chains[pair<string, Node>(var, use)];
	}
private:
	bool find_var_def(set<Node> container, string var);			//Ѱ�ұ����Ķ���
	void erase_defs(set<Node> container, string var);			//ɾ�������е�var�Ķ���
	void add_A_UDChain(string var, const Node& use);			//���һ��ʹ��-������
	bool checkPhi(string var, int blkNum);											//�����ж�������ʹ��use�ǲ���phi����ı���
};

#endif //_DATA_ANALYSE_H_