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
	Node() : var(""), bIdx(-1), lIdx(-1) {}
	Node(int blk_idx, int line_idx, string var) : var(var), bIdx(blk_idx), lIdx(line_idx) {}
	string display() const {
		return FORMAT("({}, <{}, {}>)", var, bIdx, lIdx);
	}

	friend bool operator==(const Node& lhs, const Node& rhs) {
		return tie(lhs.bIdx, lhs.lIdx, lhs.var) == tie(rhs.bIdx, rhs.lIdx, rhs.var);
	}

	friend bool operator!=(const Node& lhs, const Node& rhs) {
		return !(lhs == rhs);
	}

	friend bool operator<(const Node& lhs, const Node& rhs) {
		const int var1 = A2I(lhs.var.substr(1));
		const int var2 = A2I(rhs.var.substr(1));
		return tie(var1, lhs.bIdx, lhs.lIdx) < tie(var2, rhs.bIdx, rhs.lIdx);
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
	
	map<Node, string> def2var;								//ÿ�������ֻ�ᶨ��һ������
	map<string, set<Node>> var2defs;						//��¼ÿ�������Ķ��壨������phi�Ķ��壩

	map<pair<string, Node>, Node> chains;				//udchains
	//��Ϊ��ʱ����ֻ��ʹ��һ��
	map<pair<string, Node>, Node> duchains;		//duchains	��Ҫ����ʱ�����ã������Ƿ���Ҫ�����ѭ������ʽ����
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
		pair<string, Node> tmp(var, use);
		if (chains.find(tmp) != chains.end()) {
			return chains[tmp];
		}else {
			return Node(-1, -1, "");
		}
	}
	set<Node> get_Defs_of_var(string var) {
		if (var2defs.find(var) != var2defs.end()) {	//��
			return var2defs[var];
		}
		else {
			return set<Node>();
		}
	}
private:
	bool find_var_def(set<Node> container, string var);			//Ѱ�ұ����Ķ���
	void erase_defs(set<Node>& container, string var);			//ɾ�������е�var�Ķ���
	void add_A_UDChain(string var, const Node& use);			//���һ��ʹ��-������
	bool checkPhi(string var, int blkNum);											//�����ж�������ʹ��use�ǲ���phi����ı���
	void add_A_DUChain(string var, const Node& use);
};

#endif //_DATA_ANALYSE_H_