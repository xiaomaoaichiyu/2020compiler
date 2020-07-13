#include "ssa.h"
#include "../front/syntax.h"
#include <regex>
#include <algorithm>
#include <set>
#include <fstream>

using namespace std;
ofstream debug_ssa;

void SSA::Test_SSA() {
	// 打开文件，将测试输出信息输出到该文件
	debug_ssa.open("debug_ssa.txt");
	/* Test_* 函数用于对每个函数进行测试，输出相关信息 */
	Test_Divide_Basic_Block();
	Test_Build_Dom_Tree();
	Test_Build_Idom_Tree();
	Test_Build_Reverse_Idom_Tree();
	// Test_Build_Post_Order();
	// Test_Build_Pre_Order();
	// Test_Build_Def_Use_Chain();
	// Test_Active_Var_Analyse();
	Test_Build_Dom_Frontier();
	Test_Build_Var_Chain();
	Test_Add_Phi_Fun();
	// 关闭文件
	debug_ssa.close();
}

// 测试函数，输出添加\phi函数的信息
void SSA::Test_Add_Phi_Fun() {
	debug_ssa << "---------------- phi function -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "基本块编号" << "\t\t" << v[i][j].number << endl;
			for (vector<phiFun>::iterator iter = v[i][j].phi.begin(); iter != v[i][j].phi.end(); iter++) {
				debug_ssa << "\\phi" << "\t\t";
				debug_ssa << (*iter).name << "\t\t";
				debug_ssa << "{" << "  ";
				for (set<int>::iterator iter1 = (*iter).blockNums.begin(); iter1 != (*iter).blockNums.end(); iter1++)
					debug_ssa << *iter1 << "  ";
				debug_ssa << "}\t\t";
				debug_ssa << "{" << "  ";
				for (set<string>::iterator iter2 = (*iter).subIndexs.begin(); iter2 != (*iter).subIndexs.end(); iter2++)
					debug_ssa << *iter2 << "  ";
				debug_ssa << "}" << endl;
			}
		}
	}
}

// 输出基本块中的中间代码
void Output_IR(vector<CodeItem> v) {
	for (int i = 0; i < v.size(); i++) {
		CodeItem item = v[i];
		debug_ssa << i + 1 << "    " << item.getContent() << endl;
	}
}

// 测试函数：输出所有的基本块信息
void SSA::Test_Divide_Basic_Block() {
	debug_ssa << "---------------- basic block -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << "---------------- function " << i << " ----------------" << endl;
		int size2 = v[i].size();
		// 依次输出该函数下所有的基本块信息
		for (int j = 0; j < size2; j++) {
			debug_ssa << "该基本块的编号:  " << v[i][j].number << "\t\t";	// 基本块编号
			set<int>::iterator iter;
			// 即可以跳转到该基本块的基本块序号
			debug_ssa << "前序节点和后序节点: {  ";
			for (iter = v[i][j].pred.begin(); iter != v[i][j].pred.end(); iter++) debug_ssa << *iter << "  ";
			debug_ssa << "}" << "\t\t";
			// 即通过该基本块可以跳转到的基本块序号
			debug_ssa << "{  ";
			for (iter = v[i][j].succeeds.begin(); iter != v[i][j].succeeds.end(); iter++) debug_ssa << *iter << "  ";
			debug_ssa << "}" << endl;
			// 输出该基本块中的中间代码
			Output_IR(v[i][j].Ir);
		}
	}
}

// 测试函数：输出构建的必经节点
void SSA::Test_Build_Dom_Tree() {
	debug_ssa << "---------------- dom tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "该基本块编号: " << v[i][j].number << "\t\t";
			debug_ssa << "该基本块的必经节点: {  ";
			for (set<int>::iterator iter = v[i][j].domin.begin(); iter != v[i][j].domin.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

// 测试函数：输出直接必经节点
void SSA::Test_Build_Idom_Tree() {
	debug_ssa << "---------------- idom tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "该基本块的编号: " << v[i][j].number << "\t\t";
			debug_ssa << "该基本块的直接必经节点: {  ";
			for (set<int>::iterator iter = v[i][j].idom.begin(); iter != v[i][j].idom.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

// 测试函数：输出反向必经节点
void SSA::Test_Build_Reverse_Idom_Tree() {
	debug_ssa << "---------------- reverse idom tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "该基本块的编号: " << v[i][j].number << "\t\t";
			debug_ssa << "该基本块反向直接必经节点: {  ";
			for (set<int>::iterator iter = v[i][j].reverse_idom.begin(); iter != v[i][j].reverse_idom.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}

// 测试函数：输出后序遍历序列
void SSA::Test_Build_Post_Order() {
	debug_ssa << "---------------- post order -----------------" << endl;
	vector<vector<int>> v = postOrder;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		debug_ssa << "后序遍历序列" << "\t\t";
		for (int j = 0; j < size2; j++) debug_ssa << v[i][j] << "\t\t";
		debug_ssa << endl;
	}
}

// 测试函数：输出前序遍历序列
void SSA::Test_Build_Pre_Order() {
	debug_ssa << "---------------- pre order -----------------" << endl;
	vector<vector<int>> v = preOrder;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		debug_ssa << "前序遍历序列" << "\t\t";
		for (int j = 0; j < size2; j++) debug_ssa << v[i][j] << "\t\t";
		debug_ssa << endl;
	}
}

// 测试函数：输出必经边界
void SSA::Test_Build_Dom_Frontier() {
	debug_ssa << "---------------- df tree -----------------" << endl;
	vector<vector<basicBlock>> v = blockCore;
	int size1 = v.size();
	for (int i = 1; i < size1; i++) {
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "该基本块的编号: " << v[i][j].number << "\t\t";
			debug_ssa << "该基本块的必经边界: {  ";
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
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "该基本块的编号: " << v[i][j].number << endl;
			debug_ssa << "该基本块的def变量: {  ";
			for (set<string>::iterator iter = v[i][j].def.begin(); iter != v[i][j].def.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
			debug_ssa << "该基本块的use变量: {  ";
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
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "该基本块的编号: " << v[i][j].number << endl;
			debug_ssa << "该基本块的in集合: {  ";
			for (set<string>::iterator iter = v[i][j].in.begin(); iter != v[i][j].in.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
			debug_ssa << "该基本块的out集合: {  ";
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
		// 首先输出这是第几个函数
		debug_ssa << i << endl;
		int size2 = v[i].size();
		for (int j = 0; j < size2; j++) {
			debug_ssa << "变量名: " << v[i][j].name << endl;
			debug_ssa << "该变量的迭代必经边界: {  ";
			for (set<int>::iterator iter = v[i][j].blockNums.begin(); iter != v[i][j].blockNums.end(); iter++) debug_ssa << *iter << "\t\t";
			debug_ssa << "}" << endl;
		}
	}
}
