#include"armgenerate.h"
#include<iostream>
#include<vector>
#include<string>
#include<fstream>
#include<map>
#include<ctype.h>
#define OUTPUT(s) do{output_buffer.push_back(s);}while(0)
using namespace std;

vector<string> output_buffer;
ofstream arm("testcase.s");
int symbol_pointer;
map<string, int> var2addr;//记录函数内的绝对地址,结果减去sp得到相对偏移
int sp; //跟踪sp
int pushNum = 0;//记录传参顺序
string global_var_name;
int global_var_size = 1;
vector<string> ini_value;
bool is_global = true;
string reglist = "R4,R5,R6,R7,R8,R9,R10,R11,LR";

void global_flush()
{
	if (global_var_size == 1) {
		return;
	}
	OUTPUT(".data");
	string out = global_var_name + ": ";
	out += ".word ";
	for (string v : ini_value) {
		out += v + ",";
	}
	out = out.substr(0, out.size()-1);
	for (int i = ini_value.size(); i < global_var_size; i++) {
		out += ",0";
	}
	OUTPUT(out);
	OUTPUT(".text");
	ini_value.clear();
}

string getname(string ir_name)
{
	if (ir_name[0] == '@') {
		return ir_name.substr(1);
	}
}

pair<string, int> get_location(string name)
{
	if (name[0] == '@') {
		return { getname(name),-1 };
	}else if (isdigit(name[1]) && name[0] == '%') {
		return { "R" + to_string(stoi(name.substr(1))%7 + 4),-2 }; //此处应当模8,留出r11做临时寄存器
	}else if (var2addr.find(name) != var2addr.end()) {
		if (var2addr[name] > 0) {
			return { "R" + to_string(var2addr[name] - 1),-2 };
		}
		return { "",var2addr[name] };
	}
	else {
		return { "#" + name,-3 };
	}
}

void _global(CodeItem* ir)
{
	global_flush();
	string name = getname(ir->getResult());
	string ini_value = ir->getOperand1();
	int size = stoi(ir->getOperand2());
	global_var_size = size;
	if (size > 1) {
		global_var_name = name;
		//OUTPUT(name + ": .zero " + to_string(size * 4));
	}
	else {
		OUTPUT(".data");
		if (ini_value == "_") {
			OUTPUT(name + ": .zero 4");
		}
		else {
			OUTPUT(name + ": .word " + ini_value);
		}
		OUTPUT(".text");
	}
}

void _define(CodeItem* ir)
{
	string name = getname(ir->getResult());
	OUTPUT(name + ":");
	var2addr.clear();
	int paraNum;
	int paraIndex=0;
	int sp_without_para;
	sp = 0;
	for (symbolTable st : total[symbol_pointer]) {
		formType form = st.getForm();
		switch (form)
		{
		case CONSTANT:
		case VARIABLE: {
			int elem = st.getvarLength();
			sp -= elem * 4;
			var2addr[st.getName()] = sp;
			break;
		}
		case PARAMETER: {
			paraIndex++;
			/*
			if (paraIndex > 4) {
				var2addr[st.getName] = sp + 4 * (paraIndex - 5);
			}*/
			if (paraIndex < 5) {
				var2addr[st.getName()] = paraIndex;
			}
			else {
				var2addr[st.getName()] = sp + 4 * (paraIndex - 5);//当值大于等于零时表示在寄存器里
			}
			break;
		}
		case FUNCTION: {
			paraNum = st.getparaLength();
			if (paraNum > 4) {
				sp -= (paraNum - 4) * 4;
			}
			sp_without_para = sp;
			break;
		}
		default:
			break;
		}
	}
	OUTPUT("ADD SP,SP,#" + to_string(sp - sp_without_para));
}

void _alloc(CodeItem* ir)
{
	string name = ir->getResult();
	string ini_value = ir->getOperand1();
	int size = stoi(ir->getOperand2());
	if (ini_value != "") {
		OUTPUT("LDR R12,=" + ini_value);
		OUTPUT("STR R12,[SP,#" + to_string(get_location(name).second - sp)+"]");
	}
}

void _load(CodeItem* ir)
{
	string target = ir->getResult();
	string var = ir->getOperand1();
	string reg = get_location(target).first;
	auto p = get_location(var);
	if (p.second == -1) {
		OUTPUT("LDR R12,=" + p.first);
		OUTPUT("LDR " + reg + ",[R12,#0]");
	}
	else if (p.second == -2) {
		OUTPUT("MOV " + reg + "," + p.first);
	}
	else {
		OUTPUT("LDR " + reg + ",[SP,#" + to_string(p.second - sp) + "]");
	}
}

void _loadarr(CodeItem* ir)
{
	string target = ir->getResult();
	string var = ir->getOperand1();
	string offset = ir->getOperand2();
	string reg = get_location(target).first;
	auto p = get_location(var);
	if (offset[0] == '%') {
		if (p.second == -1) {
			OUTPUT("LDR R12,=" + p.first);
			OUTPUT("ADD R12,R12," + get_location(offset).first);
			OUTPUT("LDR " + reg + ",[R12]");
		}
		else if (p.second == -2) {
			OUTPUT("ADD R12," + p.first + "," + get_location(offset).first);
			OUTPUT("LDR " + reg + ",[R12]");
		}
		else {
			OUTPUT("LDR " + reg + ",[SP,#" + to_string(p.second - sp) + "]");
			OUTPUT("ADD " + reg + "," + reg + "," + get_location(offset).first);
			OUTPUT("LDR " + reg + ",[" + reg + "]");
		}
	}
	else if (offset[0] == '@') {
		if (p.second == -1) {
			OUTPUT("LDR R12,=" + p.first);
			OUTPUT("LDR R11,=" + get_location(offset).first);
			OUTPUT("LDR R11,[R11]");
			OUTPUT("ADD R12,R12,R11");
			OUTPUT("LDR " + reg + ",[R12]");
		}
		else if (p.second == -2) {
			OUTPUT("LDR R12,=" + get_location(offset).first);
			OUTPUT("LDR R12,[R12]");
			OUTPUT("ADD R12," + p.first + ",R12");
			OUTPUT("LDR " + reg + ",[R12]");
		}
		else {
			OUTPUT("LDR R12,=" + get_location(offset).first);
			OUTPUT("LDR R12,[R12]");
			OUTPUT("LDR " + reg + ",[SP,#" + to_string(p.second - sp) + "]");
			OUTPUT("ADD " + reg + "," + reg + ",R12");
			OUTPUT("LDR " + reg + ",[" + reg + "]");
		}
	}
	else {
		int off = stoi(ir->getOperand2());
		if (p.second == -1) {
			OUTPUT("LDR R12,=" + p.first);
			OUTPUT("LDR " + reg + ",[R12,#" + to_string(off) + "]");
		}
		else if (p.second == -2) {
			OUTPUT("LDR " + reg + ",[" + p.first + ",#" + to_string(off) + "]");
		}
		else {
			OUTPUT("LDR " + reg + ",[SP,#" + to_string(p.second - sp) + "]");
			OUTPUT("LDR " + reg + ",[" + reg + "#" + to_string(off) + "]");
		}
	}
}

void _store(CodeItem* ir)
{
	string target = ir->getResult();
	string var = ir->getOperand1();
	string reg = get_location(target).first;
	auto p = get_location(var);
	if (p.second == -1) {
		OUTPUT("LDR R12,=" + p.first);
		if (reg[0] == '#') {
			OUTPUT("LDR R11,=" + reg.substr(1));
			OUTPUT("STR R11,[R12,#0]");
		}
		else {
			OUTPUT("STR " + reg + ",[R12,#0]");
		}
	}
	else if (p.second == -2) {
		OUTPUT("MOV " + p.first + "," + reg);
	}
	else {
		if (reg[0] == '#'){
			OUTPUT("LDR R11,=" + reg.substr(1));
			OUTPUT("STR R11,[SP,#" + to_string(p.second - sp) + "]");
		}
		else {
			OUTPUT("STR " + reg + ",[SP,#" + to_string(p.second - sp) + "]");
		}
	}
}

void _storearr(CodeItem* ir)
{
	if (is_global) {
		string value = ir->getResult();
		ini_value.push_back(value);
		return;
	}
	string target = ir->getResult();
	string var = ir->getOperand1();
	string offset = ir->getOperand2();
	int off;
	string reg = get_location(target).first;
	auto p = get_location(var);
	if (offset[0] == '%') {
		if (p.second == -1) {
			OUTPUT("LDR R12,=" + p.first);
			OUTPUT("ADD R12,R12," + get_location(offset).first);
			OUTPUT("STR " + reg + ",[R12]");
		}
		else if (p.second == -2) {
			OUTPUT("ADD R12," + p.first + "," + get_location(offset).first);
			OUTPUT("STR " + reg + ",[R12]");
		}
		else {
			OUTPUT("LDR R12,[SP,#" + to_string(p.second - sp) + "]");
			OUTPUT("ADD R12,R12," + get_location(offset).first);
			OUTPUT("STR " + reg + ",[R12]");
		}
	}
	else if (offset[0] == '@') {
		if (p.second == -1) {
			OUTPUT("LDR R12,=" + p.first);
			OUTPUT("LDR R11,=" + get_location(offset).first);
			OUTPUT("LDR R11,[R11]");
			OUTPUT("ADD R12,R12,R11");
			OUTPUT("STR " + reg + ",[R12]");
		}
		else if (p.second == -2) {
			OUTPUT("LDR R12,=" + get_location(offset).first);
			OUTPUT("LDR R12,[R12]");
			OUTPUT("ADD R12," + p.first + ",R12");
			OUTPUT("STR " + reg + ",[R12]");
		}
		else {
			OUTPUT("LDR R11,=" + get_location(offset).first);
			OUTPUT("LDR R11,[R11]");
			OUTPUT("LDR R12,[SP,#" + to_string(p.second - sp) + "]");
			OUTPUT("ADD R12,R12,R11");
			OUTPUT("STR " + reg + ",[R12]");
		}
	}
	else {
		off = stoi(offset);
		if (p.second == -1) {
			OUTPUT("LDR R12,=" + p.first);
			OUTPUT("STR " + reg + ",[R12,#" + to_string(off) + "]");
		}
		else if (p.second == -2) {
			OUTPUT("STR " + reg + ",[" + p.first + ",#" + to_string(off) + "]");
		}
		else {
			OUTPUT("LDR R12,[SP,#" + to_string(p.second - sp) + "]");
			OUTPUT("STR " + reg + "[R12,#" + to_string(p.second - sp) + "]");
		}
	}
}

void _add(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	string reg0 = get_location(target).first;
	string reg1;
	string reg2;
	if (op1[0] != '%') {
		OUTPUT("LDR R12,=" + op1);
		reg1 = "R12";
	}
	else {
		reg1 = get_location(op1).first;
	}
	if (op2[0] != '%') {
		OUTPUT("LDR R12,=" + op2);
		reg2 = "R12";
	}
	else {
		reg2 = get_location(op2).first;
	}
	OUTPUT("ADD " + reg0 + "," + reg1 + "," + reg2);
}

void _sub(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	string reg0 = get_location(target).first;
	string reg1;
	string reg2;
	if (op1[0] != '%') {
		OUTPUT("LDR R12,=" + op1);
		reg1 = "R12";
	}
	else {
		reg1 = get_location(op1).first;
	}
	if (op2[0] != '%') {
		OUTPUT("LDR R12,=" + op2);
		reg2 = "R12";
	}
	else {
		reg2 = get_location(op2).first;
	}
	OUTPUT("SUB " + reg0 + "," + reg1 + "," + reg2);
}

void _mul(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	string reg0 = get_location(target).first;
	string reg1;
	string reg2;
	if (op1[0] != '%') {
		OUTPUT("LDR R12,=" + op1);
		reg1 = "R12";
	}
	else {
		reg1 = get_location(op1).first;
	}
	if (op2[0] != '%') {
		OUTPUT("LDR R12,=" + op2);
		reg2 = "R12";
	}
	else {
		reg2 = get_location(op2).first;
	}
	OUTPUT("MUL " + reg0 + "," + reg1 + "," + reg2);//效果待考察
}

void _div(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	string reg0 = get_location(target).first;
	string reg1;
	string reg2;
	if (op1[0] != '%') {
		OUTPUT("LDR R12,=" + op1);
		reg1 = "R12";
	}
	else {
		reg1 = get_location(op1).first;
	}
	if (op2[0] != '%') {
		OUTPUT("LDR R12,=" + op2);
		reg2 = "R12";
	}
	else {
		reg2 = get_location(op2).first;
	}
	OUTPUT("SDIV " + reg0 + "," + reg1 + "," + reg2); //效果待考察
}

void _rem(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	string reg0 = get_location(target).first;
	string reg1;
	string reg2;
	if (op1[0] != '%') {
		OUTPUT("LDR R12,=" + op1);
		reg1 = "R12";
	}
	else {
		reg1 = get_location(op1).first;
	}
	if (op2[0] != '%') {
		OUTPUT("LDR R12,=" + op2);
		reg2 = "R12";
	}
	else {
		reg2 = get_location(op2).first;
	}
	OUTPUT("SDIV " + reg0 + "," + reg1 + "," + reg2);
	OUTPUT("MUL " + reg0 + "," + reg0 + "," + reg2);
	OUTPUT("SUB " + reg0 + "," + reg1 + "," + reg0);//效果待考察
}

void _and(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	string reg0 = get_location(target).first;
	string reg1;
	string reg2;
	if (op1[0] != '%') {
		OUTPUT("LDR R12,=" + op1);
		reg1 = "R12";
	}
	else {
		reg1 = get_location(op1).first;
	}
	if (op2[0] != '%') {
		OUTPUT("LDR R12,=" + op2);
		reg2 = "R12";
	}
	else {
		reg2 = get_location(op2).first;
	}
	OUTPUT("AND " + reg0 + "," + reg1 + "," + reg2);
}

void _or(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	string reg0 = get_location(target).first;
	string reg1;
	string reg2;
	if (op1[0] != '%') {
		OUTPUT("LDR R12,=" + op1);
		reg1 = "R12";
	}
	else {
		reg1 = get_location(op1).first;
	}
	if (op2[0] != '%') {
		OUTPUT("LDR R12,=" + op2);
		reg2 = "R12";
	}
	else {
		reg2 = get_location(op2).first;
	}
	OUTPUT("OR " + reg0 + "," + reg1 + "," + reg2);
}

void _not(CodeItem* ir)
{

}

void _eql(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	string reg0 = get_location(target).first;
	string reg1;
	string reg2;
	if (op1[0] != '%') {
		OUTPUT("LDR R12,=" + op1);
		reg1 = "R12";
	}
	else {
		reg1 = get_location(op1).first;
	}
	if (op2[0] != '%') {
		OUTPUT("LDR R12,=" + op2);
		reg2 = "R12";
	}
	else {
		reg2 = get_location(op2).first;
	}
	OUTPUT("CMP " + reg1 + "," + reg2);
	OUTPUT("MOVEQ " + reg0 + ",#1");
	OUTPUT("MOVNE " + reg0 + ",#0");
}

void _neq(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	string reg0 = get_location(target).first;
	string reg1;
	string reg2;
	if (op1[0] != '%') {
		OUTPUT("LDR R12,=" + op1);
		reg1 = "R12";
	}
	else {
		reg1 = get_location(op1).first;
	}
	if (op2[0] != '%') {
		OUTPUT("LDR R12,=" + op2);
		reg2 = "R12";
	}
	else {
		reg2 = get_location(op2).first;
	}
	OUTPUT("CMP " + reg1 + "," + reg2);
	OUTPUT("MOVNE " + reg0 + ",#1");
	OUTPUT("MOVEQ " + reg0 + ",#0");
}

void _sgt(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	string reg0 = get_location(target).first;
	string reg1;
	string reg2;
	if (op1[0] != '%') {
		OUTPUT("LDR R12,=" + op1);
		reg1 = "R12";
	}
	else {
		reg1 = get_location(op1).first;
	}
	if (op2[0] != '%') {
		OUTPUT("LDR R12,=" + op2);
		reg2 = "R12";
	}
	else {
		reg2 = get_location(op2).first;
	}
	OUTPUT("CMP " + reg1 + "," + reg2);
	OUTPUT("MOVGT " + reg0 + ",#1");
	OUTPUT("MOVLE " + reg0 + ",#0");
}

void _sge(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	string reg0 = get_location(target).first;
	string reg1;
	string reg2;
	if (op1[0] != '%') {
		OUTPUT("LDR R12,=" + op1);
		reg1 = "R12";
	}
	else {
		reg1 = get_location(op1).first;
	}
	if (op2[0] != '%') {
		OUTPUT("LDR R12,=" + op2);
		reg2 = "R12";
	}
	else {
		reg2 = get_location(op2).first;
	}
	OUTPUT("CMP " + reg1 + "," + reg2);
	OUTPUT("MOVGE " + reg0 + ",#1");
	OUTPUT("MOVLT " + reg0 + ",#0");
}

void _slt(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	string reg0 = get_location(target).first;
	string reg1;
	string reg2;
	if (op1[0] != '%') {
		OUTPUT("LDR R12,=" + op1);
		reg1 = "R12";
	}
	else {
		reg1 = get_location(op1).first;
	}
	if (op2[0] != '%') {
		OUTPUT("LDR R12,=" + op2);
		reg2 = "R12";
	}
	else {
		reg2 = get_location(op2).first;
	}
	OUTPUT("CMP " + reg1 + "," + reg2);
	OUTPUT("MOVLT " + reg0 + ",#1");
	OUTPUT("MOVGE " + reg0 + ",#0");
}

void _sle(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	string reg0 = get_location(target).first;
	string reg1;
	string reg2;
	if (op1[0] != '%') {
		OUTPUT("LDR R12,=" + op1);
		reg1 = "R12";
	}
	else {
		reg1 = get_location(op1).first;
	}
	if (op2[0] != '%') {
		OUTPUT("LDR R12,=" + op2);
		reg2 = "R12";
	}
	else {
		reg2 = get_location(op2).first;
	}
	OUTPUT("CMP " + reg1 + "," + reg2);
	OUTPUT("MOVLE " + reg0 + ",#1");
	OUTPUT("MOVGT " + reg0 + ",#0");
}

void _label(CodeItem* ir)
{
	OUTPUT(ir->getResult().substr(1) + ":");
}

void _br(CodeItem* ir)
{
	string res = ir->getResult();
	if (res[0] == '%' && isdigit(res[1])) {
		string label1 = ir->getOperand1().substr(1);
		string label2 = ir->getOperand2().substr(1);
		string reg = get_location(res).first;
		OUTPUT("CMP " + reg + ",#0");
		OUTPUT("BEQ " + label2);
		OUTPUT("BNE " + label1);
	}
	else {
		OUTPUT("B " + res.substr(1));
	}
}

void _push(CodeItem* ir)
{
	pushNum++;//可以去掉？
	string name = ir->getResult();
	pushNum = stoi(ir->getOperand1());
	string type = ir->getOperand2();
	if (pushNum == 1) {
		OUTPUT("PUSH {" + reglist + "}");
		sp -= 36;
	}
	if (pushNum <= 4) {
		auto p = get_location(name);
		if (type == "int") {
			if (p.second == -1) {
				OUTPUT("LDR R12,=" + p.first);
				OUTPUT("LDR R"+ to_string(pushNum - 1)+",[R12,#0]");
			}
			else if (p.second == -2) {
				OUTPUT("MOV R" + to_string(pushNum - 1) + "," + p.first);
			}
			else {
				OUTPUT("LDR R" + to_string(pushNum - 1) + "[SP,#" + to_string(p.second - sp) + "]");
			}
		}
		else {
			if (p.second == -1) {
				OUTPUT("LDR R" + to_string(pushNum - 1) + ",="+p.first);
			}
			else if (p.second == -2) {
				OUTPUT("MOV R" + to_string(pushNum - 1) + "," + p.first);
			}
			else {
				OUTPUT("ADD R" + to_string(pushNum - 1) + "SP,#" + to_string(p.second - sp));
			}
		}
	}
	else {
		auto p = get_location(name);
		if (type == "int") {
			if (p.second == -1) {
				OUTPUT("LDR R12,=" + p.first);
				OUTPUT("LDR R12,[R12,#0]");
				OUTPUT("PUSH {R12}");
			}
			else if (p.second == -2) {
				OUTPUT("PUSH {" + p.first + "}");
			}
			else {
				OUTPUT("LDR R12,[SP,#" + to_string(p.second - sp) + "]");
				OUTPUT("PUSH {R12}");
			}
		}
		else {
			if (p.second == -1) {
				OUTPUT("LDR R12,=" + p.first);
				OUTPUT("PUSH {R12}");
			}
			else if (p.second == -2) {
				OUTPUT("PUSH {" + p.first + "}");
			}
			else {
				OUTPUT("ADD R12,SP,#" + to_string(p.second - sp));
				OUTPUT("PUSH {R12}");
			}
		}
		sp -= 4;
	}
}

void _call(CodeItem* ir)
{
	string funcname = getname(ir->getResult());
	string retreg = ir->getOperand1();
	OUTPUT("BL " + funcname);
	OUTPUT("POP {" + reglist + "}");
	sp += 36;
	OUTPUT("MOV " + get_location(retreg).first + ",R0");
}

void _ret(CodeItem* ir)
{
	string re = ir->getResult();
	string type = ir->getOperand1();
	if (type == "int") {
		auto p = get_location(re);
		if (p.second == -1) {
			OUTPUT("LDR R0,=" + p.first);
			OUTPUT("LDR R0,[R0,#0]");
		}
		else if (p.second == -2) {
			OUTPUT("MOV R0," + p.first);
		}
		else {
			OUTPUT("LDR R0,[SP,#" + to_string(p.second - sp) + "]");
		}
	}
	OUTPUT("ADD SP,SP,#" + to_string(-sp));
	OUTPUT("MOV PC,LR");
}

void arm_generate_without_register()
{
	for (int i = 0; i < codetotal.size(); i++) {
		for (int j = 0; j < codetotal[i].size(); j++) {
			CodeItem* ir_now = &codetotal[i][j];
			switch (ir_now->getCodetype())
			{
			case ADD:
				_add(ir_now);
				break;
			case SUB:
				_sub(ir_now);
				break;
			case DIV:
				_div(ir_now);
				break;
			case MUL:
				_mul(ir_now);
				break;
			case REM:
				_rem(ir_now);
				break;
			case AND:
				_and(ir_now);
				break;
			case OR:
				_or(ir_now);
				break;
			case NOT:
				_not(ir_now);
				break;
			case EQL:
				_eql(ir_now);
				break;
			case NEQ:
				_neq(ir_now);
				break;
			case SGT:
				_sgt(ir_now);
				break;
			case SGE:
				_sge(ir_now);
				break;
			case SLT:
				_slt(ir_now);
				break;
			case SLE:
				_sle(ir_now);
				break;
			case ALLOC:
				_alloc(ir_now);
				break;
			case STORE:
				_store(ir_now);
				break;
			case STOREARR:
				_storearr(ir_now);
				break;
			case LOAD:
				_load(ir_now);
				break;
			case LOADARR:
				_loadarr(ir_now);
				break;
			case INDEX:
				break;
			case CALL:
				_call(ir_now);
				break;
			case RET:
				_ret(ir_now);
				break;
			case PUSH:
				_push(ir_now);
				break;
			case POP:
				break;
			case LABEL:
				_label(ir_now);
				break;
			case BR:
				_br(ir_now);
				break;
			case DEFINE: {
				if (is_global) {
					global_flush();
					is_global = false;
				}
				symbol_pointer = i;
				_define(ir_now);
				break;
			}
			case PARA:
				break;
			case GLOBAL:
				_global(ir_now);
				break;
			default:
				break;
			}
		}
	}
	arm << ".global main\n";
	for (auto s : output_buffer) {
		arm << s << "\n";
	}
}

