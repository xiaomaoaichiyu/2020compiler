#ifndef _REGISTER_H_
#define _REGISTER_H_

#include "../util/meow.h"
#include "../ir/ssa.h"

#include <vector>
#include <string>
#include <map>
#include <queue>
#include <set>
using namespace std;

//=================================================================
//1. 寄存器直接指派，临时寄存器使用寄存器池
//=================================================================

class RegPool {
	vector<string> pool;
	map<string, bool> reg2avail;			//记录寄存器是否被使用，true表示可用，false表示被占用
	map<string, string> vreg2reg;			//记录分配给虚拟寄存器的物理寄存器

	map<string, bool> vreg2spill;			//记录是否被溢出
	vector<string> vreguse;					//记录虚拟寄存器分配的时间，用于抢占物理寄存器的退出策略
	int stackOffset = 0;
	map<string, int> vreg2Offset;			//记录被spiil的临时变量在栈中的偏移
public:
	RegPool(vector<string> regs) : pool(regs) {
		for (int i = 0; i < regs.size(); i++) {
			reg2avail[regs.at(i)] = true;
		}
	}
	string getReg(string vreg);
	string getAndAllocReg(string vreg);
	void releaseReg(string vreg);
	CodeItem loadVreg(string vreg);
private:
	string haveAvailReg();
	string allocReg();
	pair<string, string> spillReg();	//返回 <寄存器, 虚拟寄存器> 
};

struct Allocation {
	int from;		//指令id开始
	int to;			//指令id结束
	string reg;		//分配的物理寄存器或者内存位置
};

void registerAllocation(vector<CodeItem>& func);

//=============================================
//2. 线性扫描算法
//=============================================

//1. 基本块线性化
//class BlockOrder {
//	//loop dectection循环检测
//	map<int, basicBlock> blocks;
//	map<int, int> blk2index;
//	map<int, int> blk2depth;
//
//	vector<pair<int, int>> loop_end_lists;			//pair<int, int> : loop_end -> loop_header
//	int index = 0;
//	vector<pair<int, int>> loop_header_index;		//loop_header index
//	vector<int> orders;	//基本块的线性化顺序
//
//public:
//	BlockOrder(vector<basicBlock> blks);
//	void loop_detection(int nblks);
//	void sortInstr();
//};
//
////变量的范围
//struct Range {
//	int from;
//	int to;
//	Range(int _from, int _to) : from(_from), to(_to) {}
//};
//
////变量使用的位置
//struct UsePosition {
//	int position;
//	int use_kind; //1: regster, 2: memory
//	UsePosition(int pos, int kind) : position(pos), use_kind(kind) {}
//};
//
////变量的生存周期
//class Interval {
//	string var;
//	string reg;
//	vector<Range> ranges;
//	vector<UsePosition> positions;
//	Interval* split_parent = nullptr;
//	vector<Interval> split_children;
//	Interval* register_hint = nullptr;
//public:
//	Interval();
//	Interval(string var);
//	~Interval();
//
//	void add_range(int from, int to);
//	void add_usePosition(int pos, int use_kind);
//	Interval split(int op_id);
//};
//
//
//
//class RegsiterAllocation {
//
//public:
//
//};


#endif // _REGISTER_H_
