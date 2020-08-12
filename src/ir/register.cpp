#include "./register.h"
#include "./optimize.h"
#include <algorithm>
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

map<string, string> vreg2varReg;	//��¼����Ĵ����ͱ����Ĵ���֮��Ĺ�ϵ
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
		auto arr = var2Arr.at(k);
		vector<CodeItem> funcTmp;

		//��ʼ��

		// R12��Ϊ��ʱ�Ĵ���
		 vector<string> tmpRegs = { "R0", "R1", "R2", "R3", "R12" };	//��ʱ�Ĵ�����
		
		// R12����Ϊ��ʱ�Ĵ���
		//vector<string> tmpRegs = { "R0", "R1", "R2", "R3" };	//��ʱ�Ĵ�����

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
			//if (arr[vars.at(i)] != true) {
			//	var2reg[vars.at(i)] = FORMAT("R{}", regBegin++);
			//	first[vars.at(i)] = true;
			//	if (regBegin >= 12) {
			//		i++;
			//		break;
			//	}
			//}
			//else {	//���鲻����Ĵ���
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
		//ȫ�ֱ���ʹ����ʱ���������غ�ʹ��

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
				if (isVreg(res)) {
					resReg = allocTmpReg(regpool, res, funcTmp);	//����Ĵ������������һ��
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == ADD || op == MUL) {	//�������������ͷ�
				if (!isNumber(ope2)) {	//����������
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
				}
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseVreg(ope1);
				regpool.releaseVreg(ope2);
				resReg = allocTmpReg(regpool, res, funcTmp);	//����Ĵ������������һ��
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == AND || op == OR ||
					 op == EQL || op == NEQ || op == SGT || op == SGE || op == SLT || op == SLE) {
				if (!isNumber(ope2)) {	//����������
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
				}
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseVreg(ope1);
				regpool.releaseVreg(ope2);
				if (isVreg(res)) {
					resReg = allocTmpReg(regpool, res, funcTmp);	//����Ĵ������������һ��
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == SUB || op == DIV || op == REM) {	//�������������ͷ�
				if (!isNumber(ope2)) {	//����������
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
				}
				if (!isNumber(ope1)) {	//����������
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				}
				regpool.releaseVreg(ope1);
				regpool.releaseVreg(ope2);
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
// �Ĵ�������2
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
		if (reg2avail[pool.at(i)] == true) {	//�ҵ��˿��üĴ���
			reg2avail[pool.at(i)] = false;		//���Ϊʹ��
			var2reg[var] = pool.at(i);			//��¼�����ͼĴ�����ӳ���ϵ
			used[pool.at(i)] = true;			//ֻҪȫ�ּĴ�����ʹ�ù��ͼ�¼����
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
	reg2avail[reg] = true;									//���ȫ�ּĴ������ã�
	for (auto it = var2reg.begin(); it != var2reg.end();) {	//ɾ��֮ǰ ���� <-> ȫ�ּĴ��� ��ӳ���ϵ
		if (it->first == var) {
			it = var2reg.erase(it);
			continue;
		}
		it++;
	}
}

void GlobalRegPool::releaseNorActRegs(set<string> inVars, set<string> outVars) {
	for (auto it = var2reg.begin(); it != var2reg.end();) {
		//��in��Ծ����out����Ծ�������ͷżĴ���
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
//		//��ʼ��
//		vector<string> tmpRegs = { "R0", "R1", "R2", "R3", "R12" };	//��ʱ�Ĵ�����
//		RegPool regpool(tmpRegs);
//		vector<string> tmpGregs = { "R4","R5","R6", "R7", "R8", "R9", "R10", "R11" };	//ȫ�ּĴ���
//		GlobalRegPool gRegpool(tmpGregs);
//
//		vreg2varReg.clear();	//ֻ��loadʹ��
//		first.clear();
//		vr2index.clear();
//		vrNum = 0;
//
//		//��㺯�����ò����Ĵ������湤��
//		int vrNumber = func2vrIndex.at(k);
//		callLayer = -1;
//		callRegs.clear();		//�洢 r[0-3] <-> VR[0-9]
//		paraReg2push.clear();	//�����Ĵ�����ѹջ����ģ���ʵ��str���棩
//
//		//1. �ֲ�����ʵ���ϲ�����Ҫ�Ĵ���
//		//2. ��Ծ����������̬�ͷŲ���ʹ�õı���ռ�õļĴ���
//		//ȫ�ֱ���ʹ����ʱ���������غ�ʹ��
//
//		//����һ��ʼȫ�ֳ�ʼ��Ϊmemory
//		for (int m = 0; m < vars.size(); m++) {	//�������������Ĵ���
//			var2reg[vars.at(m)] = gRegpool.allocReg(vars.at(m));
//		}
//
//		string funcName = "";
//		for (int i = 0; i < func.size(); i++) {
//			auto ir = func.at(i).Ir;
//			auto in = func.at(i).in;	//�������Ծ����in
//			auto out = func.at(i).out;	//�������Ծ����out
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
//				//res�ֶη�����ʱ�Ĵ���
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
//					if (!isNumber(ope2)) {	//����������
//						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//					}
//					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//					regpool.releaseVreg(ope1);
//					regpool.releaseVreg(ope2);
//					resReg = allocTmpReg(regpool, res, funcTmp);	//����Ĵ������������һ��
//					instr.setInstr(resReg, ope1Reg, ope2Reg);
//				}
//				else if (op == SUB || op == DIV || op == REM) {
//					if (!isNumber(ope2)) {	//����������
//						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//					}
//					if (!isNumber(ope1)) {	//����������
//						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//					}
//					regpool.releaseVreg(ope1);
//					regpool.releaseVreg(ope2);
//					resReg = allocTmpReg(regpool, res, funcTmp);	//����Ĵ������������һ��
//					instr.setInstr(resReg, ope1Reg, ope2Reg);
//				}
//				else if (op == LEA) {
//					ope1Reg = allocTmpReg(regpool, ope1, funcTmp);
//					instr.setInstr(resReg, ope1Reg, ope2Reg);
//				}
//				else if (op == MOV) {	//������ʱ�Ĵ���
//					if (isVreg(ope2)) {
//						ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//						regpool.releaseVreg(ope2);
//					}
//					if (isVreg(ope1)) {
//						ope1Reg = allocTmpReg(regpool, ope1, funcTmp);
//					}
//					instr.setInstr(resReg, ope1Reg, ope2Reg);
//				}
//				else if (op == LOAD) {	//�����ַ�ļ��ض�������ʱ�Ĵ��������������������mov����load
//					if (ope2 == "para") {	//���ز�������ĵ�ַ 
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
//					else if (ope2 == "array") {	//���ؾֲ�����ĵ�ַ
//						resReg = allocTmpReg(regpool, res, funcTmp);
//						instr.setCodetype(LEA);
//						instr.setInstr("", resReg, ope1Reg);
//					}
//					else {	//�������ַ����
//						if (isVreg(ope1)) {	//ȫ�ֱ���
//							ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//							regpool.releaseVreg(ope1);
//							resReg = allocTmpReg(regpool, res, funcTmp);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//						else {	//ջ����
//							string regTmp = gRegpool.getReg(ope1);
//							if (regTmp == "memory") {			//����û�б�����Ĵ�����ʹ����ʱ�Ĵ�����ֵ
//								resReg = allocTmpReg(regpool, res, funcTmp);
//								instr.setInstr(resReg, ope1Reg, ope2Reg);
//							}
//							else {								//ָ�ɵ�ȫ�ּĴ���
//								//�ֲ��������������ȴ�ջ�����ֵ
//								ope1Reg = regTmp;
//								vreg2varReg[res] = ope1Reg;		//vreg <-> varReg
//								instr.setCodetype(MOV);
//								instr.setInstr("", ope1Reg, ope1Reg);
//							}
//						}
//					}
//				}
//				else if (op == LOADARR) {
//					if (isNumber(ope2)) {	//ƫ����������
//						if (isVreg(ope1)) {		//ȫ������
//							ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//							regpool.releaseVreg(ope1);
//							resReg = allocTmpReg(regpool, res, funcTmp);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//						else {					//ջ����
//							resReg = allocTmpReg(regpool, res, funcTmp);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//					}
//					else {					//ƫ���ǼĴ���
//						if (isVreg(ope1)) {		//ȫ������
//							ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//							ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//							regpool.releaseVreg(ope1);
//							regpool.releaseVreg(ope2);
//							resReg = allocTmpReg(regpool, res, funcTmp);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//						else {					//ջ����
//							ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//							regpool.releaseVreg(ope2);
//							resReg = allocTmpReg(regpool, res, funcTmp);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//					}
//				}
//				else if (op == STORE) {
//					if (isVreg(ope1)) {		//ȫ�ֱ���
//						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//						resReg = getTmpReg(regpool, res, funcTmp);
//						regpool.releaseVreg(ope1);
//						regpool.releaseVreg(res);
//						instr.setInstr(resReg, ope1Reg, ope2Reg);
//					}
//					else {	//ջ����
//						if (gRegpool.getReg(ope1) != "memory") {	//�����Ѿ������˼Ĵ���
//							resReg = getTmpReg(regpool, res, funcTmp);
//							regpool.releaseVreg(res);
//							ope1Reg = gRegpool.getReg(ope1);		//��������ڱ�����Ӧ�ļĴ���
//							instr.setCodetype(MOV);
//							instr.setInstr("", ope1Reg, resReg);
//						}
//						else {
//							string regTmp = gRegpool.allocReg(ope1);	//��������һ��ȫ�ּĴ���
//							if (regTmp == "memory") {	//����ʧ��
//								resReg = getTmpReg(regpool, res, funcTmp);
//								regpool.releaseVreg(res);
//								instr.setInstr(resReg, ope1Reg, ope2Reg);
//							}
//							else {	//��һ������
//								resReg = getTmpReg(regpool, res, funcTmp);
//								regpool.releaseVreg(res);
//								ope1Reg = regTmp;					//��������ڱ�����Ӧ�ļĴ���
//								instr.setCodetype(MOV);
//								instr.setInstr("", ope1Reg, resReg);
//							}
//						}
//					}
//				}
//				else if (op == STOREARR) {
//					if (isNumber(ope2)) {	//ƫ����������
//						if (isVreg(ope1)) {		//ȫ������
//							ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//							resReg = getTmpReg(regpool, res, funcTmp);
//							regpool.releaseVreg(ope1);
//							regpool.releaseVreg(res);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//						else {					//ջ����
//							//��Ϊ�����飬����ļĴ���û�����壬������ ��memory��
//							resReg = getTmpReg(regpool, res, funcTmp);
//							regpool.releaseVreg(res);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//					}
//					else {					//ƫ���ǼĴ���
//						if (isVreg(ope1)) {		//ȫ������
//							ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//							ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//							resReg = getTmpReg(regpool, res, funcTmp);
//							regpool.releaseVreg(ope1);
//							regpool.releaseVreg(ope2);
//							regpool.releaseVreg(res);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//						else {					//ջ����
//							//��Ϊ�����飬����ļĴ���û�����壬������ ��memory��
//							resReg = getTmpReg(regpool, res, funcTmp);
//							ope2Reg = getTmpReg(regpool, ope2, funcTmp);
//							regpool.releaseVreg(ope2);
//							regpool.releaseVreg(res);
//							instr.setInstr(resReg, ope1Reg, ope2Reg);
//						}
//					}
//				}
//				else if (op == POP) {	//ò���ò���
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
//				else if (op == PARA) {	//ǰ�ĸ�������������Ĵ������������stack���õ���ʱ���ٷ���
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
//					//��¼�����Ĳ����Ĵ��������	
//					callLayer++;
//					callRegs.push_back(map<string, string>());
//					paraReg2push.push_back(map<string, bool>());
//					//���ʽ������м�����ı��棡
//					auto save = regpool.spillAllRegs();
//					funcTmp.push_back(instr);
//					for (auto one : save) {		//�������õ�ʱ����ʱ�Ĵ�����ֵ��Ҫ����
//						setVrIndex(one.first);
//						//�������ļĴ�����push�����Ĵ���
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
//					//�������Ƕ�׵���r0-r3��spill���ں�������ǰ��ȡr0-r3
//					if (callLayer >= 0) {
//						//������м�����Ѳ����Ĵ�������ˣ���ô���
//						for (auto para : paraReg2push.at(callLayer)) {
//							//??????
//							if (para.second == true) {		//��������Ĵ���������ˣ���Ҫload����
//								string vRegTmp = callRegs.at(callLayer)[para.first];
//								CodeItem loadTmp(LOAD, para.first, vRegTmp, "para");
//								funcTmp.push_back(loadTmp);
//							}
//						}
//					}
//					for (auto para : callRegs.at(callLayer)) {		//�ͷŲ����Ĵ��� R0-R3
//						regpool.releaseVreg(para.second);
//					}
//					callLayer--;			//�����㼶����
//					callRegs.pop_back();	//�������üĴ���ɾ��
//					paraReg2push.pop_back();
//				}
//				else if (op == PUSH) {	//����ѹջ 4�Ƿֽ��
//					if (A2I(ope2) <= 4) {	//ǰ4���˲���
//						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//						regpool.releaseVreg(ope1);
//						instr.setInstr(resReg, ope1Reg, ope2Reg);
//
//						int num = A2I(ope2);
//						string paraReg = FORMAT("R{}", num - 1);
//						string curVreg = FORMAT("VR{}", vrNumber++);	//�������Ĵ��������������ط����֣�����load
//						regpool.markParaReg(paraReg, curVreg);		//��ǲ����Ĵ�����ռ��
//						callRegs.at(callLayer)[paraReg] = curVreg;	//pushһ����ζ����������Ĵ�����ռ����
//						//����������Ҫѹջ����ֵ��
//					}
//					else {					//ջ����
//						ope1Reg = getTmpReg(regpool, ope1, funcTmp);
//						regpool.releaseVreg(ope1);
//						instr.setInstr(resReg, ope1Reg, ope2Reg);
//					}
//				}
//				else {
//					//do nothing!
//				}
//				funcTmp.push_back(instr);
//			}	//ÿ��ir�Ľ�β
//			gRegpool.noteRegRelations(funcTmp);
//			//���в���Ծ������д�أ�Ȼ���ͷżĴ���
//			//if (!retFlag) {
//			//	if (brFlag) {
//			//		gRegpool.releaseNorActRegs(func.at(i).in, func.at(i).out, funcTmp, -1);	//���ָ���λ���ڵ����ڶ���
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
// �Ĵ�������3 ���� ͼ��ɫ����
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

		//��ʼ��

		// R12��Ϊ��ʱ�Ĵ���
		vector<string> tmpRegs = { "R0", "R1", "R2", "R3", "R12"};	//��ʱ�Ĵ�����
		
		// R12����Ϊ��ʱ�Ĵ���
		//vector<string> tmpRegs = { "R0", "R1", "R2", "R3" };	//��ʱ�Ĵ�����
		
		RegPool regpool(tmpRegs);
		vreg2varReg.clear();
		first.clear();
		var2reg.clear();
		regBegin = 4;
		vr2index.clear();
		vrNum = 0;
		int paraNum = 0;

		//��㺯�����ò����Ĵ������湤��
		int vrNumber = func2vrIndex.at(k);
		callLayer = -1;
		callRegs.clear();		//�洢 r[0-3] <-> VR[0-9]
		paraReg2push.clear();	//�����Ĵ�����ѹջ����ģ���ʵ��str���棩

		//����ͼ��ɫ��Ϣ����Ĵ���
		for (int i = 0; i < vars.size(); i++) {
			if (regAlloc.find(vars.at(i)) != regAlloc.end()) {
				var2reg[vars.at(i)] = regAlloc[vars.at(i)];
				first[vars.at(i)] = true;
			}
			else {
				if (!leftGregs.empty() && arr[vars.at(i)] == true) {	//��alloc��ʱ��ѵ�ַȡ����
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
		//ȫ�ֱ���ʹ����ʱ���������غ�ʹ��

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
			}
			else if (op == NOT) {
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseVreg(ope1);
				if (isVreg(res)) {
					resReg = allocTmpReg(regpool, res, funcTmp);	//����Ĵ������������һ��
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == ADD || op == MUL) {	//�������������ͷ�
				if (!isNumber(ope2)) {	//����������
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
				}
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseVreg(ope1);
				regpool.releaseVreg(ope2);
				resReg = allocTmpReg(regpool, res, funcTmp);	//����Ĵ������������һ��
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == AND || op == OR ||
				op == EQL || op == NEQ || op == SGT || op == SGE || op == SLT || op == SLE) {
				if (!isNumber(ope2)) {	//����������
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
				}
				ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				regpool.releaseVreg(ope1);
				regpool.releaseVreg(ope2);
				if (isVreg(res)) {
					resReg = allocTmpReg(regpool, res, funcTmp);	//����Ĵ������������һ��
				}
				instr.setInstr(resReg, ope1Reg, ope2Reg);
			}
			else if (op == SUB || op == DIV || op == REM) {	//�������������ͷ�
				if (!isNumber(ope2)) {	//����������
					ope2Reg = getTmpReg(regpool, ope2, funcTmp);
				}
				if (!isNumber(ope1)) {	//����������
					ope1Reg = getTmpReg(regpool, ope1, funcTmp);
				}
				regpool.releaseVreg(ope1);
				regpool.releaseVreg(ope2);
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
						WARN_MSG("array have no reg! wrong!");	//�����ϲ�����������֧
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
							else {//��ջ��Ĳ������û�мĴ�����������Ҫload�������мĴ��������Ǵ���ջ��ģ���Ҫ��һ�μ��س���
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
						if (var2reg[ope1] != "memory") {	//�������ַ�мĴ���
							instr.setInstr(resReg, var2reg[ope1], ope2Reg);
						}
						else {

							instr.setInstr(resReg, ope1Reg, ope2Reg);
						}
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
						if (var2reg[ope1] != "memory") {	//�������ַ�мĴ���
							instr.setInstr(resReg, var2reg[ope1], ope2Reg);
						}
						else {

							instr.setInstr(resReg, ope1Reg, ope2Reg);
						}
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
						if (var2reg[ope1] != "memory") {	//�������ַ�мĴ���
							instr.setInstr(resReg, var2reg[ope1], ope2Reg);
						}
						else {

							instr.setInstr(resReg, ope1Reg, ope2Reg);
						}
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
						if (var2reg[ope1] != "memory") {	//�������ַ�мĴ���
							instr.setInstr(resReg, var2reg[ope1], ope2Reg);
						}
						else {
							
							instr.setInstr(resReg, ope1Reg, ope2Reg);
						}
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
				if (arr[res] == true && var2reg[res] != "memory") {	//�ֲ��������мĴ���
					funcTmp.push_back(CodeItem(LEA, "", var2reg[res], res));	//ȡ�ֲ�����ĵ�ַ
					first[res] = false;
				}
			}
			else if (op == PARA) {
				ope1Reg = var2reg[res];
				instr.setInstr(resReg, ope1Reg, ope2Reg);
				paraNum++;
				//ͼ��ɫ��������˼Ĵ�����mov�����û�в���ʹǰ�ĸ���������Ҫstore��ջ
				if (paraNum <= 4) {
					if (var2reg[res] != "memory") {	//�����˼Ĵ�����ֱ��MOV
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
