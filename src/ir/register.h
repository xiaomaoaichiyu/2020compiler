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
public:
	RegPool(vector<string> regs) : pool(regs) {
		for (int i = 0; i < regs.size(); i++) {
			reg2avail[regs.at(i)] = true;
		}
	}
	string getReg(string vreg);
	void markParaReg(string reg, string Vreg);
	string getAndAllocReg(string vreg);
	void releaseVreg(string vreg);
	pair<string, string> spillReg();				//返回 <寄存器, 虚拟寄存器> 
	vector<pair<string, string>> spillAllRegs();
private:
	string haveAvailReg();
	string allocReg();
};

class GlobalRegPool {
	vector<string> pool;			//全局寄存器池
	map<string, bool> reg2avail;	//记录寄存器是否能够使用
	map<string, string> var2reg;	//记录变量对应的寄存器

	map<string, bool> used;		//用来记录全局寄存器是否被使用过
public:
	GlobalRegPool(vector<string> regs) : pool(regs) {
		for (int i = 0; i < regs.size(); i++) {
			reg2avail[regs.at(i)] = true;
			used[regs.at(i)] = false;
		}
	}
	string getReg(string var);		//获取一个变量的寄存器，如果没有返回"memory"
	string allocReg(string var);	//给一个变量申请全局寄存器，如果没有全局寄存器可用返回"memory"
	void releaseReg(string var);	//释放这个变量的全局寄存器
	void releaseNorActRegs(set<string> inVars, set<string> outVars);
	vector<int> getUsedRegs();
	void noteRegRelations(vector<CodeItem>& func);
};


void registerAllocation();

void registerAllocation2(vector<vector<basicBlock>>& lir);

#endif // _REGISTER_H_
