#include "optimize.h"

//===============================================================
//将MIR转换为LIR，把临时变量都用虚拟寄存器替换，vrIndex从0开始编号
//===============================================================

vector<vector<CodeItem>> LIR;

int vrIndex;
string curVreg = "";
map<string, string> tmp2vr;

void setInstr(CodeItem& instr, string res, string ope1, string ope2) {
	instr.setResult(res);
	instr.setOperand1(ope1);
	instr.setOperand2(ope2);
}

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

void MIR2LIRpass() {
	for (int i = 0; i < codetotal.size(); i++) {
		vector<CodeItem> src = codetotal.at(i);
		vector<CodeItem> dst;
		vrIndex = 0;
		for (int j = 0; j < src.size(); j++) {
			CodeItem instr = src.at(j);
			irCodeType op = instr.getCodetype();
			string res = instr.getResult();
			string ope1 = instr.getOperand1();
			string ope2 = instr.getOperand2();
			if (op == ADD || op == SUB || op == MUL || op == DIV || op == REM ||
				op == AND || op == OR || op == NOT ||
				op == EQL || op == NEQ || op == SGT || op == SGE || op == SLT || op == SGE) {
				if (isNumber(ope1)) {
					dst.push_back(CodeItem(MOV, "", getVreg(), ope1));
					ope1 = curVreg;
				}
				res = dealTmpOpe(res);
				ope1 = dealTmpOpe(ope1);
				ope2 = dealTmpOpe(ope2);
				setInstr(instr, res, ope1, ope2);
				dst.push_back(instr);
			}
			else if (op == LOAD) {
				if (1) {	//全局变量
					CodeItem address(LEA, "", getVreg(), ope1);
					ope1 = curVreg;
					res = dealTmpOpe(res);
					setInstr(instr, res, ope1, ope2);
					dst.push_back(address);
					dst.push_back(instr);
				}
				else {
					res = dealTmpOpe(res);
					setInstr(instr, res, ope1, ope2);
					dst.push_back(instr);
				}
			}
			else if (op == LOADARR) {

			}
			else if (op == STORE) {

			}
			else if (op == STOREARR) {

			}
			else if (op == CALL) {

			}
			else if (op == RET) {

			}
			else if (op == PUSH) {

			}
			else if (op == POP) {

			}
			else if (op == BR) {

			}
			else {
				dst.push_back(instr);
			}
		}
		LIR.push_back(dst);
	}
}

//================================================================
//基于活跃变量的简单寄存器分配，临时变量采用寄存器池分配临时寄存器
//================================================================

void irOptimize() {


	//寄存器分配优化
	//MIR2LIRpass(codetotal);
}