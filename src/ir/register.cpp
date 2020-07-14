#include "./register.h"

//=================================================================
// �Ĵ�����
//=================================================================

string RegPool::getReg(string vreg) {
	if (vreg2reg.find(vreg) != vreg2reg.end()) {
		return vreg2reg[vreg];	//���ط���ļĴ���
	}
	else if (vreg2spill.find(vreg) != vreg2spill.end()){	//���û�б�����Ĵ����ͷ�������Ĵ���
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
	reg2avail[tmpReg] = false;	// ���Ϊ��ռ��
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
	string vReg = vreguse.front();		//spill������Ĵ���vReg
	string spillReg = vreg2reg[vReg];
	reg2avail[spillReg] = true;			//��Ӧ������Ĵ������Ϊ����
	vreg2Offset[vReg] = stackOffset;	//vReg��ջ��ƫ��
	stackOffset += 4;
	releaseReg(vReg);					//�ͷ�vReg������Ĵ���
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
// �Ĵ��������㷨v1
// �ӱ���ָ�ɣ������������load�������ݵ���ʱ�Ĵ���
//===================================================================

map<string, string> vreg2varReg;	//��¼����Ĵ����ͱ���֮��Ĺ�ϵ

map<string, bool> first;			//��־�мĴ��������Ƿ��ǵ�һ�η���
map<string, string> var2reg;
int regBegin = 4;

//����һ������Ĵ����Ƿ�ͱ����ļĴ�����Ӧ
bool isFind(string vreg, map<string, string> container) {
	if (container.find(vreg) != container.end()) {
		return true;
	}
	return false;
}

string allocTmpReg(RegPool& regpool, std::string res, std::vector<CodeItem>& func, int& i)
{
	string reg = regpool.getAndAllocReg(res);
	if (reg == res) {	//û����ʱ�Ĵ�����
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
	regpool.releaseReg(res);			//��ʱ����ֻ����һ��
	return reg;
}

void registerAllocation(vector<CodeItem>& func, vector<string> vars) {
	vector<string> tmpRegs = { "R0", "R1", "R2", "R3", "R12" };	//��ʱ�Ĵ�����
	RegPool regpool(tmpRegs);

	vreg2varReg.clear();
	first.clear();
	var2reg.clear();
	regBegin = 4;
	//����ָ�ɾֲ�����
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
	//ȫ�ֱ���ʹ����ʱ���������غ�ʹ��

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
		
		//res�ֶη�����ʱ�Ĵ���
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
			if (!isNumber(ope2)) {	//����������
				if (isFind(ope2, vreg2varReg)) {	//vreg�Ǳ����Ĵ�����ӳ��
					ope2Reg = vreg2varReg[ope2];
				}
				else {	//֮ǰ�������ʱ�Ĵ���
					ope2Reg = getTmpReg(regpool, ope2, func, i);
				}
			}
			if (isFind(ope1, vreg2varReg)) {
				ope1Reg = vreg2varReg[ope1];
			}
			else {
				ope1Reg = getTmpReg(regpool, ope1, func, i);
			}
			resReg = allocTmpReg(regpool, res, func, i);	//����Ĵ������������һ��
			instr.setInstr(resReg, ope1Reg, ope2Reg);
		}
		else if (op == LEA) {
			ope1Reg = allocTmpReg(regpool, ope1, func, i);
			instr.setInstr(resReg, ope1Reg, ope2Reg);
		}
		else if (op == MOV) {	//������ʱ�Ĵ���
			if (isVreg(ope2)) {
				ope2Reg = getTmpReg(regpool, ope2, func, i);
			}
			if (isVreg(ope1)) {
				ope1Reg = allocTmpReg(regpool, ope1, func, i);
			}
			instr.setInstr(resReg, ope1Reg, ope2Reg);
		}
		else if (op == LOAD) {
			if (isVreg(ope1)){		//ȫ�ֱ���
				ope1Reg = getTmpReg(regpool, ope1, func, i);
				resReg = allocTmpReg(regpool, ope1, func, i);
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else {					//ջ����
				if (var2reg[ope1] == "memory") {	//������ʱ�Ĵ���
					resReg = allocTmpReg(regpool, res, func, i);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
				}
				else {								//ָ�ɵ�ȫ�ּĴ���
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
			if (isNumber(ope2)) {	//ƫ����������
				if (isVreg(ope1)) {		//ȫ������
					ope1Reg = getTmpReg(regpool, ope1, func, i);
					resReg = allocTmpReg(regpool, res, func, i);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
				}
				else {					//ջ����
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
			else {					//ƫ���ǼĴ���
				if (isVreg(ope1)) {		//ȫ������
					ope1Reg = getTmpReg(regpool, ope1, func, i);
					ope2Reg = getTmpReg(regpool, ope2, func, i);
					resReg = allocTmpReg(regpool, res, func, i);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
				}
				else {					//ջ����
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
			if (isVreg(ope1)) {		//ȫ�ֱ���
				ope1Reg = getTmpReg(regpool, ope1, func, i);
				resReg = getTmpReg(regpool, res, func, i);
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}	
			else {					//ջ����
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
		else if (op == POP) {	//ò���ò���
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
//����ɨ��Ĵ�������
//=====================================================================================
//
//BlockOrder::BlockOrder(vector<basicBlock> blks) {
//	int i = 0;
//	for (; i < blks.size(); i++) {
//		int blknumber = blks[i].number;
//		blocks[blknumber] = blks[i];
//		blk2index[blknumber] = -1;	//�������index��ʼ��Ϊ-1
//		blk2depth[blknumber] = 0;	//�������depth��ʼ��Ϊ0
//	}
//}
//
////ѭ����⣬��ÿһ�����������index��depth
//void BlockOrder::loop_detection(int nblks) {
//	queue<int> work;
//	map<int, int> active;							//����Ƿ��ʹ�
//	map<int, bool> visited;							//�Ƿ񱻷��ʹ�
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
//		//���Ϊvisited
//		visited.at(init) = true;
//		//������
//		set<int> succedds = blocks[init].succeeds;
//		for (auto i : succedds) {
//			//�ж��Ƿ���ѭ��
//			if (active[i] != 0) {
//				loop_end_lists.push_back(pair<int, int>(init, i));
//				loop_header_index.push_back(pair<int, int>(i, index++));
//			}
//			//û���ʹ��ͼ������
//			if (visited.at(i) == false) {
//				work.push(i);
//			}
//		}
//		//����ǰ׺
//		set<int> preds = blocks[init].pred;
//		for (auto i : preds) {
//			active[i]--;
//		}
//	}
//
//	//��loop_end��ʼ�������ʵ��loop_depth
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

