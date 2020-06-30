#pragma once
#include <iostream>
#include <vector>
#include <set>
#include "intermediatecode.h"

class basicBlock {
public:
	int number;							// ������ı��
	int start;								// ���������ʼ�м����
	int end;								// ������Ľ����м����
	std::set<int> pred;				// ǰ��
	std::set<int> succeeds;			// ��̽ڵ�
	std::set<int> domin;				// �ؾ��ڵ�
	std::set<int> idom;				// ֱ�ӱؾ��ڵ�
	std::set<int> tmpIdom;			// ����ֱ�ӱؾ��ڵ��㷨����Ҫ�����ݽṹ
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