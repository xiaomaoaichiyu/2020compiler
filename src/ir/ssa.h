#ifndef _SSA_H_
#define _SSA_H_

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <string>
#include "intermediatecode.h"
#include "../front/syntax.h"

// dag
class TreeNode {
public:
	irCodeType op;
	int leftchild, rightchild, father;
	int nodeid;
	bool ifIn;
	std::set<std::string> nameset;
	TreeNode();
};

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
	std::set<int> domin;			// 必经节点集
	std::set<int> idom;				// 直接必经节点集
	std::set<int> reverse_idom;		// 直接必经节点反集
	std::set<int> tmpIdom;			// 计算直接必经节点算法中需要的数据结构
	std::set<int> df;				// 必经边界
	std::set<std::string> use;		// 该基本块中的use变量，按照狼书的定义，定义前使用的变量
	std::set<std::string> def;		// 该基本块中的def变量，按照狼书的定义，使用前定义的变量
	std::set<std::string> def2;	// 只要被定义即加入，不管出现的顺序
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
	// std::vector<std::vector<CodeItem>> codetotal;	// 前端传过来的中间代码
	// std::vector<std::vector<symbolTable>> symtotal;	// 前端传过来的符号表
	std::vector<std::vector<int>> blockOrigin;				// 每个基本块的第一条中间代码
	std::vector<std::vector<basicBlock>> blockCore;		// 每个基本块结构
	std::vector<std::vector<int>> postOrder;					// 必经节点数的后序遍历序列
	std::vector<std::vector<int>> preOrder;					// 必经节点数的前序遍历序列
	std::vector<std::vector<varStruct>> varChain;			// 函数内每个局部变量对应的迭代必经边界，用于\phi函数的插入
	// 优化要用到的数据结构
	std::vector<bool> inlineFlag;									// 函数能否内联的标志
	std::map<std::string, int> funName2Num;				// 对应第几个函数的函数名是什么
	std::map<int, std::string> funNum2Name;				// 函数名和函数序号的对应关系

	std::vector<int> addrIndex;										// 数组参数命名编号
	std::vector<int> labelIndex;										// 函数内部标签改名编号
	std::vector<std::set<std::string>> inlineArrayName;// 内联函数数组传参新定义变量名
	std::vector<int> funCallCount;									// 记录一个函数第几次被调用

	std::vector<std::map<std::string, symbolTable>> varName2St;	// 变量名
	std::map<std::string, int> regName2Num{ {"R4", 4}, {"R5", 5}, {"R6", 6}, {"R7", 7}, {"R8", 8}, {"R9", 9}, {"R10", 10}, {"R11", 11} };
	std::map<int, std::string> regNum2Name{ {4, "R4"}, {5, "R5"}, {6, "R6"}, {7, "R7"}, {8, "R8"}, {9, "R9"}, {10, "R10"}, {11, "R11"} };
	const int MAX_ALLOC_GLOBAL_REG = 8;
	const int ALLOC_GLOBAL_REG_START = 4;
	std::vector<std::map<std::string, std::string>> var2reg;
	std::vector<std::map<std::string, std::set<std::string>>> clash_graph;
	std::vector<std::stack<std::string>> allocRegList;

	void find_primary_statement();								// 找到基本块的每个起始语句
	void divide_basic_block();										// 划分基本块
	void simplify_basic_block();									// 消除无法到达基本块
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
	void def2_insert(int funNum, int blkNum, std::string varName);
	void build_def_use_chain();									// 计算ud链
	void active_var_analyse();										// 活跃变量分析，生成in、out集合
	std::set<int> DF_Set(int funNum, std::set<int> s);
	void build_var_chain();											// 计算函数内每个局部变量对应的迭代必经边界，用于\phi函数的插入
	void renameVar();													// SSA变量重命名
	void add_phi_function();											// 在需要添加基本块的开始添加\phi函数
	int phi_loc_block(int funNum, int blkNum, std::string name, std::vector<bool> visited, int insertBlk);			// 查找该节点开始对应的name变量的基本块位置
	void deal_phi_function();										// 处理\phi函数
	void add_phi_to_Ir();												// 将phi函数加入到中间代码
	void delete_Ir_phi();												// 删除中间代码中的phi
	void rename_back();												// 将SSA变量带下标的中间代码恢复为正常中间代码，即做完优化后去掉下标
	void turn_back_codetotal();										//将SSA代码格式改回到codetotal格式
	
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
	void Test_Build_Clash_Graph(ofstream& debug_reg);
	void Test_Allocate_Global_Reg(ofstream& debug_reg);
	// 优化函数
	// void pre_optimize();	// 在睿轩生成的中间代码上做优化
	void simplify_br();		// 简化条件判断为常值的跳转指令
	void load_and_store();// 简化load和store指令相邻的指令
	void simplify_add_minus_multi_div_mod();	// 简化加减0、乘除模1这样的指令
	void simplify_br_label();	// 简化紧邻的跳转
	void ssa_optimize();		// 优化函数入口
	void delete_dead_codes();	// 删除死代码
	void inline_function();	// 函数内联
	std::string getNewInsertAddr(int funNum, std::string name);
	std::string getNewInsertLabel(int funNum, std::string name);
	std::string getNewFunEndLabel(int funNum, std::string name);
	std::string getFunEndLabel(int funNum, std::string name);
	std::string getRetValName(std::string name);
	void simplify_alloc();// 删除多余alloc指令
	void simpify_duplicate_assign();

	// 图着色算法分配寄存器
	void build_clash_graph();	// 构建冲突图
	void allocate_global_reg();	// 分配全局寄存器
	std::string get_new_global_reg(int funNum, std::string name);	// 获得一个全局寄存器
	
	//ly：循环优化：代码外提  待做：强度削弱、规约变量删除
	void count_UDchains();	//计算使用-定义链 用来查找不变式代码
	void back_edge();	//计算回边+查找循环
	void code_outside();	//代码外提
	void strength_reduction();	//强度削弱
	void protocol_variable_deletion(); //规约变量删除
	void mark_invariant();				//标记不变式
	bool condition1(set<int> outBlk, int instrBlk, int func);
	bool condition2(set<int> outBlk, string var, int func);

	
	void const_propagation();	//常量传播
	void copy_propagation();	//复写传播
	//ly：循环优化

	// 功能函数，可能在各个地方都会用到
	std::string deleteSuffix(std::string name);	// 删除后缀，如输入参数为%a^1，返回%a; 若不含^则直接返回；
	bool ifTempVariable(std::string name);		// 判断是否是临时变量
	bool ifDigit(std::string name);						// 判断是否是数字
	bool ifGlobalVariable(std::string name);		// 判断是否是全局变量
	bool ifLocalVariable(std::string name);		// 判断是否是局部变量
	bool ifVR(std::string name);						// 判断是否是虚拟寄存器
	bool ifRegister(std::string name);				// 判断是否是寄存器
	void init();													// 初始化varName2St结构体
	int str2int(std::string name);						// 字符串转int


	// dag
	void build_dag();	// 构建dag图
	bool ifCalIr(irCodeType ict);	// 是否是运算型指令
	void delete_common_sub_exp(int funNum, int blkNum, int IrStart, int IrEnd);
	std::string getNewTempVariable();

	void optimize_arrayinit();
	void optimize_delete_dead_codes();

public:
	/*SSA(std::vector<std::vector<CodeItem>> codetotal, std::vector<std::vector<symbolTable>> symTable) {
		this->codetotal = codetotal;
		this->symtotal = symTable;
	}*/
	SSA() {}
	void generate();		// 开始函数
	void generate_active_var_analyse();	// 仅进行活跃变量分析
	void registerAllocation();
	// 优化函数
	void pre_optimize();	// 在睿轩生成的中间代码上做优化
	void get_avtiveAnalyse_result();
	std::vector<std::map<std::string, std::string>> getvar2reg();
	vector<vector<basicBlock>>& getblocks() {
		return this->blockCore;
	}
	vector<set<string>>& getInlineArrayName() {
		return this->inlineArrayName;
	}
};

class ActiveAnalyse {
public:
	vector<map<int, set<string>>> func2in, func2out, func2def, func2use;
	
	ActiveAnalyse() {}
	void print_ly_act();
};

extern ActiveAnalyse ly_act;

string calculate(irCodeType op, string ope1, string ope2);
void printCircleIr(vector<vector<basicBlock>>& v, ofstream& debug_ssa);

//==============================
// 循环
//==============================

class Circle {
public:
	set<int> cir_blks;	//循环的基本块结点
	set<int> cir_outs;	//循环的退出结点
	int cir_begin;
	Circle() {}
	Circle(set<int>& blks) : cir_blks(blks) {}
};

//记录每个函数的循环
extern vector<vector<Circle>> func2circles;

#endif //_SSA_H_