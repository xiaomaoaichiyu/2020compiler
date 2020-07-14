#include "./register.h"

//=================================================================
// 寄存器池
//=================================================================

string RegPool::getReg(string vreg) {
	if (vreg2reg.find(vreg) != vreg2reg.end()) {
		return vreg2reg[vreg];	//返回分配的寄存器
	}
	else if (vreg2spill.find(vreg) != vreg2spill.end()){	//如果没有被分配寄存器就返回虚拟寄存器
		return "spilled";
	}
	else {
		return vreg;
	}
}

string RegPool::getAndAllocReg(string vreg) {
	string tmpReg = haveAvailReg();
	if (tmpReg == "No Reg!") {
		return vreg;
	}
	vreg2reg[vreg] = tmpReg;	// vreg <-> reg
	vreguse.push_back(vreg);	// vreguse
	reg2avail[tmpReg] = false;	// 标记为被占用
	return tmpReg;
}

void RegPool::releaseReg(string vreg) {
	string useReg = vreg2reg[vreg];
	vreg2reg.erase(vreg);
	reg2avail[useReg] = true;
	for (auto it = vreguse.begin(); it != vreguse.end();) {
		if (*it == vreg) {
			it = vreguse.erase(it);
			continue;
		}
		it++;
	}
}

//void releaseStack(string vreg) {
//
//}


pair<string, string> RegPool::spillReg() {
	string vReg = vreguse.front();		//spill的虚拟寄存器vReg
	string spillReg = vreg2reg[vReg];
	reg2avail[spillReg] = true;			//对应的物理寄存器标记为可用
	vreg2Offset[vReg] = stackOffset;	//vReg在栈的偏移
	stackOffset += 4;
	releaseReg(vReg);					//释放vReg的物理寄存器
	return pair<string, string>(vReg, spillReg);
}

int RegPool::getStackOffset(string vreg)
{
	return vreg2Offset[vreg];
}

//private

string RegPool::haveAvailReg() {
	for (int i = 0; i < pool.size(); i++) {
		if (reg2avail[pool.at(i)] == true) return pool.at(i);
	}
	return "No reg!";
}

string RegPool::allocReg()
{
	string res = haveAvailReg();
	if (res == "No reg!") {
		WARN_MSG("RegPool alloc wrong!");
	}
	return res;
}


// class RegPool end--------------------------------------------------


//===================================================================
// 寄存器分配算法v1
// 接变量指派，多余变量采用load加载数据到临时寄存器
//===================================================================

map<string, string> vreg2varReg;	//记录虚拟寄存器和变量之间的关系

map<string, bool> first;			//标志有寄存器变量是否是第一次访问
map<string, string> var2reg;
int regBegin = 4;

//查找一个虚拟寄存器是否和变量的寄存器对应
bool isFind(string vreg, map<string, string> container) {
	if (container.find(vreg) != container.end()) {
		return true;
	}
	return false;
}

string allocTmpReg(RegPool& regpool, std::string res, std::vector<CodeItem>& func, int& i)
{
	string reg = regpool.getAndAllocReg(res);
	if (reg == res) {	//没有临时寄存器了
		pair<string, string> tmp = regpool.spillReg();
		CodeItem pushInstr(PUSH, "", tmp.second, tmp.first);
		func.insert(func.begin() + i, pushInstr);
		i++;
		reg = regpool.getAndAllocReg(res);
	}
	return reg;
}

string getTmpReg(RegPool& regpool, std::string res, std::vector<CodeItem>& func, int& i) {
	if (vreg2varReg.find(res) != vreg2varReg.end()) {
		return vreg2varReg[res];
	}
	string reg = regpool.getReg(res);
	if (reg == "spilled") {
		string tmpReg = allocTmpReg(regpool, res, func, i);
		int offset = regpool.getStackOffset(res);
		CodeItem tmp(LOAD, tmpReg, res, I2A(offset));
		i++;
		reg = tmpReg;
	}
	else if (reg == res) {
		WARN_MSG("wrong in getTmpReg!");
	}
	regpool.releaseReg(res);			//临时变量只会用一次
	return reg;
}

void registerAllocation(vector<CodeItem>& func, vector<string> vars) {
	vector<string> tmpRegs = { "R0", "R1", "R2", "R3", "R12" };	//临时寄存器池
	RegPool regpool(tmpRegs);

	vreg2varReg.clear();
	first.clear();
	var2reg.clear();
	regBegin = 4;
	//无脑指派局部变量
	int i = 0;
	for (; i < vars.size(); i++) {
		var2reg[vars.at(i)] = FORMAT("R{}", regBegin++);
		first[vars.at(i)] = true;
		if (regBegin >= 12) {
			i++;
			break;
		}
	}
	for (; i < vars.size(); i++) {
		var2reg[vars.at(i)] = "memory";
		first[vars.at(i)] = true;
	}
	//全局变量使用临时变量来加载和使用

#define isVreg(reg) (reg.size() > 2 && (reg.substr(0,2) == "VR"))

	for (int i = 0; i < func.size(); i++) {
		CodeItem& instr = func.at(i);
		irCodeType op = instr.getCodetype();
		string res = instr.getResult();
		string ope1 = instr.getOperand1();
		string ope2 = instr.getOperand2();
		string resReg = res;
		string ope1Reg = ope1;
		string ope2Reg = ope2;
		
		//res字段分配临时寄存器
		if (op == NOT) {
			if (isFind(ope1, vreg2varReg)) {
				ope1Reg = vreg2varReg[ope2];
			}
			else {
				ope1Reg = getTmpReg(regpool, ope1, func, i);
			}
			resReg = allocTmpReg(regpool, ope1, func, i);
			instr.setInstr(resReg, ope1Reg, ope2Reg);
		}
		else if (op == ADD || op == SUB || op == DIV || op == MUL || op == REM ||
			op == AND || op == OR ||
			op == EQL || op == NEQ || op == SGT || op == SGE || op ==SLT || op == SLE) {
			if (!isNumber(ope2)) {	//不是立即数
				if (isFind(ope2, vreg2varReg)) {	//vreg是变量寄存器的映射
					ope2Reg = vreg2varReg[ope2];
				}
				else {	//之前申请的临时寄存器
					ope2Reg = getTmpReg(regpool, ope2, func, i);
				}
			}
			if (isFind(ope1, vreg2varReg)) {
				ope1Reg = vreg2varReg[ope1];
			}
			else {
				ope1Reg = getTmpReg(regpool, ope1, func, i);
			}
			resReg = allocTmpReg(regpool, res, func, i);	//如果寄存器不够就溢出一个
			instr.setInstr(resReg, ope1Reg, ope2Reg);
		}
		else if (op == LEA) {
			ope1Reg = allocTmpReg(regpool, ope1, func, i);
			instr.setInstr(resReg, ope1Reg, ope2Reg);
		}
		else if (op == MOV) {	//分配临时寄存器
			if (isVreg(ope2)) {
				ope2Reg = getTmpReg(regpool, ope2, func, i);
			}
			if (isVreg(ope1)) {
				ope1Reg = allocTmpReg(regpool, ope1, func, i);
			}
			instr.setInstr(resReg, ope1Reg, ope2Reg);
		}
		else if (op == LOAD) {
			if (isVreg(ope1)){		//全局变量
				ope1Reg = getTmpReg(regpool, ope1, func, i);
				resReg = allocTmpReg(regpool, ope1, func, i);
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else {					//栈变量
				if (var2reg[ope1] == "memory") {	//分配临时寄存器
					resReg = allocTmpReg(regpool, res, func, i);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
				}
				else {								//指派的全局寄存器
					if (first[ope1] == false) {
						ope1Reg = var2reg[ope1];
						vreg2varReg[res] = ope1Reg;		//vreg <-> varReg
						instr.setCodetype(MOV);
						instr.setInstr("", ope1Reg, ope1Reg);
					}
					else {
						first[ope1] = false;
						resReg = var2reg[ope1];
						vreg2varReg[res] = resReg;		//vreg <-> varReg
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
			}
		}
		else if (op == LOADARR) {
			if (isNumber(ope2)) {	//偏移是立即数
				if (isVreg(ope1)) {		//全局数组
					ope1Reg = getTmpReg(regpool, ope1, func, i);
					resReg = allocTmpReg(regpool, res, func, i);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
				}
				else {					//栈数组
					if (vreg2varReg[ope1] == "memory") {
						resReg = allocTmpReg(regpool, res, func, i);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {
						resReg = vreg2varReg[ope1];
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
			}
			else {					//偏移是寄存器
				if (isVreg(ope1)) {		//全局数组
					ope1Reg = getTmpReg(regpool, ope1, func, i);
					ope2Reg = getTmpReg(regpool, ope2, func, i);
					resReg = allocTmpReg(regpool, res, func, i);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
				}
				else {					//栈数组
					if (vreg2varReg[ope1] == "memory") {
						ope2Reg = getTmpReg(regpool, ope2, func, i);
						resReg = allocTmpReg(regpool, res, func, i);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {
						ope2Reg = getTmpReg(regpool, ope2, func, i);
						resReg = vreg2varReg[ope1];
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
			}
		}
		else if (op == STORE) {
			if (isVreg(ope1)) {		//全局变量
				ope1Reg = getTmpReg(regpool, ope1, func, i);
				resReg = getTmpReg(regpool, res, func, i);
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}	
			else {					//栈变量
				if (var2reg[ope1] == "memory") {
					resReg = getTmpReg(regpool, res, func, i);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
				}
				else {
					resReg = getTmpReg(regpool, res, func, i);
					ope1Reg = var2reg[ope1];
					first[ope1] = false;
					instr.setCodetype(MOV);
					instr.setInstr("", ope1Reg, resReg);
				}
			}
		}
		else if (op == STOREARR) {
			
		}
		else if (op == PUSH) {
			ope1Reg = getTmpReg(regpool, ope1, func, i);
			instr.setInstr(resReg, ope1Reg, ope2Reg);
		}
		else if (op == POP) {	//貌似用不到
			WARN_MSG("will use this Pop??");
		}
		else if (op == BR) {
			if (isVreg(ope1)) {
				ope1Reg = getTmpReg(regpool, ope1, func, i);
			}
			instr.setInstr(resReg, ope1Reg, ope2Reg);
		}
		else if (op == ALLOC) {
			ope1Reg = var2reg[res];
			instr.setInstr(resReg, ope1Reg, ope2Reg);
		}
		else {
			//do nothing!
		}
	}
}






//=====================================================================================
//线性扫描寄存器分配
//=====================================================================================
//
//BlockOrder::BlockOrder(vector<basicBlock> blks) {
//	int i = 0;
//	for (; i < blks.size(); i++) {
//		int blknumber = blks[i].number;
//		blocks[blknumber] = blks[i];
//		blk2index[blknumber] = -1;	//基本块的index初始化为-1
//		blk2depth[blknumber] = 0;	//基本块的depth初始化为0
//	}
//}
//
////循环检测，给每一个基本块计算index和depth
//void BlockOrder::loop_detection(int nblks) {
//	queue<int> work;
//	map<int, int> active;							//后继是否被问过
//	map<int, bool> visited;							//是否被访问过
//	map<int, int> depths;						//block's loop depth
//	for (auto it = blocks.begin(); it != blocks.end(); it++) {
//		int blkNumber = it->second.number;
//		int sucNumber = it->second.succeeds.size();
//		active[blkNumber] = sucNumber;
//		visited[blkNumber] = false;
//		depths[blkNumber] = 0;
//	}
//
//	work.push(0);
//	while (!work.empty()) {
//		int init = work.front();
//		work.pop();
//		//标记为visited
//		visited.at(init) = true;
//		//处理后继
//		set<int> succedds = blocks[init].succeeds;
//		for (auto i : succedds) {
//			//判断是否是循环
//			if (active[i] != 0) {
//				loop_end_lists.push_back(pair<int, int>(init, i));
//				loop_header_index.push_back(pair<int, int>(i, index++));
//			}
//			//没访问过就加入队列
//			if (visited.at(i) == false) {
//				work.push(i);
//			}
//		}
//		//处理前缀
//		set<int> preds = blocks[init].pred;
//		for (auto i : preds) {
//			active[i]--;
//		}
//	}
//
//	//从loop_end开始逆序遍历实现loop_depth
//	for (auto it : loop_end_lists) {
//		int header = it.second;
//		int end = it.first;
//		queue<int> tmp;
//		map<int, bool> visit;
//		while (end != header) {
//
//		}
//	}
//}
//
//void BlockOrder::sortInstr() {
//
//}
//

