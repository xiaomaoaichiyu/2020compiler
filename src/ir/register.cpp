#include "./register.h"
#include "./optimize.h"

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
	reg2avail[tmpReg] = false;	// ���Ϊ��ռ��
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
	string vReg = vreguse.front();		//spill������Ĵ���vReg
	string spillReg = vreg2reg[vReg];
	reg2avail[spillReg] = true;			//��Ӧ������Ĵ������Ϊ����
	vreg2spill[vReg] = true;			//��Ƕ�Ӧ��VRΪ���״̬
	releaseVreg(vReg);					//�ͷ�vReg������Ĵ���
	return pair<string, string>(vReg, spillReg);
}

//����ǰȫ������ʱ�Ĵ������������callǰ���ñ�����������õ����м�ֵ
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

//�м�����Ĵ洢----------------------------------------
map<string, int> vr2index;
int vrNum = 0;

void setVrIndex(string vr) {
	if (vr2index.find(vr) != vr2index.end()) {
		return;
	}
	else {
		//����ǰpush��ջ���м����(VR)��һ��λ��
		vr2index[vr] = vrNum++;
	}
}
//�м�����Ĵ洢----------------------------------------

int callLayer = -1;
vector<map<string, string>> callRegs;		//�洢 r[0-3] <-> VR[0-9]
vector<map<string, bool>>   paraReg2push;	//�����Ĵ�����ѹջ����ģ���ʵ��str���棩


//��ʱ�Ĵ�������
string allocTmpReg(RegPool& regpool, std::string res, std::vector<CodeItem>& func)
{
	string reg = regpool.getAndAllocReg(res);
	if (reg == res) {	//û����ʱ�Ĵ�����
		pair<string, string> tmp = regpool.spillReg();	//����˼Ĵ���
		setVrIndex(tmp.first);
		//����ļĴ����ǲ����Ĵ����ͱ��Ϊtrue;
		if (callLayer >= 0 && callRegs.at(callLayer).find(tmp.second) != callRegs.at(callLayer).end()) {
			paraReg2push.at(callLayer)[tmp.second] = true;
		}
		CodeItem pushInstr(STORE, tmp.second, tmp.first, "");
		func.push_back(pushInstr);
		reg = regpool.getAndAllocReg(res);
	}
	return reg;
}

//��ȡVR��Ӧ����ʱ�Ĵ���
string getTmpReg(RegPool& regpool, std::string res, std::vector<CodeItem>& func) {
	if (vreg2varReg.find(res) != vreg2varReg.end()) {
		return vreg2varReg[res];
	}
	string reg = regpool.getReg(res);
	if (reg == "spilled") {		//��������ļĴ���
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

		//��ʼ��
		vector<string> tmpRegs = { "R0", "R1", "R2", "R3", "R12" };	//��ʱ�Ĵ�����
		RegPool regpool(tmpRegs);
		vreg2varReg.clear();
		first.clear();
		var2reg.clear();
		regBegin = 4;
		vr2index.clear();
		vrNum = 0;

		//��㺯�����ò����Ĵ������湤��
		int vrNumber = func2vrIndex.at(k);
		callLayer = -1;
		callRegs.clear();		//�洢 r[0-3] <-> VR[0-9]
		paraReg2push.clear();	//�����Ĵ�����ѹջ����ģ���ʵ��str���棩
		
		//����ط������Ż���
		//1. �ֲ�����ʵ���ϲ�����Ҫ�Ĵ���
		//2. ��Ծ����������̬�ͷŲ���ʹ�õı���ռ�õļĴ���
								
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

			//res�ֶη�����ʱ�Ĵ���
			if (op == DEFINE) {
				funcName = res;
			}else if (op == NOT) {
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseVreg(ope1);
				resReg = allocTmpReg(regpool, res, funcTmp);
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == ADD || op == MUL ||
					 op == AND || op == OR ||
					 op == EQL || op == NEQ || op == SGT || op == SGE || op == SLT || op == SLE) {
				if (!isNumber(ope2)) {	//����������
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
					regpool.releaseVreg(ope2);
				}
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseVreg(ope1);
				resReg = allocTmpReg(regpool, res, funcTmp);	//����Ĵ������������һ��
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == SUB || op == DIV || op == REM) {
				if (!isNumber(ope2)) {	//����������
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
					regpool.releaseVreg(ope2);
				}
				if (!isNumber(ope1)) {	//����������
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
					regpool.releaseVreg(ope1);
				}
				resReg = allocTmpReg(regpool, res, funcTmp);	//����Ĵ������������һ��
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == LEA) {
				ope1Reg = allocTmpReg(regpool, ope1, funcTmp);
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == MOV) {	//������ʱ�Ĵ���
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
				if (ope2 == "para") {	//������������ĵ�ַ
					if (var2reg[ope1] != "memory") {	//�мĴ���
						instr.setCodetype(MOV);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr("", resReg, var2reg[ope1]);
					}
					else {
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
				else if (ope2 == "array") {	//���ؾֲ�����ĵ�ַ
					if (var2reg[ope1] != "memory") {	//�мĴ���
						resReg = var2reg[ope1];
					}
					else {
						resReg = allocTmpReg(regpool, res, funcTmp);
					}
					instr.setCodetype(LEA);
					instr.setInstr("", resReg, ope1Reg);
				}
				else {	//�������ַ����
					if (isVreg(ope1)) {		//ȫ�ֱ���
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						regpool.releaseVreg(ope1);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//ջ����
						if (var2reg[ope1] == "memory") {	//������ʱ�Ĵ���
							resReg = allocTmpReg(regpool, res, funcTmp);
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
			}
			else if (op == LOADARR) {
				if (isNumber(ope2)) {	//ƫ����������
					if (isVreg(ope1)) {		//ȫ������
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						regpool.releaseVreg(ope1);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//ջ����
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
				else {					//ƫ���ǼĴ���
					if (isVreg(ope1)) {		//ȫ������
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						regpool.releaseVreg(ope1);
						regpool.releaseVreg(ope2);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//ջ����
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						regpool.releaseVreg(ope2);
						resReg = allocTmpReg(regpool, res, funcTmp);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
			}
			else if (op == STORE) {
				if (isVreg(ope1)) {		//ȫ�ֱ���
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
					resReg = getTmpReg(regpool, res, funcTmp);
					regpool.releaseVreg(ope1);
					regpool.releaseVreg(res);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
				}
				else {					//ջ����
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
				if (isNumber(ope2)) {	//ƫ����������
					if (isVreg(ope1)) {		//ȫ������
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseVreg(ope1);
						regpool.releaseVreg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//ջ����
						//��Ϊ�����飬����ļĴ���û�����壬������ ��memory��
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseVreg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
				else {					//ƫ���ǼĴ���
					if (isVreg(ope1)) {		//ȫ������
						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						resReg = getTmpReg(regpool, res, funcTmp);
						regpool.releaseVreg(ope1);
						regpool.releaseVreg(ope2);
						regpool.releaseVreg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
					else {					//ջ����
						//��Ϊ�����飬����ļĴ���û�����壬������ ��memory��
						resReg = getTmpReg(regpool, res, funcTmp);
						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
						regpool.releaseVreg(ope2);
						regpool.releaseVreg(res);
						instr.setInstr(resReg, ope1Reg, ope2Reg);
					}
				}
			}
			else if (op == POP) {	//ò���ò���
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

#define isReg(x) ((x.size() > 1) && (x.at(0) == 'R'))

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
				//��¼�����Ĳ����Ĵ��������	
				callLayer++;
				callRegs.push_back(map<string, string>());
				paraReg2push.push_back(map<string, bool>());
				//���ʽ������м�����ı��棡
				auto save = regpool.spillAllRegs();
				funcTmp.push_back(instr);
				for (auto one : save) {		//�������õ�ʱ����ʱ�Ĵ�����ֵ��Ҫ����
					setVrIndex(one.first);
					//�������ļĴ�����push�����Ĵ���
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
				//�������Ƕ�׵���r0-r3��spill���ں�������ǰ��ȡr0-r3
				if (callLayer >= 0) {
					//������м�����Ѳ����Ĵ�������ˣ���ô���
					for (auto para : paraReg2push.at(callLayer)) {
						//??????
						if (para.second == true) {		//��������Ĵ���������ˣ���Ҫload����
							string vRegTmp = callRegs.at(callLayer)[para.first];
							CodeItem loadTmp(LOAD, para.first, vRegTmp, "para");
							funcTmp.push_back(loadTmp);
						}
					}
				}
				for (auto para : callRegs.at(callLayer)) {		//�ͷŲ����Ĵ��� R0-R3
					regpool.releaseVreg(para.second);
				}
				callLayer--;			//�����㼶����
				callRegs.pop_back();	//�������üĴ���ɾ��
				paraReg2push.pop_back();
				//int paraNum = A2I(ope2);
				//for (int i = 0; i < paraNum; i++) {
				//	regpool.releaseReg(FORMAT("R{}", i));		//�ͷŲ����Ĵ��� R0-R3
				//}
			}
			else if (op == PUSH) {	//����ѹջ 4�Ƿֽ��
				if (A2I(ope2) <= 4) {	//ǰ4���˲���
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
					regpool.releaseVreg(ope1);
					instr.setInstr(resReg, ope1Reg, ope2Reg);
					
					int num = A2I(ope2);
					string paraReg = FORMAT("R{}", num - 1);
					//push�������Ƕ�Ӧ�Ĳ����Ĵ���r0-3Ϊռ��״̬��
					//����r0�����ܱ�ռ���𣿣���
					//�����ϲ����ڣ���Ϊ�м���������������֣�Ҫ�Ǳ�ʹ���ˣ�Ҳһ�����ͷ���
					string curVreg = FORMAT("VR{}", vrNumber++);	//�������Ĵ��������������ط����֣�����load
					regpool.markParaReg(paraReg, curVreg);		//��ǲ����Ĵ�����ռ��
					callRegs.at(callLayer)[paraReg] = curVreg;	//pushһ����ζ����������Ĵ�����ռ����
					//����������Ҫѹջ����ֵ��
				}
				else {					//ջ����
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
		//�����м������offset
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
		for (int l = 4; l < regBegin; l++) {
			globalReg.push_back(FORMAT("R{}", l));
		}
		func2gReg.push_back(globalReg);
	}
	LIR = LIRTmp;
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

