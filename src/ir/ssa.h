﻿#ifndef _SSA_H_
#define _SSA_H_

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include "intermediatecode.h"
#include "../front/syntax.h"

class phiFun {
	// phi函数的例子：y^3 = \phi(y^1, y^2)
public:
	std::string primaryName;				// 初始命名为y，之后不做修改
	std::string name;							// 初始命名为y，后面更改为y^3
	std::vector<int> blockNums;				// y^1, y^2所在的基本块序号
	std::vector<std::string> subIndexs;	// y^1, y^2
	phiFun(std::string primaryName, std::string name, std::vector<int> blockNums, std::vector<std::string> subIndexs) {
		this->primaryName = primaryName;
		this->name = name;
		this->blockNums = blockNums;
		this->subIndexs = subIndexs;
	}
};

class basicBlock {
public:
	int number;							// 基本块的编号
	// int start;								// 基本块的起始中间代码（删掉）
	// int end;								// 基本块的结束中间代码（删掉）
	std::vector<CodeItem> Ir;	// 该基本块对应的中间代码
	std::set<int> pred;				// 前驱
	std::set<int> succeeds;			// 后继节点
	std::set<int> domin;				// 必经节点
	std::set<int> idom;				// 直接必经节点
	std::set<int> reverse_idom;	// 直接必经节点反
	std::set<int> tmpIdom;			// 计算直接必经节点算法中需要的数据结构
	std::set<int> df;					// 必经边界
	std::set<std::string> use;		// 该基本块中的use变量
	std::set<std::string> def;		// 该基本块中的def变量
	std::set<std::string> in;		// 该基本块中的in集合
	std::set<std::string> out;		// 该基本块中的out集合
	std::vector<phiFun> phi;		// 基本块初始需要添加的\phi函数
	basicBlock(int number, std::vector<CodeItem> Ir, std::set<int> pred, std::set<int> succeeds) {
		this->number = number;
		this->Ir = Ir;
		this->pred = pred;
		this->succeeds = succeeds;
	}
};

class varStruct {
public:
	std::string name;
	std::set<int> blockNums;			// 该变量所在的迭代必经边界，即DF+，见《高级编译器设计与实现》P186
	varStruct(std::string name, std::set<int> blockNums) {
		this->name = name;
		this->blockNums = blockNums;
	}
};

class SSA {
private:
	// std::vector<std::vector<CodeItem>> codetotal;		// 前端传过来的中间代码
	// std::vector<std::vector<symbolTable>> symtotal;		// 前端传过来的符号表
	std::vector<std::vector<int>> blockOrigin;				// 每个基本块的第一条中间代码
	std::vector<std::vector<basicBlock>> blockCore;		// 每个基本块结构
	std::vector<std::vector<int>> postOrder;					// 必经节点数的后序遍历序列
	std::vector<std::vector<int>> preOrder;					// 必经节点数的前序遍历序列
	std::vector<std::vector<varStruct>> varChain;			// 函数内每个局部变量对应的迭代必经边界，用于\phi函数的插入
	// 优化要用到的数据结构
	std::vector<bool> inlineFlag;									// 函数能否内联的标志
	std::map<std::string, int> funName2Num;				// 对应第几个函数的函数名是什么

	void find_primary_statement();								// 找到基本块的每个起始语句
	void divide_basic_block();										// 划分基本块
	void build_pred_and_succeeds();							// 建立基本块间的前序和后序关系
	void build_dom_tree();											// 建立必经节点关系
	void build_idom_tree();											// 建立直接必经关系
	void build_reverse_idom_tree();								// 直接必经节点的反关系
	void post_order(int funNum, int node);
	void build_post_order();											// 后序遍历必经节点树
	void pre_order(int funNum, int node);
	void build_pre_order();											// 前序遍历必经节点树
	void build_dom_frontier();										// 计算流图必经边界
	void use_insert(int funNum, int blkNum, std::string varName);
	void def_insert(int funNum, int blkNum, std::string varName);
	void build_def_use_chain();									// 计算ud链
	void active_var_analyse();										// 活跃变量分析，生成in、out集合
	std::set<int> DF_Set(int funNum, std::set<int> s);
	void build_var_chain();											// 计算函数内每个局部变量对应的迭代必经边界，用于\phi函数的插入
	void renameVar();													// SSA变量重命名
	void add_phi_function();											// 在需要添加基本块的开始添加\phi函数
	int phi_loc_block(int funNum, int blkNum, std::string name, std::vector<bool> visited);			// 查找该节点开始对应的name变量的基本块位置
	void deal_phi_function();										// 处理\phi函数
	void rename_back();												// 将SSA变量带下标的中间代码恢复为正常中间代码，即做完优化后去掉下标
	// 测试专用函数
	void Test_SSA();			// 测试函数的总入口
	void Test_Divide_Basic_Block();	
	void Test_Build_Dom_Tree();
	void Test_Build_Idom_Tree();
	void Test_Build_Reverse_Idom_Tree();
	void Test_Build_Post_Order();
	void Test_Build_Pre_Order();
	void Test_Build_Dom_Frontier();
	void Test_Build_Def_Use_Chain();
	void Test_Active_Var_Analyse();
	void Test_Build_Var_Chain();
	void Test_Add_Phi_Fun();
	// 优化函数
	void ssa_optimize();		// 优化函数入口
	void delete_dead_codes();	// 删除死代码
	void judge_inline_function();	// 判断内联函数
	void inline_function();	// 函数内联
	// 功能函数，可能在各个地方都会用到
	std::string deleteSuffix(std::string name);	// 删除后缀，如输入参数为%a^1，返回%a; 若不含^则直接返回；
	bool ifTempVariable(std::string name);		// 判断是否是临时变量
	bool ifDigit(std::string name);						// 判断是否是数字
	bool ifGlobalVariable(std::string name);		// 判断是否是全局变量
	bool ifLocalVariable(std::string name);		// 判断是否是局部变量
	void simplify_br();										// 简化条件判断为常值的跳转指令
public:
	/*SSA(std::vector<std::vector<CodeItem>> codetotal, std::vector<std::vector<symbolTable>> symTable) {
		this->codetotal = codetotal;
		this->symtotal = symTable;
	}*/
	SSA() {}
	void generate();		// 开始函数
};

#endif //_SSA_H_