#include "./register.h"
#include "./optimize.h"
#include <algorithm>
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

void RegPool::markParaReg(string reg, string Vreg) {
	vreg2reg[Vreg] = reg;
	reg2avail[reg] = false;
	vreguse.push_back(Vreg);
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

void RegPool::releaseVreg(string vreg) {
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

pair<string, string> RegPool::spillReg() {
	string vReg = vreguse.front();		//spill的虚拟寄存器vReg
	string spillReg = vreg2reg[vReg];
	reg2avail[spillReg] = true;			//对应的物理寄存器标记为可用
	vreg2spill[vReg] = true;			//标记对应的VR为溢出状态
	releaseVreg(vReg);					//释放vReg的物理寄存器
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

map<string, string> vreg2varReg;	//记录虚拟寄存器和变量寄存器之间的关系
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
//中间变量的存储----------------------------------------

int callLayer = -1;
vector<map<string, string>> callRegs;		//存储 r[0-3] <-> VR[0-9]
vector<map<string, bool>>   paraReg2push;	//参数寄存器被压栈保存的（其实是str保存）


//临时寄存器申请
string allocTmpReg(RegPool& regpool, std::string res, std::vector<CodeItem>& func)
{
	string reg = regpool.getAndAllocReg(res);
	if (reg == res) {	//没有临时寄存器了
		pair<string, string> tmp = regpool.spillReg();	//溢出了寄存器
		setVrIndex(tmp.first);
		//溢出的寄存器是参数寄存器就标记为true;
		if (callLayer >= 0 && callRegs.at(callLayer).find(tmp.second) != callRegs.at(callLayer).end()) {
			paraReg2push.at(callLayer)[tmp.second] = true;
		}
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
	if (reg == "spilled") {		//处理被溢出的寄存器
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
		auto arr = var2Arr.at(k);
		vector<CodeItem> funcTmp;

		//初始化

		// R12作为临时寄存器
		 vector<string> tmpRegs = { "R0", "R1", "R2", "R3", "R12" };	//临时寄存器池
		
		// R12不作为临时寄存器
		//vector<string> tmpRegs = { "R0", "R1", "R2", "R3" };	//临时寄存器池

		RegPool regpool(tmpRegs);
		vreg2varReg.clear();
		first.clear();
		var2reg.clear();
		regBegin = 4;
		vr2index.clear();
		vrNum = 0;

		//多层函数调用参数寄存器保存工作
		int vrNumber = func2vrIndex.at(k);
		callLayer = -1;
		callRegs.clear();		//存储 r[0-3] <-> VR[0-9]
		paraReg2push.clear();	//参数寄存器被压栈保存的（其实是str保存）
		
		//这个地方可以优化，
		//1. 局部数组实际上并不需要寄存器
		//2. 活跃变量分析动态释放不再使用的变量占用的寄存器
								
		//无脑指派局部变量
		int i = 0;
		for (; i < vars.size(); i++) {
			//if (arr[vars.at(i)] != true) {
			//	var2reg[vars.at(i)] = FORMAT("R{}", regBegin++);
			//	first[vars.at(i)] = true;
			//	if (regBegin >= 12) {
			//		i++;
			//		break;
			//	}
			//}
			//else {	//数组不分配寄存器
			//	var2reg[vars.at(i)] = "memory";
			//	first[vars.at(i)] = true;
			//}
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
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseVreg(ope1);
				if (isVreg(res)) {
					resReg = allocTmpReg(regpool, res, funcTmp);	//如果寄存器不够就溢出一个
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == ADD || op == MUL) {	//先申请两个后释放
				if (!isNumber(ope2)) {	//不是立即数
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
				}
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseVreg(ope1);
				regpool.releaseVreg(ope2);
				resReg = allocTmpReg(regpool, res, funcTmp);	//如果寄存器不够就溢出一个
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == AND || op == OR ||
					 op == EQL || op == NEQ || op == SGT || op == SGE || op == SLT || op == SLE) {
				if (!isNumber(ope2)) {	//不是立即数
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
				}
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseVreg(ope1);
				regpool.releaseVreg(ope2);
				if (isVreg(res)) {
					resReg = allocTmpReg(regpool, res, funcTmp);	//如果寄存器不够就溢出一个
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == SUB || op == DIV || op == REM) {	//先申请两个后释放
				if (!isNumber(ope2)) {	//不是立即数
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
				}
				if (!isNumber(ope1)) {	//不是立即数
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				}
				regpool.releaseVreg(ope1);
				regpool.releaseVreg(ope2);
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
					regpool.releaseVreg(ope2);
				}
				if (isVreg(ope1)) {
					ope1Reg = allocTmpReg(regpool, ope1, funcTmp);
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == LOAD) {
				if (ope2 == "para") {	//加载数组参数的地址
					if (var2reg[ope1] != "memory") {	//有寄存器
						instr.setCodetype(MOV);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr("", resReg, var2reg[ope1]);
					}
					else {
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
				else if (ope2 == "array") {	//加载局部数组的地址
					if (var2reg[ope1] != "memory") {	//有寄存器
						resReg = var2reg[ope1];
					}
					else {
						resReg = allocTmpReg(regpool, res, funcTmp);
					}
					instr.setCodetype(LEA);
					instr.setInstr("", resReg, ope1Reg);
				}
				else {	//非数组地址加载
					if (isVreg(ope1)) {		//全局变量
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						regpool.releaseVreg(ope1);
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
						regpool.releaseVreg(ope1);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//栈数组
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
				else {					//偏移是寄存器
					if (isVreg(ope1)) {		//全局数组
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						regpool.releaseVreg(ope1);
						regpool.releaseVreg(ope2);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//栈数组
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						regpool.releaseVreg(ope2);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
			}
			else if (op == STORE) {
				if (isVreg(ope1)) {		//全局变量
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
					resReg = getTmpReg(regpool, res, funcTmp);
					regpool.releaseVreg(ope1);
					regpool.releaseVreg(res);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
				}
				else {					//栈变量
					if (var2reg[ope1] == "memory") {
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseVreg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseVreg(res);
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
						regpool.releaseVreg(ope1);
						regpool.releaseVreg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//栈数组
						//因为是数组，分配的寄存器没有意义，不考虑 “memory”
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseVreg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
				else {					//偏移是寄存器
					if (isVreg(ope1)) {		//全局数组
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseVreg(ope1);
						regpool.releaseVreg(ope2);
						regpool.releaseVreg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//栈数组
						//因为是数组，分配的寄存器没有意义，不考虑 “memory”
						resReg = getTmpReg(regpool, res, funcTmp);
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						regpool.releaseVreg(ope2);
						regpool.releaseVreg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
			}
			else if (op == POP) {	//貌似用不到
				WARN_MSG("will use this Pop??");
			}
			else if (op == BR) {
				if (isVreg(ope1)) {
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
					regpool.releaseVreg(ope1);
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == ALLOC) {
				ope1Reg = var2reg[res];
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == PARA) {
				ope1Reg = var2reg[res];
				instr.setInstr(resReg, ope1Reg, ope2Reg);

				if (isReg(ope1)) {
					funcTmp.push_back(CodeItem(MOV, "", ope1Reg, ope1));
					first[res] = false;
				}
				else if (ope1 == "stack" && var2reg[res] != "memory") {	//????
					funcTmp.push_back(CodeItem(LOAD, var2reg[res], res, "para"));
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
				//记录函数的参数寄存器的情况	
				callLayer++;
				callRegs.push_back(map<string, string>());
				paraReg2push.push_back(map<string, bool>());
				//表达式计算的中间变量的保存！
				auto save = regpool.spillAllRegs();
				funcTmp.push_back(instr);
				for (auto one : save) {		//函数调用的时候临时寄存器的值需要保存
					setVrIndex(one.first);
					//如果溢出的寄存器是push参数寄存器
					if (callLayer > 0) {
						if (callRegs.at(callLayer - 1).find(one.second) != callRegs.at(callLayer - 1).end()) {
							paraReg2push.at(callLayer - 1)[one.second] = true;
						}
					}
					CodeItem pushTmp(STORE, one.second, one.first, "");
					funcTmp.push_back(pushTmp);
				}
				continue;
			}
			else if (op == NOTE && ope1 == "func" && ope2 == "end") {
				//nothing
			}
			else if (op == CALL) {
				//如果函数嵌套导致r0-r3被spill，在函数调用前重取r0-r3
				if (callLayer >= 0) {
					//如果是中间变量把参数寄存器溢出了，怎么标记
					for (auto para : paraReg2push.at(callLayer)) {
						//??????
						if (para.second == true) {		//如果参数寄存器被溢出了，需要load回来
							string vRegTmp = callRegs.at(callLayer)[para.first];
							CodeItem loadTmp(LOAD, para.first, vRegTmp, "para");
							funcTmp.push_back(loadTmp);
						}
					}
				}
				for (auto para : callRegs.at(callLayer)) {		//释放参数寄存器 R0-R3
					regpool.releaseVreg(para.second);
				}
				callLayer--;			//函数层级降低
				callRegs.pop_back();	//函数调用寄存器删除
				paraReg2push.pop_back();
			}
			else if (op == PUSH) {	//参数压栈 4是分界点
				if (A2I(ope2) <= 4) {	//前4个此参数
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
					regpool.releaseVreg(ope1);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
					
					int num = A2I(ope2);
					string paraReg = FORMAT("R{}", num - 1);
					//push参数后标记对应的参数寄存器r0-3为占用状态！
					//这里r0还可能被占用吗？？？
					//理论上不存在，因为中间变量不会夸参数出现，要是被使用了，也一定被释放了
					string curVreg = FORMAT("VR{}", vrNumber++);	//这个虚拟寄存器不会在其他地方出现，除了load
					regpool.markParaReg(paraReg, curVreg);		//标记参数寄存器被占用
					callRegs.at(callLayer)[paraReg] = curVreg;	//push一个意味着这个参数寄存器被占用了
					//后续可能需要压栈保存值！
				}
				else {					//栈参数
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
					regpool.releaseVreg(ope1);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
				}
			}
			else {
				//do nothing!
			}
			funcTmp.push_back(instr);
		}
		LIRTmp.push_back(funcTmp);
		func2Vr.push_back(vr2index);
		vector<string> globalReg;
		for (int l = 4; l < regBegin; l++) {
			globalReg.push_back(FORMAT("R{}", l));
		}
		func2gReg.push_back(globalReg);
	}
	LIR = LIRTmp;
}

//==================================================================================
// 寄存器分配2
//==================================================================================

string GlobalRegPool::getReg(string var) {
	for (auto one : var2reg) {
		if (one.first == var) {
			return one.second;
		}
	}
	return "memory";
}

string GlobalRegPool::allocReg(string var) {
	for (int i = 0; i < pool.size(); i++) {
		if (reg2avail[pool.at(i)] == true) {	//找到了可用寄存器
			reg2avail[pool.at(i)] = false;		//标记为使用
			var2reg[var] = pool.at(i);			//记录变量和寄存器的映射关系
			used[pool.at(i)] = true;			//只要全局寄存器被使用过就记录下来
			return pool.at(i);
		}
	}
	return "memory";
}

void GlobalRegPool::releaseReg(string var) {
	if (var2reg.find(var) == var2reg.end()) {
		WARN_MSG("delete a var with no reg!");
		return;
	}
	string reg = var2reg[var];
	reg2avail[reg] = true;									//标记全局寄存器可用！
	for (auto it = var2reg.begin(); it != var2reg.end();) {	//删除之前 变量 <-> 全局寄存器 的映射关系
		if (it->first == var) {
			it = var2reg.erase(it);
			continue;
		}
		it++;
	}
}

void GlobalRegPool::releaseNorActRegs(set<string> inVars, set<string> outVars) {
	for (auto it = var2reg.begin(); it != var2reg.end();) {
		//在in活跃，在out不活跃，可以释放寄存器
		if (outVars.find(it->first) == outVars.end()) {
			if (inVars.find(it->first) != inVars.end()) {
				it++;
				continue;
			}
			string reg = it->second;
			reg2avail[reg] = true;
			it = var2reg.erase(it);
			continue;
		}
		it++;
	}
}

vector<int> GlobalRegPool::getUsedRegs() {
	vector<int> res;
	for (auto one : used) {
		if (one.second == true) {
			res.push_back(A2I(one.first.substr(1)));
		}
	}
	return res;
}

void GlobalRegPool::noteRegRelations(vector<CodeItem>& func) {
	for (auto one : var2reg) {
		CodeItem tmp(NOTE, FORMAT("{} -> {}", one.first, one.second), "note", "");
		func.push_back(tmp);
	}
}

//void registerAllocation2(vector<vector<basicBlock>>& lir) {
//	vector<vector<CodeItem>> LIRTmp;
//	func2gReg.push_back(vector<string>());
//	func2Vr.push_back(map<string, int>());
//	LIRTmp.push_back(LIR.at(0));
//
//	for (int k = 1; k < lir.size(); k++) {
//		auto func = lir.at(k);
//		auto vars = stackVars.at(k);
//		vector<CodeItem> funcTmp;
//
//		//初始化
//		vector<string> tmpRegs = { "R0", "R1", "R2", "R3", "R12" };	//临时寄存器池
//		RegPool regpool(tmpRegs);
//		vector<string> tmpGregs = { "R4","R5","R6", "R7", "R8", "R9", "R10", "R11" };	//全局寄存器
//		GlobalRegPool gRegpool(tmpGregs);
//
//		vreg2varReg.clear();	//只在load使用
//		first.clear();
//		vr2index.clear();
//		vrNum = 0;
//
//		//多层函数调用参数寄存器保存工作
//		int vrNumber = func2vrIndex.at(k);
//		callLayer = -1;
//		callRegs.clear();		//存储 r[0-3] <-> VR[0-9]
//		paraReg2push.clear();	//参数寄存器被压栈保存的（其实是str保存）
//
//		//1. 局部数组实际上并不需要寄存器
//		//2. 活跃变量分析动态释放不再使用的变量占用的寄存器
//		//全局变量使用临时变量来加载和使用
//
//		//变量一开始全局初始化为memory
//		for (int m = 0; m < vars.size(); m++) {	//数组变量不分配寄存器
//			var2reg[vars.at(m)] = gRegpool.allocReg(vars.at(m));
//		}
//
//		string funcName = "";
//		for (int i = 0; i < func.size(); i++) {
//			auto ir = func.at(i).Ir;
//			auto in = func.at(i).in;	//基本块活跃变量in
//			auto out = func.at(i).out;	//基本块活跃变量out
//			for (int j = 0; j < ir.size(); j++) {
//				CodeItem instr = ir.at(j);
//				irCodeType op = instr.getCodetype();
//				string res = instr.getResult();
//				string ope1 = instr.getOperand1();
//				string ope2 = instr.getOperand2();
//				string resReg = res;
//				string ope1Reg = ope1;
//				string ope2Reg = ope2;
//
//				//res字段分配临时寄存器
//				if (op == DEFINE) {
//					funcName = res;
//				}
//				else if (op == NOT) {
//					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//					regpool.releaseVreg(ope1);
//					resReg = allocTmpReg(regpool, res, funcTmp);
//					instr.setInstr(resReg, ope1Reg, ope2Reg);
//				}
//				else if (op == ADD || op == MUL ||
//					op == AND || op == OR ||
//					op == EQL || op == NEQ || op == SGT || op == SGE || op == SLT || op == SLE) {
//					if (!isNumber(ope2)) {	//不是立即数
//						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//					}
//					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//					regpool.releaseVreg(ope1);
//					regpool.releaseVreg(ope2);
//					resReg = allocTmpReg(regpool, res, funcTmp);	//如果寄存器不够就溢出一个
//					instr.setInstr(resReg, ope1Reg, ope2Reg);
//				}
//				else if (op == SUB || op == DIV || op == REM) {
//					if (!isNumber(ope2)) {	//不是立即数
//						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//					}
//					if (!isNumber(ope1)) {	//不是立即数
//						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//					}
//					regpool.releaseVreg(ope1);
//					regpool.releaseVreg(ope2);
//					resReg = allocTmpReg(regpool, res, funcTmp);	//如果寄存器不够就溢出一个
//					instr.setInstr(resReg, ope1Reg, ope2Reg);
//				}
//				else if (op == LEA) {
//					ope1Reg = allocTmpReg(regpool, ope1, funcTmp);
//					instr.setInstr(resReg, ope1Reg, ope2Reg);
//				}
//				else if (op == MOV) {	//分配临时寄存器
//					if (isVreg(ope2)) {
//						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//						regpool.releaseVreg(ope2);
//					}
//					if (isVreg(ope1)) {
//						ope1Reg = allocTmpReg(regpool, ope1, funcTmp);
//					}
//					instr.setInstr(resReg, ope1Reg, ope2Reg);
//				}
//				else if (op == LOAD) {	//数组地址的加载都放在临时寄存器，变量根据情况采用mov或者load
//					if (ope2 == "para") {	//加载参数数组的地址 
//						string regTmp = gRegpool.getReg(ope1);
//						if (regTmp != "memory") {
//							instr.setCodetype(MOV);
//							resReg = allocTmpReg(regpool, res, funcTmp);
//							instr.setInstr("", resReg, regTmp);
//						}
//						else {
//							resReg = allocTmpReg(regpool, res, funcTmp);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//					}
//					else if (ope2 == "array") {	//加载局部数组的地址
//						resReg = allocTmpReg(regpool, res, funcTmp);
//						instr.setCodetype(LEA);
//						instr.setInstr("", resReg, ope1Reg);
//					}
//					else {	//非数组地址加载
//						if (isVreg(ope1)) {	//全局变量
//							ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//							regpool.releaseVreg(ope1);
//							resReg = allocTmpReg(regpool, res, funcTmp);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//						else {	//栈变量
//							string regTmp = gRegpool.getReg(ope1);
//							if (regTmp == "memory") {			//变量没有被分配寄存器，使用临时寄存器存值
//								resReg = allocTmpReg(regpool, res, funcTmp);
//								instr.setInstr(resReg, ope1Reg, ope2Reg);
//							}
//							else {								//指派的全局寄存器
//								//局部变量不可能首先从栈里加载值
//								ope1Reg = regTmp;
//								vreg2varReg[res] = ope1Reg;		//vreg <-> varReg
//								instr.setCodetype(MOV);
//								instr.setInstr("", ope1Reg, ope1Reg);
//							}
//						}
//					}
//				}
//				else if (op == LOADARR) {
//					if (isNumber(ope2)) {	//偏移是立即数
//						if (isVreg(ope1)) {		//全局数组
//							ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//							regpool.releaseVreg(ope1);
//							resReg = allocTmpReg(regpool, res, funcTmp);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//						else {					//栈数组
//							resReg = allocTmpReg(regpool, res, funcTmp);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//					}
//					else {					//偏移是寄存器
//						if (isVreg(ope1)) {		//全局数组
//							ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//							ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//							regpool.releaseVreg(ope1);
//							regpool.releaseVreg(ope2);
//							resReg = allocTmpReg(regpool, res, funcTmp);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//						else {					//栈数组
//							ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//							regpool.releaseVreg(ope2);
//							resReg = allocTmpReg(regpool, res, funcTmp);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//					}
//				}
//				else if (op == STORE) {
//					if (isVreg(ope1)) {		//全局变量
//						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//						resReg = getTmpReg(regpool, res, funcTmp);
//						regpool.releaseVreg(ope1);
//						regpool.releaseVreg(res);
//						instr.setInstr(resReg, ope1Reg, ope2Reg);
//					}
//					else {	//栈变量
//						if (gRegpool.getReg(ope1) != "memory") {	//表明已经分配了寄存器
//							resReg = getTmpReg(regpool, res, funcTmp);
//							regpool.releaseVreg(res);
//							ope1Reg = gRegpool.getReg(ope1);		//将结果放在变量对应的寄存器
//							instr.setCodetype(MOV);
//							instr.setInstr("", ope1Reg, resReg);
//						}
//						else {
//							string regTmp = gRegpool.allocReg(ope1);	//尝试申请一个全局寄存器
//							if (regTmp == "memory") {	//申请失败
//								resReg = getTmpReg(regpool, res, funcTmp);
//								regpool.releaseVreg(res);
//								instr.setInstr(resReg, ope1Reg, ope2Reg);
//							}
//							else {	//第一次申请
//								resReg = getTmpReg(regpool, res, funcTmp);
//								regpool.releaseVreg(res);
//								ope1Reg = regTmp;					//将结果放在变量对应的寄存器
//								instr.setCodetype(MOV);
//								instr.setInstr("", ope1Reg, resReg);
//							}
//						}
//					}
//				}
//				else if (op == STOREARR) {
//					if (isNumber(ope2)) {	//偏移是立即数
//						if (isVreg(ope1)) {		//全局数组
//							ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//							resReg = getTmpReg(regpool, res, funcTmp);
//							regpool.releaseVreg(ope1);
//							regpool.releaseVreg(res);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//						else {					//栈数组
//							//因为是数组，分配的寄存器没有意义，不考虑 “memory”
//							resReg = getTmpReg(regpool, res, funcTmp);
//							regpool.releaseVreg(res);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//					}
//					else {					//偏移是寄存器
//						if (isVreg(ope1)) {		//全局数组
//							ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//							ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//							resReg = getTmpReg(regpool, res, funcTmp);
//							regpool.releaseVreg(ope1);
//							regpool.releaseVreg(ope2);
//							regpool.releaseVreg(res);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//						else {					//栈数组
//							//因为是数组，分配的寄存器没有意义，不考虑 “memory”
//							resReg = getTmpReg(regpool, res, funcTmp);
//							ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//							regpool.releaseVreg(ope2);
//							regpool.releaseVreg(res);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//					}
//				}
//				else if (op == POP) {	//貌似用不到
//					WARN_MSG("will use this Pop??");
//				}
//				else if (op == BR) {
//					if (isVreg(ope1)) {
//						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//						regpool.releaseVreg(ope1);
//					}
//					instr.setInstr(resReg, ope1Reg, ope2Reg);
//				}
//				else if (op == ALLOC) {
//					ope1Reg = var2reg[res];
//					instr.setInstr(resReg, ope1Reg, ope2Reg);
//				}
//				else if (op == PARA) {	//前四个参数立即分配寄存器，后面的在stack的用到的时候再分配
//					if (isReg(ope1)) {
//						string regTmp = gRegpool.getReg(res);
//						if (regTmp != "memory") {
//							ope1Reg = regTmp;
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//							funcTmp.push_back(CodeItem(MOV, "", ope1Reg, ope1));
//						}
//					}
//					else if (ope1 == "stack" && gRegpool.getReg(res) != "memory") {
//						funcTmp.push_back(CodeItem(LOAD, gRegpool.getReg(res), res, ""));
//					}
//				}
//				else if (op == GETREG) {
//					if (isVreg(ope1)) {
//						ope1Reg = allocTmpReg(regpool, ope1, funcTmp);
//					}
//					instr.setInstr(resReg, ope1Reg, ope2Reg);
//				}
//				else if (op == NOTE && ope1 == "func" && ope2 == "begin") {
//					//记录函数的参数寄存器的情况	
//					callLayer++;
//					callRegs.push_back(map<string, string>());
//					paraReg2push.push_back(map<string, bool>());
//					//表达式计算的中间变量的保存！
//					auto save = regpool.spillAllRegs();
//					funcTmp.push_back(instr);
//					for (auto one : save) {		//函数调用的时候临时寄存器的值需要保存
//						setVrIndex(one.first);
//						//如果溢出的寄存器是push参数寄存器
//						if (callLayer > 0) {
//							if (callRegs.at(callLayer - 1).find(one.second) != callRegs.at(callLayer - 1).end()) {
//								paraReg2push.at(callLayer - 1)[one.second] = true;
//							}
//						}
//						CodeItem pushTmp(STORE, one.second, one.first, "");
//						funcTmp.push_back(pushTmp);
//					}
//					continue;
//				}
//				else if (op == NOTE && ope1 == "func" && ope2 == "end") {
//					//nothing
//				}
//				else if (op == CALL) {
//					//如果函数嵌套导致r0-r3被spill，在函数调用前重取r0-r3
//					if (callLayer >= 0) {
//						//如果是中间变量把参数寄存器溢出了，怎么标记
//						for (auto para : paraReg2push.at(callLayer)) {
//							//??????
//							if (para.second == true) {		//如果参数寄存器被溢出了，需要load回来
//								string vRegTmp = callRegs.at(callLayer)[para.first];
//								CodeItem loadTmp(LOAD, para.first, vRegTmp, "para");
//								funcTmp.push_back(loadTmp);
//							}
//						}
//					}
//					for (auto para : callRegs.at(callLayer)) {		//释放参数寄存器 R0-R3
//						regpool.releaseVreg(para.second);
//					}
//					callLayer--;			//函数层级降低
//					callRegs.pop_back();	//函数调用寄存器删除
//					paraReg2push.pop_back();
//				}
//				else if (op == PUSH) {	//参数压栈 4是分界点
//					if (A2I(ope2) <= 4) {	//前4个此参数
//						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//						regpool.releaseVreg(ope1);
//						instr.setInstr(resReg, ope1Reg, ope2Reg);
//
//						int num = A2I(ope2);
//						string paraReg = FORMAT("R{}", num - 1);
//						string curVreg = FORMAT("VR{}", vrNumber++);	//这个虚拟寄存器不会在其他地方出现，除了load
//						regpool.markParaReg(paraReg, curVreg);		//标记参数寄存器被占用
//						callRegs.at(callLayer)[paraReg] = curVreg;	//push一个意味着这个参数寄存器被占用了
//						//后续可能需要压栈保存值！
//					}
//					else {					//栈参数
//						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//						regpool.releaseVreg(ope1);
//						instr.setInstr(resReg, ope1Reg, ope2Reg);
//					}
//				}
//				else {
//					//do nothing!
//				}
//				funcTmp.push_back(instr);
//			}	//每个ir的结尾
//			gRegpool.noteRegRelations(funcTmp);
//			//进行不活跃变量的写回，然后释放寄存器
//			//if (!retFlag) {
//			//	if (brFlag) {
//			//		gRegpool.releaseNorActRegs(func.at(i).in, func.at(i).out, funcTmp, -1);	//添加指令的位置在倒数第二个
//			//	}
//			//	else {
//			//		gRegpool.releaseNorActRegs(func.at(i).in, func.at(i).out, funcTmp, 0);
//			//	}
//			//}
//			/*set<string> inTmp;
//			if (i+1 < func.size()) {
//				inTmp = func.at(i + 1).in;
//			}
//			gRegpool.releaseNorActRegs(inTmp, func.at(i).out);*/
//		}
//		LIRTmp.push_back(funcTmp);
//		func2Vr.push_back(vr2index);
//		
//		auto usedRegs = gRegpool.getUsedRegs();
//		sort(usedRegs.begin(), usedRegs.end());
//		vector<string> useRegsTmp;
//		for (auto one : usedRegs) {
//			useRegsTmp.push_back(FORMAT("R{}", one));
//		}
//		func2gReg.push_back(useRegsTmp);
//	}
//	LIR = LIRTmp;
//}

//======================================================================
// 寄存器分配3 —— 图着色分配
//=======================================================================

void registerAllocation3(vector<map<string, string>>& var2gReg) {
	vector<vector<CodeItem>> LIRTmp;
	func2gReg.push_back(vector<string>());
	func2Vr.push_back(map<string, int>());
	LIRTmp.push_back(LIR.at(0));

	for (int k = 1; k < LIR.size(); k++) {
		auto func = LIR.at(k);
		auto vars = stackVars.at(k);
		auto arr = var2Arr.at(k);
		auto regAlloc = var2gReg.at(k);
		set<string> leftGregs = { "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11"};
		for (auto one : regAlloc) {
			if (leftGregs.find(one.second) != leftGregs.end()) {
				leftGregs.erase(one.second);
			}
		}
		vector<CodeItem> funcTmp;

		//初始化

		// R12作为临时寄存器
		vector<string> tmpRegs = { "R0", "R1", "R2", "R3", "R12"};	//临时寄存器池
		
		// R12不作为临时寄存器
		//vector<string> tmpRegs = { "R0", "R1", "R2", "R3" };	//临时寄存器池
		
		RegPool regpool(tmpRegs);
		vreg2varReg.clear();
		first.clear();
		var2reg.clear();
		regBegin = 4;
		vr2index.clear();
		vrNum = 0;
		int paraNum = 0;

		//多层函数调用参数寄存器保存工作
		int vrNumber = func2vrIndex.at(k);
		callLayer = -1;
		callRegs.clear();		//存储 r[0-3] <-> VR[0-9]
		paraReg2push.clear();	//参数寄存器被压栈保存的（其实是str保存）

		//根据图着色信息分配寄存器
		for (int i = 0; i < vars.size(); i++) {
			if (regAlloc.find(vars.at(i)) != regAlloc.end()) {
				var2reg[vars.at(i)] = regAlloc[vars.at(i)];
				first[vars.at(i)] = true;
			}
			else {
				if (!leftGregs.empty() && arr[vars.at(i)] == true) {	//在alloc的时候把地址取出来
					var2reg[vars.at(i)] = *leftGregs.begin();
					leftGregs.erase(leftGregs.begin());
					first[vars.at(i)] = true;
				}
				else {
					var2reg[vars.at(i)] = "memory";
					first[vars.at(i)] = true;
				}
			}
		}
		//全局变量使用临时变量来加载和使用

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
			}
			else if (op == NOT) {
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseVreg(ope1);
				if (isVreg(res)) {
					resReg = allocTmpReg(regpool, res, funcTmp);	//如果寄存器不够就溢出一个
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == ADD || op == MUL) {	//先申请两个后释放
				if (!isNumber(ope2)) {	//不是立即数
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
				}
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseVreg(ope1);
				regpool.releaseVreg(ope2);
				resReg = allocTmpReg(regpool, res, funcTmp);	//如果寄存器不够就溢出一个
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == AND || op == OR ||
				op == EQL || op == NEQ || op == SGT || op == SGE || op == SLT || op == SLE) {
				if (!isNumber(ope2)) {	//不是立即数
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
				}
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseVreg(ope1);
				regpool.releaseVreg(ope2);
				if (isVreg(res)) {
					resReg = allocTmpReg(regpool, res, funcTmp);	//如果寄存器不够就溢出一个
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == SUB || op == DIV || op == REM) {	//先申请两个后释放
				if (!isNumber(ope2)) {	//不是立即数
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
				}
				if (!isNumber(ope1)) {	//不是立即数
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				}
				regpool.releaseVreg(ope1);
				regpool.releaseVreg(ope2);
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
					regpool.releaseVreg(ope2);
				}
				if (isVreg(ope1)) {
					ope1Reg = allocTmpReg(regpool, ope1, funcTmp);
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == LOAD) {
				if (ope2 == "para") {	//加载数组参数的地址
					if (var2reg[ope1] != "memory") {	//有寄存器
						instr.setCodetype(MOV);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr("", resReg, var2reg[ope1]);
					}
					else {
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
				else if (ope2 == "array") {	//加载局部数组的地址
					if (var2reg[ope1] != "memory") {	//有寄存器
						resReg = var2reg[ope1];
						WARN_MSG("array have no reg! wrong!");	//理论上不会出现这个分支
					}
					else {
						resReg = allocTmpReg(regpool, res, funcTmp);
					}
					instr.setCodetype(LEA);
					instr.setInstr("", resReg, ope1Reg);
				}
				else {	//非数组地址加载
					if (isVreg(ope1)) {		//全局变量
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						regpool.releaseVreg(ope1);
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
							else {//在栈里的参数如果没有寄存器，可能需要load，或者有寄存器，但是存在栈里的，需要第一次加载出来
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
						regpool.releaseVreg(ope1);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//栈数组
						resReg = allocTmpReg(regpool, res, funcTmp);
						if (var2reg[ope1] != "memory") {	//数组基地址有寄存器
							instr.setInstr(resReg, var2reg[ope1], ope2Reg);
						}
						else {

							instr.setInstr(resReg, ope1Reg, ope2Reg);
						}
					}
				}
				else {					//偏移是寄存器
					if (isVreg(ope1)) {		//全局数组
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						regpool.releaseVreg(ope1);
						regpool.releaseVreg(ope2);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//栈数组
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						regpool.releaseVreg(ope2);
						resReg = allocTmpReg(regpool, res, funcTmp);
						if (var2reg[ope1] != "memory") {	//数组基地址有寄存器
							instr.setInstr(resReg, var2reg[ope1], ope2Reg);
						}
						else {

							instr.setInstr(resReg, ope1Reg, ope2Reg);
						}
					}
				}
			}
			else if (op == STORE) {
				if (isVreg(ope1)) {		//全局变量
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
					resReg = getTmpReg(regpool, res, funcTmp);
					regpool.releaseVreg(ope1);
					regpool.releaseVreg(res);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
				}
				else {					//栈变量
					if (var2reg[ope1] == "memory") {
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseVreg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseVreg(res);
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
						regpool.releaseVreg(ope1);
						regpool.releaseVreg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//栈数组
						//因为是数组，分配的寄存器没有意义，不考虑 “memory”
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseVreg(res);
						if (var2reg[ope1] != "memory") {	//数组基地址有寄存器
							instr.setInstr(resReg, var2reg[ope1], ope2Reg);
						}
						else {

							instr.setInstr(resReg, ope1Reg, ope2Reg);
						}
					}
				}
				else {					//偏移是寄存器
					if (isVreg(ope1)) {		//全局数组
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseVreg(ope1);
						regpool.releaseVreg(ope2);
						regpool.releaseVreg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//栈数组
						//因为是数组，分配的寄存器没有意义，不考虑 “memory”
						resReg = getTmpReg(regpool, res, funcTmp);
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						regpool.releaseVreg(ope2);
						regpool.releaseVreg(res);
						if (var2reg[ope1] != "memory") {	//数组基地址有寄存器
							instr.setInstr(resReg, var2reg[ope1], ope2Reg);
						}
						else {
							
							instr.setInstr(resReg, ope1Reg, ope2Reg);
						}
					}
				}
			}
			else if (op == POP) {	//貌似用不到
				WARN_MSG("will use this Pop??");
			}
			else if (op == BR) {
				if (isVreg(ope1)) {
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
					regpool.releaseVreg(ope1);
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == ALLOC) {
				ope1Reg = var2reg[res];
				instr.setInstr(resReg, ope1Reg, ope2Reg);
				if (arr[res] == true && var2reg[res] != "memory") {	//局部数组且有寄存器
					funcTmp.push_back(CodeItem(LEA, "", var2reg[res], res));	//取局部数组的地址
					first[res] = false;
				}
			}
			else if (op == PARA) {
				ope1Reg = var2reg[res];
				instr.setInstr(resReg, ope1Reg, ope2Reg);
				paraNum++;
				//图着色如果分配了寄存器就mov，如果没有并且使前四个参数，需要store到栈
				if (paraNum <= 4) {
					if (var2reg[res] != "memory") {	//分配了寄存器，直接MOV
						funcTmp.push_back(CodeItem(MOV, "", ope1Reg, ope1));
						first[res] = false;
					}
					else {
						funcTmp.push_back(CodeItem(STORE, ope1, res, ""));
					}
				}
				if (ope1 == "stack" && var2reg[res] != "memory") {
					funcTmp.push_back(CodeItem(LOAD, var2reg[res], res, "para"));
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
				//记录函数的参数寄存器的情况	
				callLayer++;
				callRegs.push_back(map<string, string>());
				paraReg2push.push_back(map<string, bool>());
				//表达式计算的中间变量的保存！
				auto save = regpool.spillAllRegs();
				funcTmp.push_back(instr);
				for (auto one : save) {		//函数调用的时候临时寄存器的值需要保存
					setVrIndex(one.first);
					//如果溢出的寄存器是push参数寄存器
					if (callLayer > 0) {
						if (callRegs.at(callLayer - 1).find(one.second) != callRegs.at(callLayer - 1).end()) {
							paraReg2push.at(callLayer - 1)[one.second] = true;
						}
					}
					CodeItem pushTmp(STORE, one.second, one.first, "");
					funcTmp.push_back(pushTmp);
				}
				continue;
			}
			else if (op == NOTE && ope1 == "func" && ope2 == "end") {
				//nothing
			}
			else if (op == CALL) {
				//如果函数嵌套导致r0-r3被spill，在函数调用前重取r0-r3
				if (callLayer >= 0) {
					//如果是中间变量把参数寄存器溢出了，怎么标记
					for (auto para : paraReg2push.at(callLayer)) {
						//??????
						if (para.second == true) {		//如果参数寄存器被溢出了，需要load回来
							string vRegTmp = callRegs.at(callLayer)[para.first];
							CodeItem loadTmp(LOAD, para.first, vRegTmp, "para");
							funcTmp.push_back(loadTmp);
						}
					}
				}
				for (auto para : callRegs.at(callLayer)) {		//释放参数寄存器 R0-R3
					regpool.releaseVreg(para.second);
				}
				callLayer--;			//函数层级降低
				callRegs.pop_back();	//函数调用寄存器删除
				paraReg2push.pop_back();
				//int paraNum = A2I(ope2);
				//for (int i = 0; i < paraNum; i++) {
				//	regpool.releaseReg(FORMAT("R{}", i));		//释放参数寄存器 R0-R3
				//}
			}
			else if (op == PUSH) {	//参数压栈 4是分界点
				if (A2I(ope2) <= 4) {	//前4个此参数
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
					regpool.releaseVreg(ope1);
					instr.setInstr(resReg, ope1Reg, ope2Reg);

					int num = A2I(ope2);
					string paraReg = FORMAT("R{}", num - 1);
					//push参数后标记对应的参数寄存器r0-3为占用状态！
					//这里r0还可能被占用吗？？？
					//理论上不存在，因为中间变量不会夸参数出现，要是被使用了，也一定被释放了
					string curVreg = FORMAT("VR{}", vrNumber++);	//这个虚拟寄存器不会在其他地方出现，除了load
					regpool.markParaReg(paraReg, curVreg);		//标记参数寄存器被占用
					callRegs.at(callLayer)[paraReg] = curVreg;	//push一个意味着这个参数寄存器被占用了
					//后续可能需要压栈保存值！
				}
				else {					//栈参数
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
					regpool.releaseVreg(ope1);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
				}
			}
			else {
				//do nothing!
			}
			funcTmp.push_back(instr);
		}
		LIRTmp.push_back(funcTmp);
		func2Vr.push_back(vr2index);
		
		set<int> useRegs;
		for (auto one : var2reg) {
			if (one.second != "memory") {
				useRegs.insert(A2I(one.second.substr(1)));
			}
		}
		vector<int> tmp(useRegs.begin(), useRegs.end());
		sort(tmp.begin(), tmp.end());
		vector<string> useRegsTmp;
		for (auto one : useRegs) {
			useRegsTmp.push_back(FORMAT("R{}", one));
		}
		func2gReg.push_back(useRegsTmp);
	}
	LIR = LIRTmp;
}
