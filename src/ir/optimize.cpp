#include "optimize.h"

//===============================================================
//将MIR转换为LIR，把临时变量都用虚拟寄存器替换，vrIndex从0开始编号
//===============================================================
int vrIndex = 0;
map<string, string> tmp2vr;

void setInstr(CodeItem& instr, string res, string ope1, string ope2) {
	instr.setResult(res);
	instr.setOperand1(ope1);
	instr.setOperand2(ope2);
	instr.changeContent(res, ope1, ope2);
}

string dealOperand(string operand) {
	if (isTmp(operand)) {
		if (tmp2vr.find(operand) != tmp2vr.end()) {
			operand = tmp2vr[operand];
		}
		else {
			string vr = FORMAT("VR{}", vrIndex++);
			tmp2vr[operand] = vr;
			operand = vr;
		}
	}
	return operand;
}

void MIR2LIRpass(vector<vector<CodeItem>>& irCodes) {
	for (int i = 1; i < irCodes.size(); i++) {
		vector<CodeItem>& func = irCodes.at(i);
		vrIndex = 0;
		for (int j = 0; j < func.size(); j++) {
			CodeItem& instr = func.at(j);
			//处理instr的result字段
			string res = dealOperand(instr.getResult());
			//处理instr的operand1字段
			string ope1 = dealOperand(instr.getOperand1());
			//处理instr的operand2字段
			string ope2 = dealOperand(instr.getOperand2());
			setInstr(instr, res, ope1, ope2);
		}
	}
}

//================================================================
//基于活跃变量的简单寄存器分配，临时变量采用寄存器池分配临时寄存器
//================================================================

void registerAllocation() {
	SSA ssa;
	ssa.generate();


}



void irOptimize() {


	//寄存器分配优化
	MIR2LIRpass(codetotal);

}