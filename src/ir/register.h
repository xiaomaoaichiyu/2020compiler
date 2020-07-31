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

void registerAllocation();



#endif // _REGISTER_H_
