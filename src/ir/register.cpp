#include "./register.h"
#include "./optimize.h"

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
	if (tmpReg == "No reg!") {
		return vreg;
	}
	vreg2reg[vreg] = tmpReg;	// vreg <-> reg
	vreguse.push_back(vreg);	// vreguse
	reg2avail[tmpReg] = false;	// 标记为被占用
	return tmpReg;
}

void RegPool::releaseReg(string vreg) {
	if (vreg2reg.find(vreg) != vreg2reg.end()) {
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
	else {
		//WARN_MSG(vreg.c_str());
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
	vreg2spill[vReg] = true;
	stackOffset += 4;
	releaseReg(vReg);					//释放vReg的物理寄存器
	return pair<string, string>(vReg, spillReg);
}

//将当前全部的临时寄存器都溢出，在call前调用保存后续可能用到的中间值
vector<pair<string, string>> RegPool::spillAllRegs() {
	vector<pair<string, string>> res;
	while (!vreguse.empty()) {
		res.push_back(spillReg());
	}
	return res;
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

//中间变量的存储----------------------------------------
map<string, int> vr2index;
int vrNum = 0;

void setVrIndex(string vr) {
	if (vr2index.find(vr) != vr2index.end()) {
		return;
	}
	else {
		//将先前push到栈的中间变量(VR)加一个位置
		vr2index[vr] = vrNum++;
	}
}

//返回vr在栈的地址，相对于sp
string getVrOffset(string vr) {
	if (vr2index.find(vr) != vr2index.end()) {
		return I2A(vr2index[vr] * 4);
	}
	else {
		return "0";
	}
}
//中间变量的存储----------------------------------------

//临时寄存器申请
string allocTmpReg(RegPool& regpool, std::string res, std::vector<CodeItem>& func)
{
	string reg = regpool.getAndAllocReg(res);
	if (reg == res) {	//没有临时寄存器了
		pair<string, string> tmp = regpool.spillReg();
		setVrIndex(tmp.first);
		CodeItem pushInstr(STORE, tmp.second, tmp.first, "");
		func.push_back(pushInstr);
		reg = regpool.getAndAllocReg(res);
	}
	return reg;
}

//获取VR对应的临时寄存器
string getTmpReg(RegPool& regpool, std::string res, std::vector<CodeItem>& func) {
	if (vreg2varReg.find(res) != vreg2varReg.end()) {
		return vreg2varReg[res];
	}
	string reg = regpool.getReg(res);
	if (reg == "spilled") {
		string tmpReg = allocTmpReg(regpool, res, func);
		CodeItem tmp(LOAD, tmpReg, res, "");
		func.push_back(tmp);
		reg = tmpReg;
	}
	else if (reg == res) {
		WARN_MSG("wrong in getTmpReg!");
	}
	return reg;
}

void registerAllocation() {
	vector<vector<CodeItem>> LIRTmp;
	func2gReg.push_back(vector<string>());
	func2Vr.push_back(map<string, int>());
	LIRTmp.push_back(LIR.at(0));

	for (int k = 1; k < LIR.size(); k++) {
		auto func = LIR.at(k);
		auto vars = stackVars.at(k);
		vector<CodeItem> funcTmp;

		//初始化
		vector<string> tmpRegs = { "R0", "R1", "R2", "R3", "R12" };	//临时寄存器池
		RegPool regpool(tmpRegs);
		vreg2varReg.clear();
		first.clear();
		var2reg.clear();
		regBegin = 4;
		vr2index.clear();

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

		string funcName = "";
		for (int i = 0; i < func.size(); i++) {
			CodeItem instr = func.at(i);
			irCodeType op = instr.getCodetype();
			string res = instr.getResult();
			string ope1 = instr.getOperand1();
			string ope2 = instr.getOperand2();
			string resReg = res;
			string ope1Reg = ope1;
			string ope2Reg = ope2;

			//res字段分配临时寄存器
			if (op == DEFINE) {
				funcName = res;
			}else if (op == NOT) {
				if (isFind(ope1, vreg2varReg)) {
					ope1Reg = vreg2varReg[ope2];
				}
				else {
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				}
				regpool.releaseReg(ope1);
				resReg = allocTmpReg(regpool, ope1, funcTmp);
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == ADD || op == SUB || op == DIV || op == MUL || op == REM ||
				op == AND || op == OR ||
				op == EQL || op == NEQ || op == SGT || op == SGE || op == SLT || op == SLE) {
				if (!isNumber(ope2)) {	//不是立即数
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
				}
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseReg(ope1);
				regpool.releaseReg(ope2);
				resReg = allocTmpReg(regpool, res, funcTmp);	//如果寄存器不够就溢出一个
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == LEA) {
				ope1Reg = allocTmpReg(regpool, ope1, funcTmp);
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == MOV) {	//分配临时寄存器
				if (isVreg(ope2)) {
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
					regpool.releaseReg(ope2);
				}
				else if (isString(ope2)) {
					
				}
				if (isVreg(ope1)) {
					ope1Reg = allocTmpReg(regpool, ope1, funcTmp);
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == LOAD) {
				if (ope2 == "para") {	//加载数组参数的地址
					if (var2reg.find(ope1) != var2reg.end()) {	//有寄存器
						instr.setCodetype(MOV);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr("", resReg, var2reg[ope1]);
					}
					else {
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
				else {	//非数组参数
					if (isVreg(ope1)) {		//全局变量
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						regpool.releaseReg(ope1);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//栈变量
						if (var2reg[ope1] == "memory") {	//分配临时寄存器
							resReg = allocTmpReg(regpool, res, funcTmp);
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
			}
			else if (op == LOADARR) {
				if (isNumber(ope2)) {	//偏移是立即数
					if (isVreg(ope1)) {		//全局数组
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						regpool.releaseReg(ope1);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//栈数组
						if (var2reg[ope1] == "memory") {
							resReg = allocTmpReg(regpool, res, funcTmp);
							instr.setInstr(resReg, ope1Reg, ope2Reg);
						}
						else {
							resReg = var2reg[ope1];
							vreg2varReg[res] = resReg;
							instr.setInstr(resReg, ope1Reg, ope2Reg);
						}
					}
				}
				else {					//偏移是寄存器
					if (isVreg(ope1)) {		//全局数组
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						regpool.releaseReg(ope1);
						regpool.releaseReg(ope2);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//栈数组
						if (var2reg[ope1] == "memory") {
							ope2Reg = getTmpReg(regpool, ope2, funcTmp);
							regpool.releaseReg(ope2);
							resReg = allocTmpReg(regpool, res, funcTmp);
							instr.setInstr(resReg, ope1Reg, ope2Reg);
						}
						else {
							ope2Reg = getTmpReg(regpool, ope2, funcTmp);
							regpool.releaseReg(ope2);
							resReg = var2reg[ope1];
							vreg2varReg[res] = resReg;
							instr.setInstr(resReg, ope1Reg, ope2Reg);
						}
					}
				}
			}
			else if (op == STORE) {
				if (isVreg(ope1)) {		//全局变量
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
					resReg = getTmpReg(regpool, res, funcTmp);
					regpool.releaseReg(ope1);
					regpool.releaseReg(res);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
				}
				else {					//栈变量
					if (var2reg[ope1] == "memory") {
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseReg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseReg(res);
						ope1Reg = var2reg[ope1];
						first[ope1] = false;
						instr.setCodetype(MOV);
						instr.setInstr("", ope1Reg, resReg);
					}
				}
			}
			else if (op == STOREARR) {
				if (isNumber(ope2)) {	//偏移是立即数
					if (isVreg(ope1)) {		//全局数组
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseReg(ope1);
						regpool.releaseReg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//栈数组
						//因为是数组，分配的寄存器没有意义，不考虑 “memory”
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseReg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
				else {					//偏移是寄存器
					if (isVreg(ope1)) {		//全局数组
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseReg(ope1);
						regpool.releaseReg(ope2);
						regpool.releaseReg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//栈数组
						//因为是数组，分配的寄存器没有意义，不考虑 “memory”
						resReg = getTmpReg(regpool, res, funcTmp);
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						regpool.releaseReg(ope2);
						regpool.releaseReg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
			}
			else if (op == PUSH) {
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseReg(ope1);
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == POP) {	//貌似用不到
				WARN_MSG("will use this Pop??");
			}
			else if (op == BR) {
				if (isVreg(ope1)) {
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				}
				regpool.releaseReg(ope1);
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == ALLOC) {
				ope1Reg = var2reg[res];
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == PARA) {
				ope1Reg = var2reg[res];
				instr.setInstr(resReg, ope1Reg, ope2Reg);

#define isReg(x) ((x.size() > 1) && (x.at(0) == 'R'))

				if (isReg(ope1)) {
					funcTmp.push_back(CodeItem(MOV, "", ope1Reg, ope1));
					first[res] = false;
				}
			}
			else if (op == GETREG) {
				if (isVreg(ope1)) {
					ope1Reg = allocTmpReg(regpool, ope1, funcTmp);
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == NOTE && ope1 == "func" && ope2 == "begin") {
				auto save = regpool.spillAllRegs();
				funcTmp.push_back(instr);
				for (auto one : save) {
					setVrIndex(one.first);
					CodeItem pushTmp(STORE, one.second, one.first, "");
					funcTmp.push_back(pushTmp);
				}
				continue;
			}
			else {
				//do nothing!
			}
			funcTmp.push_back(instr);
		}
		//处理中间变量的offset
		//for (int j = 0; j < funcTmp.size(); j++) {
		//	CodeItem& instr = funcTmp.at(j);
		//	auto op = instr.getCodetype();
		//	auto ope1 = instr.getOperand1();
		//	if (op == LOAD && isVreg(ope1)) {
		//		instr.setOperand2(getVrOffset(ope1));
		//	}
		//	else if (op == STORE && isVreg(ope1)){
		//		instr.setOperand2(getVrOffset(ope1));
		//	}
		//	else {
		//		//nothing
		//	}
		//}
		LIRTmp.push_back(funcTmp);
		func2Vr.push_back(vr2index);
		vector<string> globalReg;
		for (auto one : var2reg) {
			globalReg.push_back(one.second);
		}
		func2gReg.push_back(globalReg);
	}
	LIR = LIRTmp;
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

