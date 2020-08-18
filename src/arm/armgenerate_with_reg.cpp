#include"armgenerate.h"
#include<iostream>
#include<vector>
#include<string>
#include<fstream>
#include<map>
#include<ctype.h>
#include<set>
#include<regex>
#include<algorithm>
#include<queue>
#define OUTPUT(s) do{output_buffer.push_back(s);}while(0)
using namespace std;

vector<string> output_buffer;
int symbol_pointer;
map<string, int> var2addr;
int sp;
int sp_recover;
int pushNum = 0;
string global_var_name;
int global_var_size = 0;
vector<pair<string, int>> ini_value;
bool is_global = true;
string reglist = "R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,LR,R0";
string reglist_without0 = "R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,LR";
string global_reg_list;
int global_reg_size;
map<string, int> func2para;
map<string, vector<string>> funcname2pushlist;
int canOutput;

unsigned ror(unsigned val, int size)
{
	unsigned res = val >> size;
	res |= val << (32 - size);
	return res;
}

bool is_illegal(string im, bool str = false);

void li(string reg, int im)
{
	//user ldr
	//OUTPUT("LDR " + reg + ",=" + to_string(im));
	//return;
	//user movw,movt
	if (is_illegal(to_string(im))) {
		OUTPUT("MOV " + reg + ",#" + to_string(im));
		return;
	}
	OUTPUT("MOVW " + reg + ",#" + to_string(im & 0xffff));
	if ((im & 0xffff0000) != 0) {
		OUTPUT("MOVT " + reg + ",#" + to_string((unsigned)(im & 0xffff0000) >> 16));
	}
	return;
}

string getname(string ir_name)
{
	if (ir_name[0] == '@') {
		return ir_name.substr(1);
	}
}

string get_varname_scale(string name)
{
	if (name.size() == 0) {
		return "";
	}
	for (int i = 0; i < name.size(); i++) {
		if (name[i] == '+') {
			return name.substr(i + 1);
		}
	}
	return "";
}

void getcallerMap()
{
	map<string, set<int>> func2reg_without_inline;
	map<string, int> funcname2index;
	map<string, set<string>> funcname2caller;
	string funcname = "";
	for (int i = 0; i < LIR.size(); i++) {
		for (int j = 0; j < LIR[i].size(); j++) {
			CodeItem* ir_now = &LIR[i][j];
			auto type = ir_now->getCodetype();
			if (type == DEFINE) {
				funcname = getname(ir_now->getResult());
				funcname2index[funcname] = i;
			}
			else if (type == CALL) {
				string name = getname(ir_now->getOperand1());
				if (funcname2caller.find(name) != funcname2caller.end()) {
					funcname2caller[name].insert(funcname);
				}
				else {
					funcname2caller[name] = { funcname };
				}
			}
			else if (type == ALLOC || type == PARA) {
				string scale = get_varname_scale(ir_now->getResult());
				if (scale == funcname) {
					string loca = ir_now->getOperand1();
					if (loca[0] == 'R') {
						func2reg_without_inline[funcname].insert(stoi(loca.substr(1)));
					}
				}
			}
		}
	}
	for (auto p : funcname2caller) {
		set<string> caller_gReg;
		queue<string> caller_queue;
		set<string> func_done; //in case some dunc call itself
		for (string c : p.second) {
			caller_queue.push(c);
		}
		while (caller_queue.size() != 0) {
			string f = caller_queue.front();
			for (string c : funcname2caller[f]) {
				if (func_done.find(c) == func_done.end()) {
					caller_queue.push(c);
				}
			}
			caller_queue.pop();
			for (int reg : func2reg_without_inline[f]) {
				caller_gReg.insert("R" + to_string(reg));
			}
			func_done.insert(f);
		}
		for (string g : func2gReg[funcname2index[p.first]]) {
			if (caller_gReg.find(g) != caller_gReg.end()) {
				funcname2pushlist[p.first].push_back(g);
			}
		}
	}
}

bool judgetemp(string a)		//丛加的
{
	if (a == "R0" || a == "R1" || a == "R2" || a == "R3" || a == "R12") {
		return true;
	}
	return false;
}
bool judgeglobal(string a)		//丛加的
{
	if (a == "R4" || a == "R5" || a == "R6" || a == "R7" || a == "R8" || a == "R9" || a == "R10" || a == "R11") {
		return true;
	}
	return false;
}
bool judgeLoadOp(string a)		//丛加的
{
	if (a == "LDR" || a == "ADD" || a == "SUB" || a == "ASR" || a == "LSL" || a == "MUL"  || a == "MOV" || a == "MLS" || a == "MLA") {
		return true;
	}
	return false;
}
bool is_nonsence(int index)
{
	string str = output_buffer[index];
	smatch result;
	canOutput = 1;
	if (str == "NOP") {
		return true;
	}
	regex pattern1("MOV\\s+([SPLR0-9]+)\\s*,\\s*([SPLR0-9]+)\\s*");
	if (regex_match(str, result, pattern1)) {
		if (result[1] == result[2]) {
			return true;
		}
	}
	regex pattern2("(ADD|SUB)\\s+([SPLR0-9]+)\\s*,\\s*([SPLR0-9]+)\\s*,\\s*#-?0\\s*");
	if (regex_match(str, result, pattern2)) {
		if (result[2] == result[3]) {
			return true;
		}
	}
	//mull + add -> mla
	regex pattern3("MUL\\s+([SPLR0-9]+)\\s*,\\s*([SPLR0-9]+)\\s*,\\s*([SPLR0-9]+)\\s*");
	regex pattern4("ADD\\s+([SPLR0-9]+)\\s*,\\s*([SPLR0-9]+)\\s*,\\s*([SPLR0-9]+)\\s*");
	if (regex_match(str, result, pattern3)) {
		string mul_re = result[1];
		string mul_op1 = result[2];
		string mul_op2 = result[3];
		if (output_buffer[index + 1].substr(0, 3) == "ADD" &&
			regex_match(output_buffer[index + 1], result, pattern4)) {
			string add_re = result[1];
			string add_op1 = result[2];
			string add_op2 = result[3];
			if (mul_re == add_op1) {
				output_buffer[index] = "MLA " + add_re + "," + mul_op1 + "," + mul_op2 + "," + add_op2;
				output_buffer[index + 1] = "NOP";
				return false;
			}
			if (mul_re == add_op2) {
				output_buffer[index] = "MLA " + add_re + "," + mul_op1 + "," + mul_op2 + "," + add_op1;
				output_buffer[index + 1] = "NOP";
				return false;
			}
		}
	}


	string a = str.substr(0, 3);	//删除连续相同MOV,丛加的
	if (a == "MOV" && output_buffer[index] == output_buffer[index - 1]) {
		return true;
	}
	//数组做参数取值或存值时，基址是全局寄存器，会先将其放到临时寄存器产生冗余
	//例：MOV R2,R7   STR R1,[R2,R0]   ——>  STR R1,[R7,R0]		丛加的
	//注：由于寄存器可能是3位，因此会带来问题
	if (index + 1 < output_buffer.size()) {
		string strNext = output_buffer[index + 1];
		string nextOp = strNext.substr(0, 3);
		if (a == "MOV" && (nextOp == "STR" || nextOp == "LDR")) {
			string str_num1 = str.substr(4, 2);
			string str_num2 = str.substr(7, 2);
			string next_num1 = strNext.substr(8, 2);
			string next_num2 = strNext.substr(9, 2);      //MOV R2,R11  LDR R12,[R2,R12]
			if (str_num1 == next_num1 && judgetemp(str_num1) && judgeglobal(str_num2)) {	//情况一：默认寄存器都是2位
				output_buffer.erase(output_buffer.begin() + index);
				output_buffer.erase(output_buffer.begin() + index);   //连删两条指令
				cout << str << endl;
				cout << strNext << endl;
				string newstr = strNext.substr(0, 8) + str_num2 + strNext.substr(10, strNext.size());
				output_buffer.insert(output_buffer.begin() + index, newstr);
				canOutput = 0;
				return false;
			}
			if (str_num1 == next_num2 && judgetemp(str_num1) && judgeglobal(str_num2)) {	//情况一：默认寄存器都是2位
				output_buffer.erase(output_buffer.begin() + index);
				output_buffer.erase(output_buffer.begin() + index);   //连删两条指令
				cout << str << endl;
				cout << strNext << endl;
				string newstr = strNext.substr(0, 9) + str_num2 + strNext.substr(11, strNext.size());
				output_buffer.insert(output_buffer.begin() + index, newstr);
				canOutput = 0;
				return false;
			}
			str_num1 = str.substr(4, 2);
			str_num2 = str.substr(7, 3);
			next_num1 = strNext.substr(8, 2); //例：MOV R2,R10   STR R1,[R2,R0] 
			next_num2 = strNext.substr(9, 2); //例：MOV R2,R10   STR R1,[R2,R0] 
			if (str_num1 == next_num1 && judgetemp(str_num1) && judgeglobal(str_num2)) {	//情况二：默认全局寄存器是3位
				output_buffer.erase(output_buffer.begin() + index);
				output_buffer.erase(output_buffer.begin() + index);   //连删两条指令
				cout << str << endl;
				cout << strNext << endl;
				string newstr = strNext.substr(0, 8) + str_num2 + strNext.substr(10, strNext.size());
				output_buffer.insert(output_buffer.begin() + index, newstr);
				canOutput = 0;
				return false;
			}
			if (str_num1 == next_num2 && judgetemp(str_num1) && judgeglobal(str_num2)) {	//情况一：默认寄存器都是2位
				output_buffer.erase(output_buffer.begin() + index);
				output_buffer.erase(output_buffer.begin() + index);   //连删两条指令
				cout << str << endl;
				cout << strNext << endl;
				string newstr = strNext.substr(0, 9) + str_num2 + strNext.substr(11, strNext.size());
				output_buffer.insert(output_buffer.begin() + index, newstr);
				canOutput = 0;
				return false;
			}
			str_num1 = str.substr(4, 3);
			str_num2 = str.substr(8, 2);
			next_num1 = strNext.substr(8, 3);  //例：MOV R12,R4   STR R1,[R12,R0] 
			next_num2 = strNext.substr(9, 3);
			if (str_num1 == next_num1 && judgetemp(str_num1) && judgeglobal(str_num2)) {	//情况三：默认临时寄存器是3位
				output_buffer.erase(output_buffer.begin() + index);
				output_buffer.erase(output_buffer.begin() + index);   //连删两条指令
				cout << str << endl;
				cout << strNext << endl;
				string newstr = strNext.substr(0, 8) + str_num2 + strNext.substr(11, strNext.size());
				output_buffer.insert(output_buffer.begin() + index, newstr);
				canOutput = 0;
				return false;
			}
			if (str_num1 == next_num2 && judgetemp(str_num1) && judgeglobal(str_num2)) {	//情况一：默认寄存器都是2位
				output_buffer.erase(output_buffer.begin() + index);
				output_buffer.erase(output_buffer.begin() + index);   //连删两条指令
				cout << str << endl;
				cout << strNext << endl;
				string newstr = strNext.substr(0, 9) + str_num2 + strNext.substr(12, strNext.size());
				output_buffer.insert(output_buffer.begin() + index, newstr);
				canOutput = 0;
				return false;
			}
			str_num1 = str.substr(4, 3);
			str_num2 = str.substr(8, 3);
			next_num1 = strNext.substr(8, 3);  //例：MOV R12,R10   STR R1,[R12,R0] 
			next_num2 = strNext.substr(9, 3);
			if (str_num1 == next_num1 && judgetemp(str_num1) && judgeglobal(str_num2)) {	//情况四：默认临时寄存器和全局寄存器都是3位
				output_buffer.erase(output_buffer.begin() + index);
				output_buffer.erase(output_buffer.begin() + index);   //连删两条指令
				cout << str << endl;
				cout << strNext << endl;
				string newstr = strNext.substr(0, 8) + str_num2 + strNext.substr(11, strNext.size());
				output_buffer.insert(output_buffer.begin() + index, newstr);
				canOutput = 0;
				return false;
			}
			if (str_num1 == next_num2 && judgetemp(str_num1) && judgeglobal(str_num2)) {	//情况一：默认寄存器都是2位
				output_buffer.erase(output_buffer.begin() + index);
				output_buffer.erase(output_buffer.begin() + index);   //连删两条指令
				cout << str << endl;
				cout << strNext << endl;
				string newstr = strNext.substr(0, 9) + str_num2 + strNext.substr(12, strNext.size());
				output_buffer.insert(output_buffer.begin() + index, newstr);
				canOutput = 0;
				return false;
			}
		}
	}
	//传参优化	上一条是指令运算结果，比如LDR、ADD、SUB、ASR、LSL、MUL为上一条指令可求出结果的指令，只能以R0为相同寄存器
	//例：LDR R0,=C  MOV R3,R0  ——> LDR R3,=C		丛加的
	//注：这里只涉及R0~R3的情况，所以截取2单位长度没问题
	if (index + 1 < output_buffer.size()) {
		string strNext = output_buffer[index + 1];
		string nextOp = strNext.substr(0, 3);
		if (nextOp == "MOV" && judgeLoadOp(a)) {
			string next_num1 = strNext.substr(4, 2);
			string next_num2 = strNext.substr(7, 2);
			string next_num3 = strNext.substr(7, 3);		//防止next_num2实际是R11、R12只是因为取两位才为R1
			string num1 = str.substr(4, 2);
			string num2 = str.substr(4, 3);
			if (num1 == next_num2 && (next_num2 == "R0" || next_num2 == "R1" || next_num2 == "R2" || next_num2 == "R3") 
				&& next_num2 == next_num3 && num2 != "R10" && num2!="R11" && num2 != "R12") {
				output_buffer.erase(output_buffer.begin() + index);
				output_buffer.erase(output_buffer.begin() + index);   //连删两条指令
				cout << str << endl;
				cout << strNext << endl;
				string newstr = str.substr(0, 4) + next_num1 + str.substr(6, str.size());
				output_buffer.insert(output_buffer.begin() + index, newstr);
				return false;
			}
			//MOV R10,R0  MOV R4,R1
			next_num1 = strNext.substr(4, 3);
			next_num2 = strNext.substr(8, 2);
			next_num3 = strNext.substr(8, 3);				//防止next_num2实际是R11、R12只是因为取两位才为R1
			num1 = str.substr(4, 2);		//情况二：LDR R0,=C   MOV R10, R0
			num2 = str.substr(4, 3);
			if (num1 == next_num2 && (next_num2 == "R0" || next_num2 == "R1" || next_num2 == "R2" || next_num2 == "R3") 
				&& next_num2 == next_num3 && num2 != "R10" && num2 != "R11" && num2 != "R12") {
				output_buffer.erase(output_buffer.begin() + index);
				output_buffer.erase(output_buffer.begin() + index);   //连删两条指令
				cout << str << endl;
				cout << strNext << endl;
				string newstr = str.substr(0, 4) + next_num1 + str.substr(6, str.size());
				output_buffer.insert(output_buffer.begin() + index, newstr);
				return false;
			}

		}
	}
	//存取优化  STR R0,[SP,#176]	LDR R0, [SP, #176]这里第二条LDR完全没用
	if (index + 1 < output_buffer.size()) {
		string strNext = output_buffer[index + 1];
		string nextOp = strNext.substr(0, 3);
		if (nextOp == "LDR" && a=="STR") {
			string aa = str.substr(4, str.size());
			string bb = strNext.substr(4, str.size());
			if (aa == bb) {
				output_buffer.erase(output_buffer.begin() + index + 1);
				cout << str << endl;
				cout << strNext << endl;
				return false;
			}
		}
	}
	return false;
}
/*
bool is_nonsence(string str)
{
	smatch result;
	regex pattern1("MOV\\s+([SPLR0-9]+)\\s*,\\s*([SPLR0-9]+)\\s*");
	if (regex_match(str, result, pattern1)) {
		if (result[1] == result[2]){
			return true;
		}
	}
	regex pattern2("(ADD|SUB)\\s+([SPLR0-9]+)\\s*,\\s*([SPLR0-9]+)\\s*,\\s*#-?0\\s*");
	if (regex_match(str, result, pattern2)) {
		if (result[2] == result[3]) {
			return true;
		}
	}
	return false;
}
*/

bool replace_div(int d, int* m_high, int* sh_post)
{
	bool sign = d >= 0;
	if (!sign) {
		d = -d;
	}
	int l = 0;
	int n = d;
	if (n >> 16) { n >>= 16; l |= 16; }
	if (n >> 8) { n >>= 8; l |= 8; }
	if (n >> 4) { n >>= 4; l |= 4; }
	if (n >> 2) { n >>= 2; l |= 2; }
	if (n >> 1) { n >>= 1; l |= 1; }
	int sh = l;
	long long m_low = (0x100000000L << l) / d;
	long long m_h = (0x100000002L << l) / d;
	while (m_low / 2 < m_h / 2 && sh > 0) {
		m_low /= 2;
		m_h /= 2;
		sh--;
	}
	*m_high = m_h;
	*sh_post = sh;
	return sign;
}

int is_power2(string im) {
	unsigned int n = stoi(im);
	if (n == -1) {
		return -33; //琛ㄧず鏄?1
	}
	int flag = n >= 0 ? 1 : -1;
	n *= flag;
	if ((n & (n - 1)) != 0) {
		return 33; //琛ㄧず闈炰簩鐨勫箓娆℃柟
	}
	int cnt = 0;
	if (n >> 16) { n >>= 16; cnt |= 16; }
	if (n >> 8) { n >>= 8; cnt |= 8; }
	if (n >> 4) { n >>= 4; cnt |= 4; }
	if (n >> 2) { n >>= 2; cnt |= 2; }
	if (n >> 1) { n >>= 1; cnt |= 1; }
	return flag * cnt; //0琛ㄧず1
}

bool check_format(CodeItem* ir) {
	string ops[3] = { ir->getResult(),ir->getOperand1(),ir->getOperand2() };
	for (auto s : ops) {
		if (s[0] == '%' && isdigit(s[1])) {
			return false;
		}
	}
	return true;
}

bool is_illegal(string im, bool str) {
	int i = stoi(im);
	if (str) {
		return i < 4096;
	}
	if (i <= 127 && i >= -128) {
		return true;
	}
	unsigned int mask = 0xff;
	do {
		if ((i & (~mask)) == 0) {
			return true;
		}
		mask = ror(mask, 2);
	} while (mask != 0xff);
	return false;
}

void global_flush()
{
	if (global_var_size == 0) {
		return;
	}
	OUTPUT(".data");
	OUTPUT(global_var_name + ":");
	int pre = 0;
	int off;
	for (auto p : ini_value) {
		off = p.second;
		if (off == pre) {
			OUTPUT(".word " + p.first);
		}
		else {
			OUTPUT(".zero " + to_string(off - pre));
			OUTPUT(".word " + p.first);
		}
		pre = off + 4;
	}
	if (pre / 4 != global_var_size) {
		OUTPUT(".zero " + to_string((long long)global_var_size * 4 - pre));
	}

	/*int zero_cnt = 0;
	for (string v : ini_value) {
		if (v == "") {
			zero_cnt++;
		}
		else {
			if (zero_cnt != 0) {
				OUTPUT(".zero " + to_string(zero_cnt * 4));
			}
			OUTPUT(".word " + v);
			zero_cnt = 0;
		}
	}
	if (zero_cnt != 0) {
		OUTPUT(".zero " + to_string(zero_cnt * 4));
	}*/
	OUTPUT(".text");
	ini_value.clear();
	global_var_size = 0;
}

string get_cmp_label(string logistic) {
	static int cnt = 0;
	return logistic + ".L" + to_string(cnt++);
}

pair<string, int> get_location(string name)
{
	if (name[0] == '@') {
		return { getname(name),-1 };
	}
	else if (isdigit(name[1]) && name[0] == '%') {
		return { "R" + to_string(stoi(name.substr(1)) % 7 + 4),-2 }; //锟剿达拷应锟斤拷模8,锟斤拷锟斤拷r11锟斤拷锟斤拷时锟侥达拷锟斤拷
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
	string value = ir->getOperand1();
	int size = stoi(ir->getOperand2());
	global_var_size = size;
	if (size > 1) {
		global_var_name = name;
		/*for (int i = 0; i < size; i++) {
			ini_value.push_back("");
		}*/
		//OUTPUT(name + ": .zero " + to_string(size * 4));
	}
	else {
		if (value == "") {
			global_var_name = name;
			//ini_value.push_back("");
		}
		else {
			OUTPUT(".data");
			OUTPUT(name + ": .word " + value);
			OUTPUT(".text");
			global_var_size = 0;
		}
	}
}

void _define(CodeItem* ir)
{
	string name = getname(ir->getResult());
	OUTPUT("");
	OUTPUT(".ltorg");
	OUTPUT(name + ":");
	var2addr.clear();
	int paraNum;
	int paraIndex = 0;
	int sp_without_para = 0;
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
				sp -= 4;
				var2addr[st.getName()] = sp;
			}
			else {
				var2addr[st.getName()] = sp_without_para + 4 * (paraIndex - 5);
			}
			break;
		}
		case FUNCTION: {
			paraNum = st.getparaLength();
			func2para[name] = paraNum;
			if (paraNum > 4) {
				sp -= (paraNum - 4) * 4;
			}
			sp_without_para = sp;
			//store regs
			{
				global_reg_list = "";
				global_reg_size = 0;
				int regN = 1;
				for (string reg : funcname2pushlist[name]) {
					global_reg_list += "," + reg;
					regN++;
				}
				global_reg_size = regN - 1;
				sp -= regN * 4;
				if (global_reg_list != "") {
					OUTPUT("PUSH {" + global_reg_list.substr(1) + ",LR}"); //push LR together,right?
				}
				else {
					OUTPUT("PUSH {LR}");
				}
				sp_recover = sp;
			}
			//pre alloc VR size
			{
				sp -= func2Vr[symbol_pointer].size() * 4;
			}
			break;
		}
		default:
			break;
		}
	}
	int temp_sp = sp_recover;
	for (auto m : func2Vr[symbol_pointer]) {
		temp_sp -= 4;
		var2addr[m.first] = temp_sp;
	}

	if (!is_illegal(to_string(sp - sp_recover))) {
		//OUTPUT("LDR R12,=" + to_string(sp - sp_without_para));
		li("LR", sp - sp_recover);
		OUTPUT("ADD SP,SP,LR");
	}
	else {
		OUTPUT("ADD SP,SP,#" + to_string(sp - sp_recover));
	}
}

void _alloc(CodeItem* ir)
{
	string name = ir->getResult();
	string ini_value = ir->getOperand1();
	int size = stoi(ir->getOperand2());
	if (ini_value != "") {
		//OUTPUT("LDR R12,=" + ini_value);
		li("R12", stoi(ini_value));
		int im = get_location(name).second - sp;
		if (!is_illegal(to_string(im))) {
			//OUTPUT("LDR LR,=" + to_string(im));
			li("LR", im);
			OUTPUT("STR R12,[SP,LR]");
		}
		else {
			OUTPUT("STR R12,[SP,#" + to_string(im) + "]");
		}
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
		if (!is_illegal(to_string(p.second - sp), true)) {
			//OUTPUT("LDR LR,=" + to_string(p.second - sp));
			li("LR", p.second - sp);
			OUTPUT("LDR " + target + ",[SP,LR]");
		}
		else {
			OUTPUT("LDR " + target + ",[SP,#" + to_string(p.second - sp) + "]");
		}
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
			//丛改的
			if (!is_illegal(to_string(p.second - sp))) {
				//OUTPUT("LDR LR,=" + to_string(p.second - sp));
				li("LR", p.second - sp);
				OUTPUT("ADD LR,LR," + offset);
				OUTPUT("LDR " + target + ",[SP,LR]");
			}
			else {			//如果立即数合法，可以少一条LDR立即数指令
				if (p.second - sp == 0) {
					OUTPUT("LDR " + target + ",[SP," + offset + "]");
				}
				else {
					OUTPUT("ADD LR," + offset + ",#" + to_string(p.second - sp));
					OUTPUT("LDR " + target + ",[SP,LR]");
				}
			}
			/*
			OUTPUT("ADD " + offset + "," + offset + ",SP");
			OUTPUT("ADD " + offset + "," + offset + ",#" + to_string(p.second - sp));
			OUTPUT("LDR " + target + ",[" + offset + "]");
			*/
		}
	}
	else {
		if (var[0] == 'R') {
			if (!is_illegal(offset, true)) {
				//OUTPUT("LDR LR,=" + offset);
				li("LR", stoi(offset));
				OUTPUT("LDR " + target + ",[" + var + ",LR]");
			}
			else {
				OUTPUT("LDR " + target + ",[" + var + ",#" + offset + "]");
			}
		}
		else {
			auto p = get_location(var);
			int im = p.second - sp + stoi(offset);
			if (!is_illegal(to_string(im), true)) {
				//OUTPUT("LDR LR,=" + to_string(im));
				li("LR", im);
				OUTPUT("LDR " + target + ",[SP,LR]");
			}
			else {
				OUTPUT("LDR " + target + ",[SP,#" + to_string(im) + "]");
			}
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
		if (!is_illegal(to_string(p.second - sp), true)) {
			//OUTPUT("LDR LR,=" + to_string(p.second - sp));
			li("LR", p.second - sp);
			OUTPUT("STR " + value + ",[SP,LR]");
		}
		else {
			OUTPUT("STR " + value + ",[SP,#" + to_string(p.second - sp) + "]");
		}
	}
}

void _storearr(CodeItem* ir)
{
	if (is_global) {
		string value = ir->getResult();
		int off = stoi(ir->getOperand2());
		ini_value.push_back({ value,off });
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
			//丛改的
			if (!is_illegal(to_string(p.second - sp))) {
				//OUTPUT("LDR LR,=" + to_string(p.second - sp));
				li("LR", p.second - sp);
				OUTPUT("ADD LR,LR," + offset);
				OUTPUT("STR " + value + ",[SP,LR]");
			}
			else {			//如果立即数合法，可以少一条LDR立即数指令
				if (p.second - sp == 0) {
					OUTPUT("STR " + value + ",[SP," + offset + "]");
				}
				else {
					OUTPUT("ADD LR," + offset + ",#" + to_string(p.second - sp));
					OUTPUT("STR " + value + ",[SP,LR]");
				}
			}
			/*
			OUTPUT("ADD " + offset + ",SP," + offset);
			OUTPUT("ADD " + offset + "," + offset + ",#" + to_string(p.second - sp));
			OUTPUT("STR " + value + ",[" + offset + "]");
			*/
		}
	}
	else {
		if (var[0] == 'R') {
			if (!is_illegal(offset, true)) {
				//OUTPUT("LDR LR,=" + offset);
				li("LR", stoi(offset));
				OUTPUT("STR " + value + ",[" + var + ",LR]");
			}
			else {
				OUTPUT("STR " + value + ",[" + var + ",#" + offset + "]");
			}
		}
		else {
			auto p = get_location(var);
			int im = p.second - sp + stoi(offset);
			if (!is_illegal(to_string(im), true)) {
				//OUTPUT("LDR LR,=" + to_string(im));
				li("LR", im);
				OUTPUT("STR " + value + ",[SP,LR]");
			}
			else {
				OUTPUT("STR " + value + ",[SP,#" + to_string(im) + "]");
			}
		}
	}
}

void _add(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op2[0] != 'R') {
		if (!is_illegal(op2)) {
			//OUTPUT("LDR LR,=" + op2);
			li("LR", stoi(op2));
			op2 = "LR";
		}
		else {
			op2 = "#" + op2;
		}
	}
	OUTPUT("ADD " + target + "," + op1 + "," + op2);
}

void _sub(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op1[0] != 'R') {
		if (!is_illegal(op1)) {
			//OUTPUT("LDR LR,=" + op1);
			li("LR", stoi(op1));
			OUTPUT("SUB " + target + ",LR," + op2);
		}
		else {
			OUTPUT("RSB " + target + "," + op2 + ",#" + op1);
		}
		return;
	}
	if (op2[0] != 'R') {
		if (!is_illegal(op2)) {
			//OUTPUT("LDR LR,=" + op2);
			li("LR", stoi(op2));
			op2 = "LR";
		}
		else {
			op2 = "#" + op2;
		}
	}
	OUTPUT("SUB " + target + "," + op1 + "," + op2);
}

void _mul(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op2[0] != 'R') {
		//姝ゅ鍙互鐪嬩竴涓嬭兘鍚︿娇鐢ㄧЩ浣嶅疄鐜颁箻娉?
		if (op2 == "0" || op2 == "-0") {
			OUTPUT("MOV " + target + ",#0");
			return;
		}
		else {
			int bitoff = is_power2(op2);
			if (bitoff == -33) {
				OUTPUT("RSB " + target + "," + op1 + ",#0");
				return;
			}
			else if (bitoff == 0) {
				OUTPUT("MOV " + target + "," + op1);
				return;
			}
			else if (bitoff < 0) {
				OUTPUT("LSL " + target + "," + op1 + ",#" + to_string(-bitoff));
				OUTPUT("RSB " + target + "," + target + ",#0");
				return;
			}
			else if (bitoff > 0 && bitoff != 33) {
				OUTPUT("LSL " + target + "," + op1 + ",#" + to_string(bitoff));
				return;
			}
		}
		//OUTPUT("LDR LR,=" + op2);
		li("LR", stoi(op2));
		op2 = "LR";
	}
	OUTPUT("MUL " + target + "," + op1 + "," + op2);
}

void _div(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op1[0] != 'R') {
		OUTPUT("PUSH {R0}");
		OUTPUT("PUSH {R1,R2,R3}");//remove R12,right?
		OUTPUT("MOV R1," + op2);
		//OUTPUT("LDR R0,=" + op1);
		li("R0", stoi(op1));
		OUTPUT("BL __aeabi_idiv");
		if (target == "R0") {
			OUTPUT("POP {R1,R2,R3}");//remove R12,right?
			OUTPUT("ADD SP,SP,#4");
		}
		else {
			OUTPUT("POP {R1,R2,R3}");//remove R12,right?
			OUTPUT("MOV " + target + ",R0");
			OUTPUT("POP {R0}");
		}
		return;
	}
	if (op2[0] != 'R') {
		int bitoff = is_power2(op2);
		if (bitoff == -33) {
			/*OUTPUT("MOV LR,#0");
			OUTPUT("SUB " + target + ",LR," + op1);*/
			OUTPUT("RSB " + target + "," + op1 + ",#0");
		}
		else if (bitoff == 33) {
			int m_high;
			int sh_post;
			bool sign = replace_div(stoi(op2), &m_high, &sh_post);
			bool pop = false;
			li("LR", m_high);
			//OUTPUT("SMMUL LR,LR," + op1);
			//if (m_high < 0) {
			//	OUTPUT("ADD LR,LR," + op1);//璇曡瘯
			//}
			if (m_high < 0) {
				//OUTPUT("ADD LR,LR," + op1);
				OUTPUT("SMMLA LR,LR," + op1 + "," + op1);
			}
			else {
				OUTPUT("SMMUL LR,LR," + op1);
			}
			OUTPUT("ASR LR,LR,#" + to_string(sh_post));
			OUTPUT("SUB " + target + ",LR," + op1 + ",ASR #31");
			if (!sign) {
				/*OUTPUT("MVN " + target + "," + target);
				OUTPUT("ADD " + target + "," + target + ",#1");*/
				OUTPUT("RSB " + target + "," + target + ",#0");
			}
		}
		else if (bitoff == 0) {
			OUTPUT("MOV " + target + "," + op1);
		}
		else if (bitoff > 0) {
			OUTPUT("ASR LR," + op1 + ",#31");
			OUTPUT("ADD " + target + "," + op1 + ",LR,LSR #" + to_string(32 - bitoff));
			OUTPUT("ASR " + target + "," + target + ",#" + to_string(bitoff));
		}
		else if (bitoff < 0) {
			OUTPUT("ASR LR," + op1 + ",#31");
			OUTPUT("ADD " + target + "," + op1 + ",LR,LSR #" + to_string(32 + bitoff));
			OUTPUT("ASR " + target + "," + target + ",#" + to_string(-bitoff));
			/*OUTPUT("MOV LR,#0");
			OUTPUT("SUB " + target + ",LR," + target);*/
			OUTPUT("RSB " + target + "," + target + ",#0");
		}
		return;
	}
	{
		OUTPUT("PUSH {R0}");
		OUTPUT("PUSH {R1,R2,R3}");//remove R12,right?
		if (op2 == "R0") {
			OUTPUT("MOV LR,R0");
		}
		OUTPUT("MOV R0," + op1);
		if (op2[0] == 'R') {
			if (op2 == "R0") {
				OUTPUT("MOV R1,LR");
			}
			else {
				OUTPUT("MOV R1," + op2);
			}
		}
		else {
			//OUTPUT("LDR R1,=" + op2);
			li("R1", stoi(op2));
		}
		OUTPUT("BL __aeabi_idiv");
		if (target == "R0") {
			OUTPUT("POP {R1,R2,R3}");//remove R12,right?
			OUTPUT("ADD SP,SP,#4");
		}
		else {
			OUTPUT("POP {R1,R2,R3}");//remove R12,right?
			OUTPUT("MOV " + target + ",R0");
			OUTPUT("POP {R0}");
		}
		return;
	}
}

void _rem(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (op1[0] != 'R') {
		OUTPUT("PUSH {R1}");
		OUTPUT("PUSH {R0,R2,R3}"); //remove R12,right?
		OUTPUT("MOV R1," + op2);
		//OUTPUT("LDR R0,=" + op1);
		li("R0", stoi(op1));
		OUTPUT("BL __aeabi_idivmod");
		if (target == "R1") {
			OUTPUT("POP {R0,R2,R3}"); //remove R12,right?
			OUTPUT("ADD SP,SP,#4");
		}
		else {
			OUTPUT("POP {R0,R2,R3}"); //remove R12,right?
			OUTPUT("MOV " + target + ",R1");
			OUTPUT("POP {R1}");
		}
		return;
	}
	if (op2[0] != 'R') {
		int bitoff = is_power2(op2);
		if (bitoff == -33 || bitoff == 0) {
			OUTPUT("MOV " + target + ",#0");
		}
		else if (bitoff == 33) {
			int m_high;
			int sh_post;
			bool sign = replace_div(stoi(op2), &m_high, &sh_post);
			li("LR", m_high);
			/*OUTPUT("SMMUL LR,LR," + op1);*/
			if (m_high < 0) {
				//OUTPUT("ADD LR,LR," + op1);
				OUTPUT("SMMLA LR,LR," + op1 + "," + op1);
			}
			else {
				OUTPUT("SMMUL LR,LR," + op1);
			}
			OUTPUT("ASR LR,LR,#" + to_string(sh_post));
			OUTPUT("SUB LR,LR," + op1 + ",ASR #31");
			if (!sign) {
				/*OUTPUT("MVN " + target + "," + target);
				OUTPUT("ADD " + target + "," + target + ",#1");*/
				OUTPUT("RSB LR,LR,#0");
			}
			string tempreg;
			bool pop = false;
			if (target != op1) {
				tempreg = target;
			}
			else if (target == "R12") {
				OUTPUT("PUSH {R3}");
				tempreg = "R3";
				pop = true;
			}
			else {
				OUTPUT("PUSH {R12}");
				tempreg = "R12";
				pop = true;
			}
			//OUTPUT("LDR " + tempreg + ",=" + op2);
			li(tempreg, stoi(op2));
			/*OUTPUT("MUL LR,LR," + tempreg);
			OUTPUT("SUB " + target + "," + op1 + ",LR");*/
			OUTPUT("MLS " + target + ",LR," + tempreg + "," + op1);
			if (pop) {
				OUTPUT("POP {" + tempreg + "}");
			}
		}
		else {
			int im = stoi(op2);
			im = bitoff < 0 ? -im : im;
			OUTPUT("RSBS LR," + op1 + ",#0");
			OUTPUT("AND LR,LR,#" + to_string(im - 1));
			OUTPUT("AND " + target + "," + op1 + ",#" + to_string(im - 1));
			OUTPUT("RSBPL " + target + ",LR,#0");
		}
		return;
	}
	{
		OUTPUT("PUSH {R1}");
		OUTPUT("PUSH {R0,R2,R3}");//remove R12,right?
		if (op2 == "R0") {
			OUTPUT("MOV LR,R0");
		}
		OUTPUT("MOV R0," + op1);
		if (op2[0] == 'R') {
			if (op2 == "R0") {
				OUTPUT("MOV R1,LR");
			}
			else {
				OUTPUT("MOV R1," + op2);
			}
		}
		else {
			//OUTPUT("LDR R1,=" + op2);
			li("R1", stoi(op2));
		}
		OUTPUT("BL __aeabi_idivmod");
		if (target == "R1") {
			OUTPUT("POP {R0,R2,R3}");//remove R12,right?
			OUTPUT("ADD SP,SP,#4");
		}
		else {
			OUTPUT("POP {R0,R2,R3}");//remove R12,right?
			OUTPUT("MOV " + target + ",R1");
			OUTPUT("POP {R1}");
		}
	}
}

void _and(CodeItem* ir)
{
	string target = ir->getResult();
	string op1 = ir->getOperand1();
	string op2 = ir->getOperand2();
	if (target[0] == '%') {
		if (op2[0] != 'R') {
			int im = stoi(op2);
			if (im == 0) {
				OUTPUT("B " + target.substr(1));
			}
			else {
				OUTPUT("CMP " + op1 + ",#0");
				OUTPUT("BEQ " + target.substr(1));
			}
		}
		else {
			OUTPUT("CMP " + op1 + ",#0");
			OUTPUT("BEQ " + target.substr(1));
			OUTPUT("CMP " + op2 + ",#0");
			OUTPUT("BEQ " + target.substr(1));
		}
		return;
	}
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
	if (target[0] == '%') {
		if (op2[0] != 'R') {
			int im = stoi(op2);
			if (im == 0) {
				OUTPUT("CMP " + op1 + ",#0");
				OUTPUT("BEQ " + target.substr(1));
			}
			else {
				//do nothing
			}
		}
		else {
			string label = get_cmp_label("OR");
			OUTPUT("CMP " + op1 + ",#0");
			OUTPUT("BNE " + label);
			OUTPUT("CMP " + op2 + ",#0");
			OUTPUT("BEQ " + target.substr(1));
			OUTPUT(label + ":");
		}
		return;
	}
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
	if (target[0] == '%') {
		OUTPUT("CMP " + op1 + ",#0");
		OUTPUT("BNE " + target.substr(1));
		return;
	}
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
		if (!is_illegal(op2)) {
			//OUTPUT("LDR LR,=" + op2);
			li("LR", stoi(op2));
			op2 = "LR";
		}
		else {
			op2 = "#" + op2;
		}
	}
	if (target[0] == '%') {
		OUTPUT("CMP " + op1 + "," + op2);
		OUTPUT("BEQ " + target.substr(1));
		return;
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
		if (!is_illegal(op2)) {
			//OUTPUT("LDR LR,=" + op2);
			li("LR", stoi(op2));
			op2 = "LR";
		}
		else {
			op2 = "#" + op2;
		}
	}
	if (target[0] == '%') {
		OUTPUT("CMP " + op1 + "," + op2);
		OUTPUT("BNE " + target.substr(1));
		return;
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
		if (!is_illegal(op2)) {
			//OUTPUT("LDR LR,=" + op2);
			li("LR", stoi(op2));
			op2 = "LR";
		}
		else {
			op2 = "#" + op2;
		}
	}
	if (target[0] == '%') {
		OUTPUT("CMP " + op1 + "," + op2);
		OUTPUT("BGT " + target.substr(1));
		return;
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
		if (!is_illegal(op2)) {
			//OUTPUT("LDR LR,=" + op2);
			li("LR", stoi(op2));
			op2 = "LR";
		}
		else {
			op2 = "#" + op2;
		}
	}
	if (target[0] == '%') {
		OUTPUT("CMP " + op1 + "," + op2);
		OUTPUT("BGE " + target.substr(1));
		return;
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
		if (!is_illegal(op2)) {
			//OUTPUT("LDR LR,=" + op2);
			li("LR", stoi(op2));
			op2 = "LR";
		}
		else {
			op2 = "#" + op2;
		}
	}
	if (target[0] == '%') {
		OUTPUT("CMP " + op1 + "," + op2);
		OUTPUT("BLT " + target.substr(1));
		return;
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
		if (!is_illegal(op2)) {
			//OUTPUT("LDR LR,=" + op2);
			li("LR", stoi(op2));
			op2 = "LR";
		}
		else {
			op2 = "#" + op2;
		}
	}
	if (target[0] == '%') {
		OUTPUT("CMP " + op1 + "," + op2);
		OUTPUT("BLE " + target.substr(1));
		return;
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
		//OUTPUT("BNE " + label1); //鏈夐棶棰樺彲浠ュ洖澶嶈繖閲岃瘯璇?
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
	/*
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
	}*/
	if (!is_illegal(to_string(sp_recover - sp))) {
		//OUTPUT("LDR R12,=" + to_string(sp_without_para - fake_sp));
		li("R12", sp_recover - sp);
		OUTPUT("ADD SP,SP,R12");
	}
	else {
		OUTPUT("ADD SP,SP,#" + to_string(sp_recover - sp));
	}
	if (global_reg_list != "") {
		OUTPUT("POP {" + global_reg_list.substr(1) + ",PC}"); //pop lr together,right?
	}
	else {
		OUTPUT("POP {PC}");
	}
	//OUTPUT("MOV PC,LR");
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
		int im = stoi(src);
		if (!is_illegal(to_string(im))) {
			OUTPUT("LDR " + dst + ",=" + src);
		}
		else {
			OUTPUT("MOV " + dst + ",#" + src);
		}
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
		if (!is_illegal(to_string(p.second - sp))) {
			//OUTPUT("LDR LR,=" + to_string(p.second - sp));
			li("LR", p.second - sp);
			OUTPUT("ADD " + reg + ",SP,LR");
		}
		else {
			OUTPUT("ADD " + reg + ",SP,#" + to_string(p.second - sp));
		}
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
	//OUTPUT("@note " + ir->getResult() + " " + ir->getOperand1() + " " + ir->getOperand2());
	/*
	string status = ir->getOperand2();
	string note = ir->getOperand1();
	if (status == "begin" && note == "func") {
		OUTPUT("PUSH {R0}");
		OUTPUT("PUSH {" + reglist_without0 + "}");
		sp -= 56;
	}*/
	string name = ir->getResult();
	string type = ir->getOperand1();
	string status = ir->getOperand2();
	if (type == "func" && status.substr(0, 3) == "end") {
		//int paraN = func2para[name.substr(1)];
		int paraN = stoi(status.substr(3));
		if (paraN > 4) {
			int off = (paraN - 4) * 4;
			sp += off;
			if (!is_illegal(to_string(off))) {
				//OUTPUT("LDR LR,=" + to_string(off));
				li("LR", off);
				OUTPUT("ADD SP,SP,LR");
			}
			else {
				OUTPUT("ADD SP,SP,#" + to_string(off));
			}
		}
	}
}

void _arrayinit(CodeItem* ir)
{
	string iniv = ir->getResult();
	string name = ir->getOperand1();
	string size = ir->getOperand2();
	auto p = get_location(name);
	////第一种，使用memset
	//if (!is_illegal(to_string(p.second - sp))) {
	//	OUTPUT("LDR LR,=" + to_string(p.second - sp));
	//	OUTPUT("ADD R0,SP,LR");
	//}
	//else {
	//	OUTPUT("ADD R0,SP,#" + to_string(p.second - sp));
	//}
	//OUTPUT("LDR R1,=" + iniv);
	//OUTPUT("LDR R2,=" + to_string(stoi(size) * 4));
	//OUTPUT("BL memset");
	//第二种，连续存
	int length = stoi(size) + stoi(ir->getExtend());
	//OUTPUT("LDR LR,=" + iniv);
	li("LR", stoi(iniv));
	for (int i = stoi(ir->getExtend()); i < length; i += 1) {
		int off = p.second - sp + i * 4;
		if (!is_illegal(to_string(off), true)) {
			//OUTPUT("LDR R12,=" + to_string(off));
			li("R12", off);
			OUTPUT("STR LR,[SP,R12]");
		}
		else {
			OUTPUT("STR LR,[SP,#" + to_string(off) + "]");
		}
	}
}

void arm_generate(string sname)
{
	ofstream arm(sname);
	getcallerMap();
	//将局部数组移动到最后
	for (int i = 1; i < total.size(); i++) {
		int max = 0;
		int k = 0;
		for (int j = 1; j < total[i].size(); j++) {
			if (total[i][j].getForm() == VARIABLE && total[i][j].getDimension() > 0) {
				if (total[i][j].getUseCount() > max) {
					max = total[i][j].getUseCount();
					k = j;
				}
			}
		}
		if (k > 0) {	//移动符号表，把局部数组使用次数最多的元素放到最后面
			symbolTable item = total[i][k];
			total[i].erase(total[i].begin() + k);
			total[i].push_back(item);
		}
	}
	for (int i = 0; i < LIR.size(); i++) {
		for (int j = 0; j < LIR[i].size(); j++) {
			CodeItem* ir_now = &LIR[i][j];
			/*if (!check_format(ir_now)) {
				continue;
			}*/
			switch (ir_now->getCodetype())
			{
			case ADD:
				_add(ir_now);
				break;
			case SUB:
				_sub(ir_now);
				break;
			case DIV:
				_div(ir_now); //锟斤拷锟斤拷
				break;
			case MUL:
				_mul(ir_now);
				break;
			case REM:
				_rem(ir_now); //锟斤拷锟斤拷
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
				//_alloc(ir_now); //啥锟斤拷锟斤拷锟矫干ｏ拷
				break;
			case STORE:
				_store(ir_now); //全锟斤拷锟斤拷么锟姐？
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
				_call(ir_now); //R0锟斤拷锟角ｏ拷
				break;
			case RET:
				_ret(ir_now); //RET锟斤拷么锟斤拷
				break;
			case PUSH:
				_push(ir_now); //前锟侥革拷锟斤拷锟解，锟斤拷锟斤拷顺锟斤拷锟斤拷锟解。
				break;
			case POP:
				break;//锟斤拷锟斤拷POP锟斤拷锟阶革拷锟斤拷
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
				//_getreg(ir_now);
				break;
			case NOTE:
				_note(ir_now);
				break;
			case ARRAYINIT:
				_arrayinit(ir_now);
				break;
			default:
				break;
			}
		}
	}
	arm << ".global main\n";
	arm << ".global __aeabi_idiv\n";
	arm << ".global __aeabi_idivmod\n";
	//arm << "main:\nMOV PC,LR\n";
	cout << "get rid of these:\n";
	int out_bufferSize;
	for (out_bufferSize = 0; out_bufferSize < output_buffer.size(); out_bufferSize++) {
		if (is_nonsence(out_bufferSize)) {
			cout << output_buffer[out_bufferSize] << "\n";
		}
		else {
			if (canOutput == 1) {
				arm << output_buffer[out_bufferSize] << "\n";
			}
			else {
				out_bufferSize--;		//优化后的指令又再被优化的可能
			}
		}
	}
	/*
	for (auto s : output_buffer) {
		if (is_nonsence(s)) {
			cout << s << "\n";
		}
		else {
			arm << s << "\n";
		}
	}
	*/
}
