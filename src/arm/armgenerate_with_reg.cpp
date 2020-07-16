#include"armgenerate.h"
#include<iostream>
#include<vector>
#include<string>
#include<fstream>
#include<map>
#include<ctype.h>
#include<set>
#define OUTPUT(s) do{output_buffer.push_back(s);}while(0)
using namespace std;

vector<string> output_buffer;
int symbol_pointer;
map<string, int> var2addr;//记录函数内的绝对地址,结果减去sp得到相对偏移
int sp; //跟踪sp
int pushNum = 0;//记录传参顺序
string global_var_name;
int global_var_size = 1;
vector<string> ini_value;
bool is_global = true;
string reglist = "R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,LR,R0";
string reglist_without0 = "R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,LR";

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
	out = out.substr(0, out.size() - 1);
	for (int i = ini_value.size(); i < global_var_size; i++) {
		out += ",0";
	}
	OUTPUT(out);
	OUTPUT(".text");
	ini_value.clear();
}

string get_cmp_label(string logistic) {
	static int cnt = 0;
	return logistic + ".L" + to_string(cnt++);
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
	}
	else if (isdigit(name[1]) && name[0] == '%') {
		return { "R" + to_string(stoi(name.substr(1)) % 7 + 4),-2 }; //此处应当模8,留出r11做临时寄存器
	}
	else if (var2addr.find(name) != var2addr.end()) {
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
		if (ini_value == "") {
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
	int paraIndex = 0;
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
		OUTPUT("STR R12,[SP,#" + to_string(get_location(name).second - sp) + "]");
	}
}

void _load(CodeItem* ir)
{
	string target = ir->getResult();
	string var = ir->getOperand1();
	if (var[0] == 'R') {
		OUTPUT("LDR " + target + ",[" + var + "]");
	}
	else {
		auto p = get_location(var);
		OUTPUT("LDR " + target + ",[SP,#" + to_string(p.second-sp) + "]");
	}
}

void _loadarr(CodeItem* ir)
{
	string target = ir->getResult();
	string var = ir->getOperand1();
	string offset = ir->getOperand2();
	if (offset[0] == 'R') {
		if (var[0] == 'R') {
			OUTPUT("LDR " + target + ",[" + var + "," + offset + "]");
		}
		else {
			auto p = get_location(var);
			OUTPUT("ADD " + offset + "," + offset + ",SP");
			OUTPUT("ADD " + offset + "," + offset + ",#" + to_string(p.second - sp));
			OUTPUT("LDR " + target + ",[" + offset + "]");
		}
	}
	else {
		if (var[0] == 'R') {
			OUTPUT("LDR " + target + ",[" + var + ",#" + offset + "]");
		}
		else {
			auto p = get_location(var);
			OUTPUT("LDR " + target + ",[SP,#" + to_string(p.second - sp + stoi(offset)) + "]");
		}
	}
}

void _store(CodeItem* ir)
{
	string value = ir->getResult();
	string loca = ir->getOperand1();
	if (loca[0] == 'R') {
		OUTPUT("STR " + value + ",[" + loca + "]");
	}
	else {
		auto p = get_location(loca);
		OUTPUT("STR " + value + ",[SP,#" + to_string(p.second - sp) + "]");
	}
}

void _storearr(CodeItem* ir)
{
	if (is_global) {
		string value = ir->getResult();
		ini_value.push_back(value);
		return;
	}
	string value = ir->getResult();
	string var = ir->getOperand1();
	string offset = ir->getOperand2();
	if (offset[0] == 'R') {
		if (var[0] == 'R') {
			OUTPUT("STR " + value + ",[" + var + "," + offset + "]");
		}
		else {
			auto p = get_location(var);
			OUTPUT("ADD " + offset + ",SP," + offset);
			OUTPUT("ADD " + offset + "," + offset + ",#" + to_string(p.second - sp));
			OUTPUT("STR " + value + ",[" + offset + "]");
		}
	}
	else {
		if (var[0] == 'R') {
			OUTPUT("STR " + value + ",[" + var + ",#" + offset + "]");
		}
		else {
			auto p = get_location(var);
			OUTPUT("STR " + value + ",[SP,#" + to_string(p.second - sp + stoi(offset)) + "]");
		}
	}
}

void _add(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op2[0] != 'R') {
		op2 = "#" + op2;
	}
	OUTPUT("ADD " + target + "," + op1 + "," + op2);
}

void _sub(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op2[0] != 'R') {
		op2 = "#" + op2;
	}
	OUTPUT("SUB " + target + "," + op1 + "," + op2);
}

void _mul(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op2[0] != 'R') {
		op2 = "#" + op2;
	}
	OUTPUT("MUL " + target + "," + op1 + "," + op2);
}

void _div(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	OUTPUT("PUSH {R0}");
	OUTPUT("PUSH {R1,R2,R3,R12,LR}");
	OUTPUT("MOV R0," + op1);
	if (op2[0] == 'R') {
		OUTPUT("MOV R1," + op2);
	}
	else {
		OUTPUT("MOV R1,#" + op2);
	}
	OUTPUT("BL __aeabi_idiv");
	if (target == "R0") {
		OUTPUT("POP {R1,R2,R3,R12,LR}");
		OUTPUT("ADD SP,SP,#4");
	}
	else {
		OUTPUT("POP {R1,R2,R3,R12,LR}");
		OUTPUT("MOV " + target + ",R0");
		OUTPUT("POP {R0}");
	}
}

void _rem(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	OUTPUT("PUSH {R1}");
	OUTPUT("PUSH {R0,R2,R3,R12,LR}");
	OUTPUT("MOV R0," + op1);
	if (op2[0] == 'R') {
		OUTPUT("MOV R1," + op2);
	}
	else {
		OUTPUT("MOV R1,#" + op2);
	}
	OUTPUT("BL __aeabi_idivmod");
	if (target == "R1") {
		OUTPUT("POP {R0,R2,R3,R12,LR}");
		OUTPUT("ADD SP,SP,#4");
	}
	else {
		OUTPUT("POP {R0,R2,R3,R12,LR}");
		OUTPUT("MOV " + target + ",R1");
		OUTPUT("POP {R1}");
	}
}

void _and(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op2[0] != 'R') {
		int im = stoi(op2);
		if (im == 0) {
			OUTPUT("LDR " + target + ",=0");
		}
		else {
			OUTPUT("CMP " + op1 + ",#0");
			OUTPUT("LDREQ " + target + ",=0");
			OUTPUT("LDRNE " + target + ",=1");
		}
	}
	else {
		string L1 = get_cmp_label("AND");
		string L2 = get_cmp_label("AND");
		OUTPUT("CMP " + op1 + ",#0");
		OUTPUT("BEQ " + L1);
		OUTPUT("CMP " + op2 + ",#0");
		OUTPUT("BEQ " + L1);
		OUTPUT("LDR " + target + ",=1");
		OUTPUT("B " + L2);
		OUTPUT(L1 + ":");
		OUTPUT("LDR " + target + ",=0");
		OUTPUT(L2 + ":");
	}
}

void _or(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op2[0] != 'R') {
		int im = stoi(op2);
		if (im != 0) {
			OUTPUT("LDR " + target + ",=1");
		}
		else {
			OUTPUT("CMP " + op1 + ",#0");
			OUTPUT("LDREQ " + target + ",=0");
			OUTPUT("LDRNE " + target + ",=1");
		}
	}
	else {
		string L1 = get_cmp_label("OR");
		string L2 = get_cmp_label("OR");
		OUTPUT("CMP " + op1 + ",#0");
		OUTPUT("BNE " + L1);
		OUTPUT("CMP " + op2 + ",#0");
		OUTPUT("BNE " + L1);
		OUTPUT("LDR " + target + ",=0");
		OUTPUT("B " + L2);
		OUTPUT(L1 + ":");
		OUTPUT("LDR " + target + ",=1");
		OUTPUT(L2 + ":");
	}
}

void _not(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	OUTPUT("CMP " + op1 + ",#0");
	OUTPUT("LDREQ " + target + ",=1");
	OUTPUT("LDRNE " + target + ",=0");
}

void _eql(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op2[0] != 'R') {
		op2 = "#" + op2;
	}
	OUTPUT("CMP " + op1 + "," + op2);
	OUTPUT("MOVEQ " + target + ",#1");
	OUTPUT("MOVNE " + target + ",#0");
}

void _neq(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op2[0] != 'R') {
		op2 = "#" + op2;
	}
	OUTPUT("CMP " + op1 + "," + op2);
	OUTPUT("MOVNE " + target + ",#1");
	OUTPUT("MOVEQ " + target + ",#0");
}

void _sgt(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op2[0] != 'R') {
		op2 = "#" + op2;
	}
	OUTPUT("CMP " + op1 + "," + op2);
	OUTPUT("MOVGT " + target + ",#1");
	OUTPUT("MOVLE " + target + ",#0");
}

void _sge(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op2[0] != 'R') {
		op2 = "#" + op2;
	}
	OUTPUT("CMP " + op1 + "," + op2);
	OUTPUT("MOVGE " + target + ",#1");
	OUTPUT("MOVLT " + target + ",#0");
}

void _slt(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op2[0] != 'R') {
		op2 = "#" + op2;
	}
	OUTPUT("CMP " + op1 + "," + op2);
	OUTPUT("MOVLT " + target + ",#1");
	OUTPUT("MOVGE " + target + ",#0");
}

void _sle(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op2[0] != 'R') {
		op2 = "#" + op2;
	}
	OUTPUT("CMP " + op1 + "," + op2);
	OUTPUT("MOVLE " + target + ",#1");
	OUTPUT("MOVGT " + target + ",#0");
}

void _label(CodeItem* ir)
{
	OUTPUT(ir->getResult().substr(1) + ":");
}

void _br(CodeItem* ir)
{
	string res = ir->getOperand1();
	if (res[0] != '%') {
		string label1 = ir->getOperand2().substr(1);
		string label2 = ir->getResult().substr(1);
		OUTPUT("CMP " + res + ",#0");
		OUTPUT("BEQ " + label2);
		OUTPUT("BNE " + label1);
	}
	else {
		OUTPUT("B " + res.substr(1));
	}
}

void _push(CodeItem* ir)
{
	string type = ir->getResult();
	if (type == "") {
		string reg = ir->getOperand1();
		OUTPUT("PUSH {" + reg + "}");
		sp -= 4;
		return;
	}
	string name = ir->getOperand1();
	pushNum = stoi(ir->getOperand2());
	/*
	if (pushNum == 1) {
		OUTPUT("PUSH {" + reglist + "}");
		sp -= 56;
	}*/
	if (pushNum <= 4) {
		OUTPUT("MOV R" + to_string(pushNum - 1) + "," + name);
	}
	else {
		OUTPUT("PUSH {" + name + "}");
		sp -= 4;
	}
}

void _call(CodeItem* ir)
{
	string funcname = getname(ir->getOperand1());
	//string retreg = ir->getOperand1();
	OUTPUT("BL " + funcname);
}

void _ret(CodeItem* ir)
{
	string re = ir->getOperand1();
	string type = ir->getOperand2();
	if (type == "int") {
		if (re[0] == 'R') {
			OUTPUT("MOV R0," + re);
		}
		else {
			auto p = get_location(re);
			if (p.second == -1) {
				OUTPUT("LDR R0,=" + p.first);
				OUTPUT("LDR R0,[R0]");
			}
			else if (p.second == -3) {
				OUTPUT("MOV R0," + p.first);
			}
			else {
				OUTPUT("LDR R0,[SP,#" + to_string(p.second - sp) + "]");
			}
		}
	}
	OUTPUT("ADD SP,SP,#" + to_string(-sp));
	OUTPUT("MOV PC,LR");
}

void _mov(CodeItem* ir) {
	string dst = ir->getOperand1();
	string src = ir->getOperand2();
	if (src[0] == 'R') {
		OUTPUT("MOV " + dst + "," + src);
	}
	else if (src[0] == '\"') {
		string slabel = get_cmp_label("STRING");
		OUTPUT(".data");
		OUTPUT(slabel + ": .ascii " + src);
		OUTPUT(".text");
		OUTPUT("LDR " + dst + ",=" + slabel);
	}
	else {
		OUTPUT("MOV " + dst + ",#" + src);
	}
}

void _lea(CodeItem* ir) {
	string reg = ir->getOperand1();
	string loca = ir->getOperand2();
	auto p = get_location(loca);
	if (p.second == -1) {
		OUTPUT("LDR " + reg + ",=" + p.first);
	}
	else {
		OUTPUT("ADD " + reg + ",SP,#" + to_string(p.second - sp));
	}
}

void _getreg(CodeItem* ir) {
	string ret = ir->getOperand1();
	if (ret == "R0") {
		OUTPUT("POP {" + reglist_without0 + "}");
		sp += 52;
		OUTPUT("ADD SP,SP,#4");
		sp += 4;
	}
	else if (ret == "") {
		OUTPUT("POP {" + reglist_without0 + "}");
		OUTPUT("POP {R0}");
		sp += 56;
	}
	else {
		OUTPUT("POP {" + reglist_without0 + "}");
		sp += 52;
		OUTPUT("MOV " + ret + ",R0");
		OUTPUT("POP {R0}");
		sp += 4;
	}
}

void _note(CodeItem* ir) {
	string status = ir->getOperand2();
	string note = ir->getOperand1();
	if (status == "begin" && note == "func") {
		OUTPUT("PUSH {R0}");
		OUTPUT("PUSH {" + reglist_without0 + "}");
		sp -= 56;
	}
}

void arm_generate(string sname)
{
	ofstream arm(sname);
	for (int i = 0; i < LIR.size(); i++) {
		for (int j = 0; j < LIR[i].size(); j++) {
			CodeItem* ir_now = &LIR[i][j];
			switch (ir_now->getCodetype())
			{
			case ADD:
				_add(ir_now);
				break;
			case SUB:
				_sub(ir_now);
				break;
			case DIV:
				_div(ir_now); //待定
				break;
			case MUL:
				_mul(ir_now);
				break;
			case REM:
				_rem(ir_now); //待定
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
				//_alloc(ir_now); //啥都不用干？
				break;
			case STORE:
				_store(ir_now); //全局怎么搞？
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
				_call(ir_now); //R0覆盖？
				break;
			case RET:
				_ret(ir_now); //RET怎么搞
				break;
			case PUSH:
				_push(ir_now); //前四个问题，传参顺序问题。
				break;
			case POP:
				break;//所以POP到底干嘛
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
			case MOV:
				_mov(ir_now);
				break;
			case LEA:
				_lea(ir_now);
				break;
			case GETREG:
				_getreg(ir_now);
			case NOTE:
				_note(ir_now);
			default:
				break;
			}
		}
	}
	arm << ".global main\n";
	arm << ".global __aeabi_idiv\n"; 
	arm << ".global __aeabi_idivmod\n";
	for (auto s : output_buffer) {
		arm << s << "\n";
	}
}

