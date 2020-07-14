#include "optimize.h"
#include "register.h"

//===============================================================
//将MIR转换为LIR，把临时变量都用虚拟寄存器替换，vrIndex从0开始编号
//===============================================================

vector<vector<CodeItem>> LIR;

int vrIndex;
string curVreg = "";
map<string, string> tmp2vr;

string getVreg() {
	curVreg = FORMAT("VR{}", vrIndex++);
	return curVreg;
}

string dealTmpOpe(string operand) {
	if (isTmp(operand)) {
		if (tmp2vr.find(operand) != tmp2vr.end()) {
			operand = tmp2vr[operand];
		}
		else {
			string vr = getVreg();
			tmp2vr[operand] = vr;
			operand = vr;
		}
	}
	return operand;
}

void dealNumber(CodeItem& instr) {
	auto op = instr.getCodetype();
	string res = instr.getResult();
	string ope1 = instr.getOperand1();
	string ope2 = instr.getOperand2();
	if (op == ADD || op == MUL || op == AND || op == OR || op == EQL || op == NEQ) {
		if (isNumber(ope1)) {
			string tmp = ope2;
			ope2 = ope1;
			ope1 = tmp;
			instr.setInstr(res, ope1, ope2);
		}
	}
	else if (op == SGT) {
		if (isNumber(ope1)) {
			string tmp = ope2;
			ope2 = ope1;
			ope1 = tmp;
			instr.setInstr(res, ope1, ope2);
			instr.setCodetype(SLE);
		}
	}
	else if (op == SLT) {
		if (isNumber(ope1)) {
			string tmp = ope2;
			ope2 = ope1;
			ope1 = tmp;
			instr.setInstr(res, ope1, ope2);
			instr.setCodetype(SGE);
		}
	}
	else if (op == SGE) {
		if (isNumber(ope1)) {
			string tmp = ope2;
			ope2 = ope1;
			ope1 = tmp;
			instr.setInstr(res, ope1, ope2);
			instr.setCodetype(SLT);
		}
	}
	else if (op == SLE) {
		if (isNumber(ope1)) {
			string tmp = ope2;
			ope2 = ope1;
			ope1 = tmp;
			instr.setInstr(res, ope1, ope2);
			instr.setCodetype(SGT);
		}
	}
	else {

	}
}

void MIR2LIRpass() {
	LIR.push_back(codetotal.at(0));
	for (int i = 1; i < codetotal.size(); i++) {
		vector<CodeItem> src = codetotal.at(i);
		vector<CodeItem> dst;
		vrIndex = 0;
		curVreg = "";
		tmp2vr.clear();
		for (int j = 0; j < src.size(); j++) {
			CodeItem instr = src.at(j);
			dealNumber(instr);
			irCodeType op = instr.getCodetype();
			string res = instr.getResult();
			string ope1 = instr.getOperand1();
			string ope2 = instr.getOperand2();
			if (op == NOT) {
				if (isNumber(ope1)) {
					int tmp = A2I(ope1);
					if (tmp == 0) {
						dst.push_back(CodeItem(MOV, "", res, "1"));
					}
					else {
						dst.push_back(CodeItem(MOV, "", res, "0"));
					}
				}
				else {
					dst.push_back(instr);
				}
			}else if (op == ADD || op == SUB || op == MUL || op == DIV || op == REM ||
					  op == AND || op == OR ||
					  op == EQL || op == NEQ || op == SGT || op == SGE || op == SLT || op == SGE) {
				if (isNumber(ope1)) {
					dst.push_back(CodeItem(MOV, "", getVreg(), ope1));
					ope1 = curVreg;
				}
				res = dealTmpOpe(res);
				ope1 = dealTmpOpe(ope1);
				ope2 = dealTmpOpe(ope2);
				instr.setInstr(res, ope1, ope2);
				dst.push_back(instr);
			}
			else if (op == LOAD) {
				if (isGlobal(ope1)) {	//全局变量
					CodeItem address(LEA, "", getVreg(), ope1);
					ope1 = curVreg;
					res = dealTmpOpe(res);
					instr.setInstr(res, ope1, ope2);
					dst.push_back(address);
					dst.push_back(instr);
				}
				else {
					res = dealTmpOpe(res);
					instr.setInstr(res, ope1, ope2);
					dst.push_back(instr);
				}
			}
			else if (op == LOADARR) {
				if (isNumber(ope2)) {	//偏移是立即数
					if (isGlobal(ope1)) {	//数组是全局数组
						CodeItem address(LEA, "", getVreg(), ope1);
						ope1 = curVreg;
						res = dealTmpOpe(res);
						dst.push_back(address);
						instr.setInstr(res, ope1, ope2);
						dst.push_back(instr);
					}
					else {
						res = dealTmpOpe(res);
						instr.setInstr(res, ope1, ope2);
						dst.push_back(instr);
					}
				}
				else {
					if (isGlobal(ope1)) {
						CodeItem address(LEA, "", getVreg(), ope1);
						ope1 = curVreg;
						res = dealTmpOpe(res);
						ope2 = dealTmpOpe(ope2);
						dst.push_back(address);
						instr.setInstr(res, ope1, ope2);
						dst.push_back(instr);
					}
					else {
						res = dealTmpOpe(res);
						ope2 = dealTmpOpe(ope2);
						instr.setInstr(res, ope1, ope2);
						dst.push_back(instr);
					}
				}
			}
			else if (op == STORE) {
				if (isNumber(res)) {	//存一个立即数
					if (isGlobal(ope1)) {	//全局变量
						CodeItem movNumber(MOV, "", getVreg(), res);
						res = curVreg;
						CodeItem address(LEA, "", getVreg(), ope1);
						ope1 = curVreg;
						res = dealTmpOpe(res);
						instr.setInstr(res, ope1, ope2);
						dst.push_back(movNumber);
						dst.push_back(address);
						dst.push_back(instr);
					}
					else {
						CodeItem movNumber(MOV, "", getVreg(), res);
						res = curVreg;
						instr.setInstr(res, ope1, ope2);
						dst.push_back(movNumber);
						dst.push_back(instr);
					}
				}
				else {					//存一个寄存器
					if (isGlobal(ope1)) {	//全局变量
						CodeItem address(LEA, "", getVreg(), ope1);
						ope1 = curVreg;
						res = dealTmpOpe(res);
						instr.setInstr(res, ope1, ope2);
						dst.push_back(address);
						dst.push_back(instr);
					}
					else {
						res = dealTmpOpe(res);
						instr.setInstr(res, ope1, ope2);
						dst.push_back(instr);
					}
				}
			}
			else if (op == STOREARR) {
				if (isNumber(ope2)) {	//偏移是立即数
					if (isGlobal(ope1)) {	//全局数组
						CodeItem address(LEA, "", getVreg(), ope1);
						ope1 = curVreg;
						res = dealTmpOpe(res);
						dst.push_back(address);
						instr.setInstr(res, ope1, ope2);
						dst.push_back(instr);
					}
					else {
						CodeItem movNumber(MOV, "", getVreg(), res);
						res = curVreg;
						instr.setInstr(res, ope1, ope2);
						dst.push_back(movNumber);
						dst.push_back(instr);
					}
				}
				else {
					if (isGlobal(ope1)) {
						CodeItem address(LEA, "", getVreg(), ope1);
						ope1 = curVreg;
						res = dealTmpOpe(res);
						ope2 = dealTmpOpe(ope2);
						dst.push_back(address);
						instr.setInstr(res, ope1, ope2);
						dst.push_back(instr);
					}
					else {
						res = dealTmpOpe(res);
						ope2 = dealTmpOpe(ope2);
						instr.setInstr(res, ope1, ope2);
						dst.push_back(instr);
					}
				}
			}
			else if (op == CALL) {
				if (isTmp(res)) {
					res = dealTmpOpe(res);
					CodeItem tmp(MOV, "", res, "R0");
					instr.setInstr(res, ope1, ope2);
					dst.push_back(instr);
					dst.push_back(tmp);
				}
				else {
					dst.push_back(instr);
				}
			}
			else if (op == RET) {
				if (ope2 != "void") {
					ope1 = dealTmpOpe(ope1);
					CodeItem tmp(MOV, "", "R0", ope1);
					dst.push_back(tmp);
				}
				ope1 = "R0";
				instr.setInstr(res, ope1, ope2);
				dst.push_back(instr);
			}
			else if (op == PUSH || op == POP) {
				if (isNumber(ope1)) {
					CodeItem tmp(MOV, "", getVreg(), ope1);
					ope1 = curVreg;
					dst.push_back(tmp);
				}
				res = dealTmpOpe(res);
				instr.setInstr(res, ope1, ope2);
				dst.push_back(instr);
			}
			else if (op == BR) {
				if (isNumber(ope1)) {
					CodeItem tmp(MOV, "", getVreg(), ope1);
					ope1 = curVreg;
					dst.push_back(tmp);
				}
				ope1 = dealTmpOpe(ope1);
				instr.setInstr(res, ope1, ope2);
				dst.push_back(instr);
			}
			else {
				dst.push_back(instr);
			}
		}
		LIR.push_back(dst);
	}
}

void printLIR(string outputFile) {
	ofstream irtxt(outputFile);
	for (int i = 0; i < LIR.size(); i++) {
		vector<CodeItem> item = LIR[i];
		for (int j = 0; j < item.size(); j++) {
			//cout << item[j].getContent() << endl;
			irtxt << item[j].getContent() << endl;
		}
		//cout << "\n";
		irtxt << "\n";
	}
	irtxt.close();
}

vector<vector<string>> stackVars;

void countVars() {
	stackVars.push_back(vector<string>());
	for (int i = 1; i < codetotal.size(); i++) {
		auto func = codetotal.at(i);
		vector<string> vars;
		for (int i = 1; i < func.size(); i++) {
			auto instr = func.at(i);
			if (instr.getCodetype() == PARA || instr.getCodetype() == ALLOC) {
				vars.push_back(instr.getResult());
			}
		}
		stackVars.push_back(vars);
	}
}

//检查irCode类型
// 1: 计算类型
int checkType(irCodeType op) {
	if (op == ADD ||  op == SUB || op == DIV || op == MUL || op ==REM ||
		op == AND || op == OR || op == NOT ||
		op == EQL || op == NEQ || op == SGT || op == SGE || op == SLT || op ==SLE) {
		return 1;
	}
	return 0;
}

//窥孔优化
void peepholeOptimization() {
	for (int i = 1; i < LIR.size(); i++) {
		vector<CodeItem>& func = LIR.at(i);
		for (int j = 0; j < func.size() - 1; j++) {
			CodeItem& instr = func.at(j);
			CodeItem& next = func.at(j + 1);
			if ((checkType(instr.getCodetype()) == 1) && next.getCodetype() == MOV) {
				string res = instr.getResult();
				string nextOpe1 = next.getOperand1();
				string nextOpe2 = next.getOperand2();
				if (res == nextOpe2) {
					instr.setResult(nextOpe1);
					next.setOperand2(nextOpe1);
				}
			}
		}
		//删除多余的MOV指令
		for (int j = 1; j < func.size();) {
			auto op = func.at(j).getCodetype();
			auto ope1 = func.at(j).getOperand1();
			auto ope2 = func.at(j).getOperand2();
			if (op == MOV && ope1 == ope2) {
				func.erase(func.begin() + j);
				continue;
			}
			j++;
		}
	}
}

//=============================================================
// 优化函数
//=============================================================

void irOptimize() {
	
	//运行优化
	SSA ssa;
	ssa.generate();


	//寄存器分配优化
	MIR2LIRpass();
	printLIR("LIR.txt");
	countVars();

	//寄存器直接指派
	for (int i = 1; i < LIR.size(); i++) {
		registerAllocation(LIR.at(i), stackVars.at(i));
	}

	printLIR("armIR.txt");

	peepholeOptimization();

	printLIR("armIR_2.txt");

}

