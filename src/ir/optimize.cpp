#include "optimize.h"
#include "register.h"

//存放给后端的中间代码
vector<vector<CodeItem>> LIR;

//保存每个函数使用的全局寄存器，从1开始编号，和LIR一样
vector<vector<string>> func2gReg;

//保存每个函数用到的临时变量，VR->定义顺序
vector<map<string, int>> func2Vr;




//================================================================================
//将MIR转换为LIR，把临时变量都用虚拟寄存器替换，vrIndex从0开始编号
//================================================================================

vector<int> func2vrIndex;
int vrIndex;
string curVreg = "";
map<string, string> tmp2vr;

//获取一个虚拟寄存器
string getVreg() {
	curVreg = FORMAT("VR{}", vrIndex++);
	return curVreg;
}

//将临时变量转换为对应的虚拟寄存器
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

//中间代码保证不会出现两个立即数的情况
//如果第一操作数是立即数，和第二个操作数进行交换
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
			instr.setCodetype(SLT);
		}
	}
	else if (op == SLT) {
		if (isNumber(ope1)) {
			string tmp = ope2;
			ope2 = ope1;
			ope1 = tmp;
			instr.setInstr(res, ope1, ope2);
			instr.setCodetype(SGT);
		}
	}
	else if (op == SGE) {
		if (isNumber(ope1)) {
			string tmp = ope2;
			ope2 = ope1;
			ope1 = tmp;
			instr.setInstr(res, ope1, ope2);
			instr.setCodetype(SLE);
		}
	}
	else if (op == SLE) {
		if (isNumber(ope1)) {
			string tmp = ope2;
			ope2 = ope1;
			ope1 = tmp;
			instr.setInstr(res, ope1, ope2);
			instr.setCodetype(SGE);
		}
	}
	else {

	}
}

vector<set<string>> inlineArray;	//内联函数数组传参新定义变量名

void MIR2LIRpass() {
	LIR.push_back(codetotal.at(0));
	func2vrIndex.push_back(0);
 	for (int i = 1; i < codetotal.size(); i++) {	//遍历处理每个函数
		vector<CodeItem> src = codetotal.at(i);
		vector<CodeItem> dst;
		vrIndex = 0;
		curVreg = "";
		tmp2vr.clear();
		int paraNum = 0;
		set<string> paras;

		for (int j = 0; j < src.size(); j++) {
			CodeItem instr = src.at(j);
			dealNumber(instr);								//如果是第二个条件(if)就交换立即数的位置和对应的操作符
			irCodeType op = instr.getCodetype();
			string res = instr.getResult();
			string ope1 = instr.getOperand1();
			string ope2 = instr.getOperand2();

			if (op == NOT) {
				res = dealTmpOpe(res);
				ope1 = dealTmpOpe(ope1);
				instr.setInstr(res, ope1, ope2);
				dst.push_back(instr);
			}else if (op == ADD || op == MUL ||		//只有一个立即数，只能放在第二个ope2
					  op == AND || op == OR ||
					  op == EQL || op == NEQ || op == SGT || op == SGE || op == SLT || op == SLE) {
				res = dealTmpOpe(res);
				ope1 = dealTmpOpe(ope1);
				ope2 = dealTmpOpe(ope2);
				instr.setInstr(res, ope1, ope2);
				dst.push_back(instr);
			}
			else if (op == SUB || op == DIV || op == REM) {		//只会有一个立即数，第一个和第二个均可，后端处理
				res = dealTmpOpe(res);
				ope1 = dealTmpOpe(ope1);
				ope2 = dealTmpOpe(ope2);
				instr.setInstr(res, ope1, ope2);
				dst.push_back(instr);
			}
			else if (op == LOAD) {
				if (ope2 == "array") {	//加载数组的地址 —— 局部数组
					instr.setCodetype(LEA);
					res = dealTmpOpe(res);
					instr.setInstr("", res, ope1);
					dst.push_back(instr);
				}
				else if (ope2 == "para") {	//加载数组的地址 —— 参数数组
					res = dealTmpOpe(res);
					instr.setResult(res);
					dst.push_back(instr);
				}
				else {	//加载变量的值（非数组）
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
			else if (op == LOADARR) {
				if (isNumber(ope2)) {	//偏移是立即数
					if (inlineArray.size() > i && inlineArray.at(i).find(ope1) != inlineArray.at(i).end()) {	//是inline的数组元素
						CodeItem loadTmp(LOAD, getVreg(), ope1, "");
						ope1 = curVreg;
						dst.push_back(loadTmp);
						res = dealTmpOpe(res);
						instr.setInstr(res, ope1, ope2);
						dst.push_back(instr);
					}
					else {
						if (isGlobal(ope1)) {	//数组是全局数组
							CodeItem address(LEA, "", getVreg(), ope1);
							ope1 = curVreg;
							res = dealTmpOpe(res);
							dst.push_back(address);
							instr.setInstr(res, ope1, ope2);
							dst.push_back(instr);
						}
						else {
							if (paras.find(ope1) != paras.end()) { //如果是参数数组，需要加载地址到一个寄存器
								CodeItem loadTmp(LOAD, getVreg(), ope1, "para");
								ope1 = curVreg;
								dst.push_back(loadTmp);
							}
							res = dealTmpOpe(res);
							instr.setInstr(res, ope1, ope2);
							dst.push_back(instr);
						}
					}
				}
				else {
					if (inlineArray.size() > i && inlineArray.at(i).find(ope1) != inlineArray.at(i).end()) {	//是inline的数组元素
						CodeItem loadTmp(LOAD, getVreg(), ope1, "");
						ope1 = curVreg;
						dst.push_back(loadTmp);
						ope2 = dealTmpOpe(ope2);
						res = dealTmpOpe(res);
						instr.setInstr(res, ope1, ope2);
						dst.push_back(instr);
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
							if (paras.find(ope1) != paras.end()) { //如果是参数数组，需要加载地址到一个寄存器
								CodeItem loadTmp(LOAD, getVreg(), ope1, "para");
								ope1 = curVreg;
								dst.push_back(loadTmp);
							}
							res = dealTmpOpe(res);
							ope2 = dealTmpOpe(ope2);
							instr.setInstr(res, ope1, ope2);
							dst.push_back(instr);
						}
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
					if (inlineArray.size() > i && inlineArray.at(i).find(ope1) != inlineArray.at(i).end()) {	//是inline的数组元素
						CodeItem loadTmp(LOAD, getVreg(), ope1, "");
						ope1 = curVreg;
						dst.push_back(loadTmp);
						if (isNumber(res)) {
							CodeItem movNumber(MOV, "", getVreg(), res);
							dst.push_back(movNumber);
							res = curVreg;
						}
						else {
							res = dealTmpOpe(res);
						}
						instr.setInstr(res, ope1, ope2);
						dst.push_back(instr);
					}
					else {
						if (isGlobal(ope1)) {	//全局数组
							CodeItem address(LEA, "", getVreg(), ope1);
							ope1 = curVreg;
							if (isNumber(res)) {
								CodeItem movNumber(MOV, "", getVreg(), res);
								dst.push_back(movNumber);
								res = curVreg;
							}
							else {
								res = dealTmpOpe(res);
							}
							dst.push_back(address);
							instr.setInstr(res, ope1, ope2);
							dst.push_back(instr);
						}
						else {
							if (isNumber(res)) {
								CodeItem movNumber(MOV, "", getVreg(), res);
								dst.push_back(movNumber);
								res = curVreg;
							}
							else {
								res = dealTmpOpe(res);
							}
							if (paras.find(ope1) != paras.end()) {
								CodeItem loadTmp(LOAD, getVreg(), ope1, "para");
								ope1 = curVreg;
								dst.push_back(loadTmp);
							}
							instr.setInstr(res, ope1, ope2);
							dst.push_back(instr);
						}
					}
				}
				else {		//偏移是寄存器
					if (inlineArray.size() > i && inlineArray.at(i).find(ope1) != inlineArray.at(i).end()) {	//是inline的数组元素
						CodeItem loadTmp(LOAD, getVreg(), ope1, "");
						ope1 = curVreg;
						dst.push_back(loadTmp);
						ope2 = dealTmpOpe(ope2);
						if (isNumber(res)) {
							CodeItem movNumber(MOV, "", getVreg(), res);
							dst.push_back(movNumber);
							res = curVreg;
						}
						else {
							res = dealTmpOpe(res);
						}
						instr.setInstr(res, ope1, ope2);
						dst.push_back(instr);
					}
					else {
						if (isGlobal(ope1)) {		//全局数组
							CodeItem address(LEA, "", getVreg(), ope1);
							ope1 = curVreg;
							if (isNumber(res)) {
								CodeItem movNumber(MOV, "", getVreg(), res);
								dst.push_back(movNumber);
								res = curVreg;
							}
							else {
								res = dealTmpOpe(res);
							}
							ope2 = dealTmpOpe(ope2);
							dst.push_back(address);
							instr.setInstr(res, ope1, ope2);
							dst.push_back(instr);
						}
						else {
							if (isNumber(res)) {
								CodeItem movNumber(MOV, "", getVreg(), res);
								dst.push_back(movNumber);
								res = curVreg;
							}
							else {
								res = dealTmpOpe(res);
							}
							if (paras.find(ope1) != paras.end()) {
								CodeItem loadTmp(LOAD, getVreg(), ope1, "para");
								ope1 = curVreg;
								dst.push_back(loadTmp);
							}
							ope2 = dealTmpOpe(ope2);
							instr.setInstr(res, ope1, ope2);
							dst.push_back(instr);
						}
					}
				}
			}
			else if (op == CALL) {
				if (isTmp(res)) {
					res = dealTmpOpe(res);
					CodeItem tmp(GETREG, "", res, "");
					instr.setInstr("", ope1, ope2);
					dst.push_back(instr);
					dst.push_back(tmp);
				}
				else {
					CodeItem tmp(GETREG, "", "", "");
					dst.push_back(instr);
					dst.push_back(tmp);
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
			else if (op == PUSH) {
				if (res == "int*") {
					ope1 = dealTmpOpe(ope1);
					instr.setOperand1(ope1);
					dst.push_back(instr);
				}
				else if (res == "string") {
					CodeItem tmp(MOV, "", getVreg(), ope1);
					ope1 = curVreg;
					instr.setInstr(res, ope1, ope2);
					dst.push_back(tmp);
					dst.push_back(instr);
				}
				else {
					if (isNumber(ope1)) {
						CodeItem tmp(MOV, "", getVreg(), ope1);
						ope1 = curVreg;
						dst.push_back(tmp);
					}
					ope1 = dealTmpOpe(ope1);
					instr.setInstr(res, ope1, ope2);
					dst.push_back(instr);
				}
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
			else if (op == PARA) {
				if (paraNum < 4) {
					instr.setOperand1(FORMAT("R{}", paraNum++));
				}
				else {
					instr.setOperand1("stack");
				}
				dst.push_back(instr);
				paras.insert(res);
			}
			else {
				dst.push_back(instr);
			}
		}
		//删除多余的call的res字段
		map<string, bool> callres2use;
		for (auto instr : dst) {
			auto op = instr.getCodetype();
			switch (op)
			{
			case ADD:case SUB:case DIV:case MUL:case REM:case AND:case OR:case NOT:case EQL:case NEQ:case SGT:case SGE:case SLT:case SLE:
			case STORE:case STOREARR:
			case LOAD:case LOADARR:case PUSH:case POP:case BR:case MOV:case LEA: {
				if (callres2use.find(instr.getResult()) != callres2use.end()) callres2use[instr.getResult()] = true;
				if (callres2use.find(instr.getOperand1()) != callres2use.end()) callres2use[instr.getOperand1()] = true;
				if (callres2use.find(instr.getOperand2()) != callres2use.end()) callres2use[instr.getOperand2()] = true;
				break;
			}
			case GETREG: {
				if (isVreg(instr.getOperand1())) {
					callres2use[instr.getOperand1()] = false;	//先设置为不会被使用
				}
				break;
			}
			default:
				break;
			}
		}
		for (auto& instr : dst) {
			if (instr.getCodetype() == GETREG 
				&& callres2use.find(instr.getOperand1()) != callres2use.end() && callres2use[instr.getOperand1()] == false) {
				instr.setOperand1("");
			}
		}
		func2vrIndex.push_back(vrIndex);
		LIR.push_back(dst);
	}
}

void printLIR(string outputFile) {
	if (TIJIAO) {
		ofstream irtxt(outputFile);
		for (int i = 0; i < LIR.size(); i++) {
			vector<CodeItem> item = LIR[i];
			for (int j = 0; j < item.size(); j++) {
				//cout << item[j].getContent() << endl;
				if (item[j].getContent() != "") {
					irtxt << item[j].getContent() << endl;
				}
			}
			//cout << "\n";
			irtxt << "\n";
		}
		irtxt.close();
	}
}

vector<vector<string>> stackVars;
vector<map<string, bool>> var2Arr;

void countVars() {
	stackVars.push_back(vector<string>());
	var2Arr.push_back(map<string, bool>());
	for (int i = 1; i < codetotal.size(); i++) {
		auto func = codetotal.at(i);
		vector<string> vars;
		map<string, bool> arr;
		for (int j = 1; j < func.size(); j++) {
			auto instr = func.at(j);
			if (instr.getCodetype() == ALLOC) {	
				vars.push_back(instr.getResult());
				if (instr.getOperand2() != "1") {	//局部数组才设置为true
					arr[instr.getResult()] = true;
				}
				else {
					arr[instr.getResult()] = false;
				}
			}
			else if (instr.getCodetype() == PARA) {
				vars.push_back(instr.getResult());
				arr[instr.getResult()] = false;
			}
		}
		stackVars.push_back(vars);
		var2Arr.push_back(arr);
	}
}

//检查irCode类型
// 1: 计算类型
int checkType(irCodeType op) {
	if (op == ADD ||  op == SUB || op == DIV || op == MUL ||
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
			else if (instr.getCodetype() == MOV && next.getCodetype() == MOV) {
				string instrOpe1 = instr.getOperand1();
				string nextOpe1 = next.getOperand1();
				string nextOpe2 = next.getOperand2();
				if (instrOpe1 == nextOpe2) {
					instr.setOperand1(nextOpe1);
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

bool isCondition(irCodeType op) {
	if (op == EQL || op == NEQ || op == SGT || op == SGE || op == SLT || op == SLE) {
		return true;
	}
	else {
		return false;
	}
}

irCodeType converse(irCodeType op) {
	if (op == EQL) {
		return NEQ;
	}
	else if (op == NEQ) {
		return EQL;
	}
	else if (op == SLT) {
		return SGE;
	}
	else if (op == SLE) {
		return SGT;
	}
	else if (op == SGT) {
		return SLE;
	}
	else if (op == SGE) {
		return SLT;
	}
	else {
		return op;
	}
}

//将beq、bne、slt、sle、sgt、sge转换为条件跳转
void convertCondition() {
	for (int i = 1; i < LIR.size(); i++) {
		vector<CodeItem>& func = LIR.at(i);
		for (int j = 0; j < func.size(); j++) {
			CodeItem& instr = func.at(j);
			if (instr.getCodetype() == BR && isVreg(instr.getOperand1())) {
				int k = j - 1;
				while (k > 0) {	//找到对应的条件
					if (func.at(k).getResult() == instr.getOperand1()) {
						break;
					}
					k--;
				}
				auto& prev = func.at(k);
				if (isCondition(prev.getCodetype())) {
					instr.setOperand1(prev.getOperand1());
					instr.setOperand2(prev.getOperand2());
					instr.setCodetype(converse(prev.getCodetype()));
					prev.setCodetype(NOTE);
					prev.setInstr("", "", "");
				}
				if (prev.getCodetype() == AND) {	//and
					instr.setOperand1(prev.getOperand1());
					instr.setOperand2(prev.getOperand2());
					instr.setCodetype(prev.getCodetype());
					prev.setCodetype(NOTE);
					prev.setInstr("", "", "");
				}
				if (prev.getCodetype() == OR) {	//or
					instr.setOperand1(prev.getOperand1());
					instr.setOperand2(prev.getOperand2());
					instr.setCodetype(prev.getCodetype());
					prev.setCodetype(NOTE);
					prev.setInstr("", "", "");
				}
				if (prev.getCodetype() == NOT) {	//not
					instr.setOperand1(prev.getOperand1());
					instr.setOperand2(prev.getOperand2());
					instr.setCodetype(prev.getCodetype());
					prev.setCodetype(NOTE);
					prev.setInstr("", "", "");
				}
			}
		}
	}
}


string get_Vreg_of_tmp(map<string, string>& container, string tmp, int funcNum) {
	if (!isTmp(tmp)) {	//不是%临时变量则返回原值
		if (tmp.find('^') != -1 && tmp.size() > 1 && tmp.at(0) == '%') {
			return tmp.substr(0, tmp.find('^'));
		}
		else {
			return tmp;
		}
	}
	
	if (container.find(tmp) != container.end()) {
		return container[tmp];
	}
	else {
		string res = FORMAT("VR{}", func2vrIndex.at(funcNum));
		func2vrIndex.at(funcNum)++;
		container[tmp] = res;
		return res;
	}
}

void code_getIn(vector<map<string, string>>& var2greg) {
	for (int k = 1; k < LIR.size(); k++) {
		auto var2gloReg = var2greg.at(k);
		auto tmpvar2codes = func2tmpvar2Codes.at(k);
		map<string, bool> tmpvar2back;
		for (auto one : tmpvar2codes) {
			//外提后的代码没有分配到寄存器
			if (var2gloReg.find(one.first) != var2gloReg.end() && var2gloReg[one.first] == "memory") {
				int flag = 0;	//是否回提的标志
				//看外提的代码时候有变量没有寄存器
				for (auto instr : one.second) {
					auto ope1 = instr.getOperand1();
					if (instr.getCodetype() == LOAD
						&& var2gloReg.find(ope1) != var2gloReg.end() && var2gloReg[ope1] != "memory") {
						flag++;
					}
				}
				if (one.second.size() > 5) {	//5条指令作为分界线
					flag++;
				}
				if (flag <= 1) {	//回提
					tmpvar2back[one.first] = true;
				}
				else {
					tmpvar2back[one.first] = false;
				}
			}
		}
		//删除对应的store和load
		auto& src = LIR.at(k);
		map<string, string> tmp2VRreg;
		map<string, string> vr2vr;
		for (int i = 0; i < src.size(); i++) {
			auto& instr = src.at(i);
			auto op = instr.getCodetype();
			auto res = instr.getResult();
			auto ope1 = instr.getOperand1();
			auto ope2 = instr.getOperand2();
			if (op == STORE && tmpvar2back.find(ope1) != tmpvar2back.end() && tmpvar2back[ope1] == true) {
				//暂时先用store区分删除的界限
				do {
					src.erase(src.begin() + i);
					i--;
				} while (src.at(i).getCodetype() != STORE);
			}
			else if (op == LOAD && tmpvar2back.find(ope1) != tmpvar2back.end() && tmpvar2back[ope1] == true) {
				string resback;
				for (auto instrTmp : tmpvar2codes[ope1]) {
					if (instrTmp.getCodetype() == STORE) {
						break;
					}
					auto resTmp = get_Vreg_of_tmp(tmp2VRreg, instrTmp.getResult(), k);
					auto ope1Tmp = get_Vreg_of_tmp(tmp2VRreg, instrTmp.getOperand1(), k);
					auto ope2Tmp = get_Vreg_of_tmp(tmp2VRreg, instrTmp.getOperand2(), k);
					instrTmp.setInstr(resTmp, ope1Tmp, ope2Tmp);
					resback = resTmp;
					src.insert(src.begin() + i, instrTmp);
					i++;
				}
				vr2vr[res] = resback;
				src.erase(src.begin() + i);
				i--;
			}
			else if (op == ALLOC && tmpvar2back.find(res) != tmpvar2back.end() && tmpvar2back[res] == true) {
				src.erase(src.begin() + i);
				i--;
			}
			else {
				if (vr2vr.find(res) != vr2vr.end()) src.at(i).setResult(vr2vr[res]);
				if (vr2vr.find(ope1) != vr2vr.end()) src.at(i).setOperand1(vr2vr[ope1]);
				if (vr2vr.find(ope2) != vr2vr.end()) src.at(i).setOperand2(vr2vr[ope2]);
			}
		}
		for (auto one : tmpvar2back) {
			if (one.second == true) {
				auto& fuhaobiao = total.at(k);
				for (auto it = fuhaobiao.begin(); it != fuhaobiao.end();) {
					if (it->getName() == one.first) {
						it = fuhaobiao.erase(it);
						continue;
					}
					it++;
				}
			}
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
	inlineArray = ssa.getInlineArrayName();
	
	try {
		//寄存器分配优化
		
 		MIR2LIRpass();
		printLIR("LIR.txt");
		countVars();
		
		//2. 图着色分配寄存器
		if (1) {
			codetotal = LIR;
			ssa.registerAllocation();
			//各个函数中变量名与寄存器的对应关系，在debug_reg.txt文件中可以见到输出	
			vector<map<string, string>> var2reg = ssa.getvar2reg();
			convertCondition();		//优化条件跳转
			//code_getIn(var2reg);  //不变式回提，将多余的且效果可能并不会很好的不变式回提
			printLIR("LIR2.txt");
			//图着色寄存器分配
			registerAllocation3(var2reg);
		}
		
		//1. 寄存器直接指派
		if (0) {
			convertCondition();		//优化条件跳转
			printLIR("LIR2.txt");
			registerAllocation();
		}

		printLIR("armIR.txt");
		//窥孔优化
		peepholeOptimization();
		printLIR("armIR_2.txt");
	}
	catch (exception e) {
		WARN_MSG("IrOptimize wrong!");
	}
}

