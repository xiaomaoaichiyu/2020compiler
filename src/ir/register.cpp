#include "./register.h"

//=================================================================
//1. �Ĵ���ֱ��ָ�ɣ���ʱ�Ĵ���ʹ�üĴ�����
//=================================================================

string RegPool::getReg(string vreg) {
	if (vreg2reg.find(vreg) != vreg2reg.end()) {
		return vreg2reg[vreg];	//���ط���ļĴ���
	}
	else {	//���û�б�����Ĵ����ͷ�������Ĵ���
		return vreg;
	} 
}

string RegPool::allocReg(string vreg) {
	string tmpReg = haveAvailReg();
	if (tmpReg != "No Reg!") {
		vreg2reg[vreg] = tmpReg;	
		vreguse.push_back(vreg);
		reg2avail[tmpReg] = false;	//���Ϊ��ռ��
	}
	else {
		return vreg;
	}
}

void RegPool::releaseReg(string vreg) {
	string useReg = vreg2reg[vreg];
	vreg2reg.erase(vreg);
	reg2avail[useReg] = true;
	for (auto it = vreguse.begin(); it != vreguse.end(); it++) {
		if (*it == vreg) {
			vreguse.erase(it);
		}
	}
}

//private
string RegPool::haveAvailReg() {
	for (int i = 0; i < pool.size(); i++) {
		if (reg2avail[pool.at(i)] == true) return pool.at(i);
	}
	return "No reg!";
}

void setInstr(CodeItem& instr, string res, string ope1, string ope2) {
	instr.setResult(res);
	instr.setOperand1(ope1);
	instr.setOperand2(ope2);
}

//�Ĵ��������㷨v1
//: �ӱ���ָ�ɣ������������load�������ݵ���ʱ�Ĵ���
void registerAllocation(vector<CodeItem>& func) {
	vector<string> tmpRegs = { "R0", "R1", "R2", "R3", "R12" };	//��ʱ�Ĵ�����
	RegPool regpool(tmpRegs);
	
	map<string, bool> first;		//��־�мĴ��������Ƿ��ǵ�һ�η���
	vector<string> vars;
	map<string, string> var2reg;
	int begin = 4;
	//����ָ�ɾֲ�����
	int i = 0;
	for (; i < vars.size(); i++) {
		var2reg[vars.at(i)] = FORMAT("R{}", begin++);
		first[vars.at(i)] = true;
		if (begin >= 12) {
			i++;
			break;
		}
	}
	for (; i < vars.size(); i++) {
		var2reg[vars.at(i)] = "memory";
		first[vars.at(i)] = true;
	}
	//ȫ�ֱ�����ô�ƣ����Լ��ص���ʱ�����У�

	for (int i = 0; i < func.size(); i++) {
		CodeItem& instr = func.at(i);
		irCodeType op = instr.getCodetype();
		string res = instr.getResult();
		string ope1 = instr.getOperand1();
		string ope2 = instr.getOperand2();
		string resReg = res;
		string ope1Reg = ope1Reg;
		string ope2Reg = ope2Reg;
		
		//res�ֶη�����ʱ�Ĵ���
		if (op == ADD || op == SUB || op == DIV || op == MUL || op == REM ||
			op == AND || op == OR || op ==NOT ||
			op == EQL || op == NEQ || op == SGT || op == SGE || op ==SLT || op == SLE) {
			if (isTmp(ope1)) {
				ope1Reg = regpool.getReg(ope1);
				regpool.releaseReg(ope1);
			}
			if (isTmp(ope2)) {
				ope2Reg = regpool.getReg(ope2);
				regpool.releaseReg(ope2);
			}
			resReg = regpool.allocReg(res);
			setInstr(instr, resReg, ope1Reg, ope2Reg);
		}
		else if (op == MOV) {	//������ʱ�Ĵ���
			if (isTmp(ope2)) {
				ope2Reg = regpool.getReg(ope2);
				regpool.releaseReg(ope2);
			}
			ope1Reg = regpool.allocReg(ope1);
			setInstr(instr, res, ope1Reg, ope2Reg);
		}
		else if (op == LOAD) {	//������ʱ�Ĵ���
			if (var2reg[ope1] == "memory") {
				resReg = regpool.allocReg(res);
				setInstr(instr, resReg, ope1, ope2);
			}
			else {
				resReg = var2reg[ope1];
				if (first[ope1] == false) {
					instr.setCodetype(MOV);
					setInstr(instr, "", resReg, resReg);
				}
				else {
					first[ope1] = false;
					setInstr(instr, resReg, ope1, ope2);
				}
			}
		}
		else if (op == LOADARR) {	//������ʱ�Ĵ���
			
		}
		else if (op == STORE) {
			if (var2reg[ope1] == "memory") {
				resReg = regpool.getReg(res);
				regpool.releaseReg(res);
				setInstr(instr, resReg, ope1, ope2);
			}
			else {
				resReg = regpool.getReg(res);
				regpool.releaseReg(res);
				ope1Reg = var2reg[ope1];
				first[ope1] = false;
				instr.setCodetype(MOV);
				setInstr(instr, "", ope1Reg, resReg);
			}
		}
		else if (op == STOREARR) {
			resReg = regpool.getReg(res);
			ope2Reg = regpool.getReg(ope2);
			setInstr(instr, resReg, ope1, ope2Reg);
		}
		else if (op == CALL) {
			if (res != "void") {
				resReg = regpool.allocReg(res);
			}
			setInstr(instr, resReg, ope1, ope2);
		}
		else if (op == RET) {
			if (isTmp(ope1)) {
				ope1Reg = regpool.getReg(ope1);
				regpool.releaseReg(ope1);
			}
			setInstr(instr, res, ope1Reg, ope2);
		}
		else if (op == PUSH) {
			ope1Reg = regpool.getReg(ope1);
			setInstr(instr, res, ope1Reg, ope2);
		}
		else if (op == POP) {	//ò���ò���
			ope1Reg = regpool.getReg(ope1);
			setInstr(instr, res, ope1Reg, ope2);
			WARN_MSG("will use this Pop??");
		}
		else if (op == BR) {
			if (isTmp(ope1)) {
				ope1Reg = regpool.getReg(ope1);
			}
			setInstr(instr, res, ope1Reg, ope2);
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

