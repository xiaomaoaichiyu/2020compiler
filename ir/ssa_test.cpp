#include "ssa.h"
#include <regex>
#include <algorithm>
#include <set>
#include <fstream>

using namespace std;
ofstream debug_ssa;

void SSA::Test_SSA() {
	// ���ļ��������������Ϣ��������ļ�
	debug_ssa.open("debug_ssa.txt");
	/* Test_* �������ڶ�ÿ���������в��ԣ���������Ϣ */
	Test_Divide_Basic_Block();
	Test_Build_Dom_Tree();
	Test_Build_Idom_Tree();
	Test_Build_Reverse_Idom_Tree();
	Test_Build_Post_Order();
	Test_Build_Pre_Order();
	Test_Build_Dom_Frontier();
	Test_Build_Def_Use_Chain();
	Test_Active_Var_Analyse();
	Test_Build_Var_Chain();
	// �ر��ļ�
	debug_ssa.close();
}

// ���Ժ�����������еĻ�������Ϣ
void SSA::Test_Divide_Basic_Block() {
	debug_ssa << "---------------- basic block -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// ����������ǵڼ�������
		debug_ssa << i << endl;
		int size2 = v[i].size();
		// ��������ú��������еĻ�������Ϣ
		debug_ssa << "��������" << "\t\t" << "����м����" << "\t\t" << "�����м����" << "\t\t" << "ǰ��ڵ�" << "\t\t" << "����ڵ�" << "\t\t" << endl;
		for (int j = 0; j < size2; j++) {
			debug_ssa << v[i][j].number << "\t\t";	// ��������
			debug_ssa << v[i][j].start << "\t\t";		// ����м����
			debug_ssa << v[i][j].end << "\t\t";		// �����м����
			set<int>::iterator iter;
			// ��������ת���û�����Ļ��������
			debug_ssa << "{ ";
			for (iter = v[i][j].pred.begin(); iter != v[i][j].pred.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << "\t\t";
			// ��ͨ���û����������ת���Ļ��������
			debug_ssa << "{ ";
			for (iter = v[i][j].succeeds.begin(); iter != v[i][j].succeeds.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

// ���Ժ�������������ıؾ��ڵ�
void SSA::Test_Build_Dom_Tree() {
	debug_ssa << "---------------- dom tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// ����������ǵڼ�������
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "�û�������: " << v[i][j].number << "\t\t";
			debug_ssa << "�û�����ıؾ��ڵ�: {  ";
			for (set<int>::iterator iter = v[i][j].domin.begin(); iter != v[i][j].domin.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

// ���Ժ��������ֱ�ӱؾ��ڵ�
void SSA::Test_Build_Idom_Tree() {
	debug_ssa << "---------------- idom tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// ����������ǵڼ�������
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "�û�����ı��: " << v[i][j].number << "\t\t";
			debug_ssa << "�û������ֱ�ӱؾ��ڵ�: {  ";
			for (set<int>::iterator iter = v[i][j].idom.begin(); iter != v[i][j].idom.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

// ���Ժ������������ؾ��ڵ�
void SSA::Test_Build_Reverse_Idom_Tree() {
	debug_ssa << "---------------- reverse idom tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// ����������ǵڼ�������
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "�û�����ı��: " << v[i][j].number << "\t\t";
			debug_ssa << "�û����鷴��ֱ�ӱؾ��ڵ�: {  ";
			for (set<int>::iterator iter = v[i][j].reverse_idom.begin(); iter != v[i][j].reverse_idom.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

// ���Ժ�������������������
void SSA::Test_Build_Post_Order() {
	debug_ssa << "---------------- post order -----------------" << endl;
	vector<vector<int>> v = postOrder;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// ����������ǵڼ�������
		debug_ssa << i << endl;
		int size2 = v[i].size();
		debug_ssa << "�����������" << "\t\t";
		for (int j = 0; j < size2; j++) debug_ssa << v[i][j] << "\t\t";
		debug_ssa << endl;
	}
}

// ���Ժ��������ǰ���������
void SSA::Test_Build_Pre_Order() {
	debug_ssa << "---------------- pre order -----------------" << endl;
	vector<vector<int>> v = preOrder;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// ����������ǵڼ�������
		debug_ssa << i << endl;
		int size2 = v[i].size();
		debug_ssa << "ǰ���������" << "\t\t";
		for (int j = 0; j < size2; j++) debug_ssa << v[i][j] << "\t\t";
		debug_ssa << endl;
	}
}

// ���Ժ���������ؾ��߽�
void SSA::Test_Build_Dom_Frontier() {
	debug_ssa << "---------------- df tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// ����������ǵڼ�������
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "�û�����ı��: " << v[i][j].number << "\t\t";
			debug_ssa << "�û�����ıؾ��߽�: {  ";
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
		// ����������ǵڼ�������
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "�û�����ı��: " << v[i][j].number << endl;
			debug_ssa << "�û������def����: {  ";
			for (set<string>::iterator iter = v[i][j].def.begin(); iter != v[i][j].def.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
			debug_ssa << "�û������use����: {  ";
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
		// ����������ǵڼ�������
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "�û�����ı��: " << v[i][j].number << endl;
			debug_ssa << "�û������in����: {  ";
			for (set<string>::iterator iter = v[i][j].in.begin(); iter != v[i][j].in.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
			debug_ssa << "�û������out����: {  ";
			for (set<string>::iterator iter = v[i][j].out.begin(); iter != v[i][j].out.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

void SSA::Test_Build_Var_Chain() {
	debug_ssa << "---------------- var chain -----------------" << endl;
	vector<vector<varStruct>> v = varChain;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// ����������ǵڼ�������
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "������: " << v[i][j].name << endl;
			debug_ssa << "�ñ����ĵ����ؾ��߽�: {  ";
			for (set<int>::iterator iter = v[i][j].blockNums.begin(); iter != v[i][j].blockNums.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}