﻿#include "syntax.h"
#include "../util/meow.h"

using namespace std;

const char* print[26] = {
	"IDENFR ","INTCON ","PLUS ","MINU ","MULT ","DIV_WORD ","MOD ","AND_WORD ","OR_WORD ",
	"LSS ","LEQ ","GRE ","GEQ ","EQL_WORD ","NEQ_WORD ","REV ","ASSIGN ","SEMICN ","COMMA ",
	"LPARENT ","RPARENT ","LBRACK ","RBRACK ","LBRACE ","RBRACE ","STRING "
};

const char* premain[10] = {
	"CONSTTK", "INTTK", "VOIDTK", "MAINTK", "IFTK", "ELSETK",
	"WHILETK", "BREAKTK", "CONTINUETK", "RETURNTK"
};



//符号表引入变量
int Funcindex = 0;		//函数符号表在table中总位置
symbolTable searchItem; //查找的符号表
vector<symbolTable> table;      //分符号表
vector<vector<symbolTable>> total;	//总符号表
vector<int> fatherBlock;	//所有父block号
int Blockindex;			//函数中当前block号,函数最外层和参数block号为1
vector< map<string, int> > names;	//所有变量、常量、参数名

//数组赋值引入变量
int offset;						//当前维度下偏移量
int totalSize;					//总大小
vector<int> matrixLength;		//当前数组每个维度值
int Ndimension;					//当前嵌套维度
string nodeName;				//数组名
int isNesting;                  //是否嵌套，即{{
int isBegin;					//是否开头，最前面的{不算

int Temp = 0;				//中间变量起始编号
vector<int> func2tmpIndex;	//临时变量索引值
int iflabelIndex;          //if标签下标
int whilelabelIndex;		//while标签下标
vector<string> whileLabel;      //用于break和continue对应的标签
int paraNum;				//调用函数时记录参数个数

int interIndex;          //中间代码下标
string interRegister;        //中间变量字符串
int interType;				 //中间变量类型

vector<vector<CodeItem>> codetotal;   //总中间代码
string numToString(int a)
{
	stringstream trans;          //数字和字符串相互转化渠道
	trans << a;
	return trans.str();
}
int stringToNum(string a)
{
	stringstream trans;
	trans << a;
	int number;
	trans >> number;
	return number;
}
int Range;									//作用域  0表示全局   ＞0表示局部
symbolTable checkItem(string checkname)             //查表：在传参时判断变量是否为数组时使用
{
	int i, j, checkblock, block;
	string name;
	for (j = fatherBlock.size() - 1; j >= 0; j--) {
		checkblock = fatherBlock[j];
		for (i = 0; i < total[Funcindex].size(); i++) {   //先从本函数作用域找
			name = total[Funcindex][i].getName();
			block = total[Funcindex][i].getblockIndex();
			if (name == checkname && block == checkblock) {  //从最近的作用域找到了
				Range = Funcindex;
				return total[Funcindex][i];
			}
		}
	}
	for (i = 0; i < total[0].size(); i++) {
		name = total[0][i].getName();
		if (name == checkname) {  //从最近的作用域找到了
			total[0][i].setFuncindex(Funcindex);
			Range = 0;
			return total[0][i];
		}
	}
	exit(0);	// 新增by lzh
}
//生成目标语言引入变量
int paraIntNode = 0;		  //记录数组是否作为参数
string matrixName;			  //记录数组名

//词法、语法分析引入变量
string token;   //分析出来的单词
enum Memory symbol;  //分析出来的单词类别
//ofstream outfile;
Word wordAnalysis = Word();

void printMessage()
{
	string message;
	if (symbol > 9 && symbol != 36) {
		token = wordAnalysis.getToken();
		message = print[symbol - 10] + token;
	}
	else if (symbol >= 0 && symbol <= 9) {
		message = premain[symbol] + remain[symbol];
	}
	//outfile << message << endl;
}

void CompUnit();			   //编译单元
void ConstDecl(int index, int block);		       //常量说明
void ConstDef(int index, int block);			   //常量定义
void VarDecl(int index, int block);			   //变量说明
void VarDef(int index, int block);             //变量定义
void InitVal(int index);				//变量初值
void valueFuncDef();    //有返回值函数定义
void novalueFuncDef();  //无返回值函数定义
void Block();      //复合语句
int ConstExp();			//常量表达式         就不直接使用表达式了
int ConstMulExp();			//常量项
int ConstUnaryExp();			//常量因子
void ConstInitVal(int index);      //常量初值，返回维度值
void FuncFParams();         //参数表
void FuncFParam();         //函数形参
void Exp();					    //表达式(加减表达式)
void MulExp();					//乘除模表达式
void UnaryExp();					//一元表达式
void Stmt();              //语句
void assignStmt();        //赋值语句
void ifStmt();            //条件语句
void loopStmt();          //循环语句
void FuncRParams(string name);		  //值参数表
void returnStmt();        //返回语句
void EqExp();			  //相等性表达式
void RelExp();			  //关系表达式

//void Cond();              //条件表达式(逻辑或表达式)
//void LAndExp();			  //逻辑与表达式
//短路逻辑
void Cond(string label);              //条件表达式(逻辑或表达式)
void LAndExp(string label);			  //逻辑与表达式
int orlabelIndex = 0;		//生成or标签
int andlabelIndex = 0;		//生成and标签
string newName(string name, int blockindex)
{
	stringstream trans;          //数字和字符串相互转化渠道
	trans << blockindex;
	return name + "-" + trans.str();
}

int isinlineFunc = 0;		//1代表是，0代表不是
string newinlineName(string name, string Funcname)
{
	return name + "+" + Funcname;
}
/*
string newFuncName(string name)		//给函数名加后缀，防止与变量重名
{
	if (name == "putf"|| name == "starttime" || name == "stoptime" || name == "putarray"
		|| name == "putch" || name == "putint" || name == "getint" || name == "getarray" || name == "getch" || name == "main") {
		return name;
	}
	else {
		return name + ".1";
	}
}
*/
symbolTable checkTable(string checkname, int function_number, vector<int> fatherBlock);					//查表：改进中间代码和符号表时使用
void change(int index);				//修改中间代码、符号表
void putAllocGlobalFirst();		//将中间代码中alloc类型前移
void changeForInline(int index);		//如果为内联函数修改符号表和中间代码的名字

void youhuaDivCompare();		//将涉及到除法比较优化成乘法比较
bool isCompare(irCodeType type);

//相同表达式删除
int noWhileLabel;				//记录最后一条和while相关中间代码下标，在changeForInline函数内完成，节约一次遍历
map<string, int>  record;		//记录load或loadarr型变量出现次数
map<string, string> newVarName;	//记录新的变量名
map<string, int> maxLine;		//记录变量最先出现store对应行数
void deleteSameExp(int index);
bool isTemp(string a);
bool isNoChangeFunc(string a);		//跳转到该函数不会修改任何内容

//全局变量多次使用变成局部变量，可以避免多次LDR和STR
void changeGlobalToAlloc(int index);
void changeGlobalToAlloc2(int index);
int haveCall;			//该函数不能出现call类型中间代码，在changeForInline函数内完成，节约一次遍历

//局部数组基址和SP重合
map<string, int> matrixUseCount;
void addUseCount(int index);

//=============================================================================================================
//        以上为全局变量定义以及函数定义
//=============================================================================================================

int frontExecute(string syname)
{
	wordAnalysis.setFilepath(syname);
	//outfile.open("output.txt");
	//先读一个单词然后进入分析程序
	wordAnalysis.getsym();   //这里不需要使用其返回的bool类型值
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	CompUnit();
	//为所有VOID类型函数最后没有return语句的添加return
	for (int i = 0; i < codetotal.size(); i++) {
		vector<CodeItem> item = codetotal[i];
		int size = item.size();
		if (size > 0) {
			if (item[0].getCodetype() == DEFINE && item[0].getOperand1() == "void") {
				if (item[size - 1].getCodetype() != RET) {
					CodeItem citem = CodeItem(RET, "", "", "void");
					citem.setFatherBlock(fatherBlock);
					codetotal[i].push_back(citem);
				}
			}
		}
	}
	//自动为参数补上第一维大小
	for (int i = 0; i < total.size(); i++) {
		vector<symbolTable> item = total[i];
		for (int j = 0; j < item.size(); j++) {
			if (item[j].getForm() == PARAMETER) {
				int size = item[j].getDimension();
				if (size > 0) total[i][j].setfirstNodesize(0);
			}
		}
	}
	//符号表对名字加"@"和"%"
	for (int i = 0; i < total.size(); i++) {
		string b;
		for (int j = 0; j < total[i].size(); j++) {
			if (i == 0 || j == 0) {	//i=0为全局,j=0为函数
				b = "@";
			}
			else {
				b = "%";
			}
			total[i][j].changeName(b + total[i][j].getName());
		}
	}
	putAllocGlobalFirst();		//将中间代码中alloc、global类型前移
	youhuaDivCompare();				//除法比较进行优化
	for (int i = 1; i < codetotal.size(); i++) {
		changeGlobalToAlloc2(i);		//二次将全局变局部
	}
	func2tmpIndex.pop_back();
	func2tmpIndex.push_back(Temp);			//重新更新Temp上限
	//检测符号表内容
	/*
	cout << "名字 " << "Block下标 " << "种类 0Con 1Var 2Para 3Func " << "维度 " << endl;
	for (int i = 0; i < total.size(); i++) {
		vector<symbolTable> item = total[i];
		for (int j = 0; j < item.size(); j++) {
			if (item[j].getForm() == FUNCTION) {
				cout << item[j].getName() << " " << item[j].getForm() << " " << item[j].getparaLength() << endl;
			}
			else {
				cout << item[j].getName() << " " << item[j].getblockIndex() << " " << item[j].getForm() << " " << item[j].getDimension() << endl;
			}
		}
		cout << "\n";   //一个作用域换一行
	}
	*/
	//检查全局变量是否赋值
	/*
	vector<symbolTable> item = total[0];
	for (int j = 0; j < item.size(); j++) {
		if (item[j].getDimension() == 0) {
			cout << item[j].getName() << " " << numToString(item[j].getIntValue()[0]);
		}
		else {
			cout << item[j].getName();
			for (int p = 0; p < item[j].getIntValue().size(); p++) {
				cout << " " << item[j].getIntValue()[p];
			}
		}
		cout << "\n";   //一个元素换一行
	}
	*/
	//检测数组赋值正确性 testcase8
	/*
	for (int i = 0; i < total[0].size(); i++) {
		vector<int> values = total[0][i].getIntValue();
		for (int j = 0; j < values.size(); j++) {
			cout << values[j] << " ";
		}
		cout << "\n";
	}
	*/
	TestIrCode("irbeforeInline.txt");
	//changecodeForInline();
	//检测中间代码正确性
	//TestIrCode("irafterInline.txt");
	//outfile.close();
	cout << "yes" << endl;
	return 0;
}


void TestIrCode(string outputFile) {
	if (TIJIAO) {
		ofstream irtxt(outputFile);
		for (int i = 0; i < codetotal.size(); i++) {
			vector<CodeItem> item = codetotal[i];
			for (int j = 0; j < item.size(); j++) {
				//cout << item[j].getContent() << endl;
				irtxt << item[j].getContent() << endl;
			}
			//cout << "\n";
			irtxt << "\n";
		}
		irtxt.close();
	}
}

void CompUnit()
{
	vector<symbolTable> global;
	total.push_back(global);
	vector<CodeItem> global1;
	codetotal.push_back(global1);
	map<string, int> globalll;
	names.push_back(globalll);
	while (symbol == INTTK || symbol == CONSTTK || symbol == VOIDTK) {
		if (symbol == CONSTTK) {    //常量声明
			ConstDecl(0, 0);
		}
		else {   //变量声明或者函数声明                
			if (symbol == INTTK) {
				Memory sym_tag = symbol;
				string token_tag = wordAnalysis.getToken();
				int record_tag = wordAnalysis.getfRecord();
				wordAnalysis.getsym();   //标识符
				wordAnalysis.getsym();   //是(则为有返回值函数定义
				symbol = wordAnalysis.getSymbol();
				//恢复出厂设置
				wordAnalysis.setfRecord(record_tag);
				wordAnalysis.setSymbol(sym_tag);
				wordAnalysis.setToken(token_tag);
				if (symbol == LPARENT) {   //函数声明
					symbol = sym_tag;
					Funcindex++;
					func2tmpIndex.push_back(Temp);
					vector<symbolTable> item;
					total.push_back(item);
					vector<CodeItem> item1;
					codetotal.push_back(item1);
					map<string, int> item2;
					names.push_back(item2);
					isinlineFunc = 1;
					valueFuncDef();
					change(Funcindex);		//修改中间代码、符号表
					changeForInline(Funcindex);
					total[Funcindex][0].setisinlineFunc(isinlineFunc);
					changeGlobalToAlloc(Funcindex);
					deleteSameExp(Funcindex);
					addUseCount(Funcindex);
				}
				else {
					symbol = sym_tag;
					VarDecl(0, 0);
				}
			}
			else {
				Funcindex++;
				func2tmpIndex.push_back(Temp);
				vector<symbolTable> item;
				total.push_back(item);
				vector<CodeItem> item1;
				codetotal.push_back(item1);
				map<string, int> item2;
				names.push_back(item2);
				isinlineFunc = 1;
				novalueFuncDef();
				change(Funcindex);			//修改中间代码、符号表
				changeForInline(Funcindex);
				total[Funcindex][0].setisinlineFunc(isinlineFunc);
				changeGlobalToAlloc(Funcindex);
				deleteSameExp(Funcindex);
				addUseCount(Funcindex);
			}
		}
	}
	func2tmpIndex.push_back(Temp);
	//outfile << "<编译单元>" << endl;
}
void ConstDecl(int index, int block) 		   //常量说明
{
	//printMessage();    //输出const信息
	//判断symbol==CONSTTK
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	//printMessage();    //输出int信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //预读
	ConstDef(index, block);  //常量定义
	//判断symbol=;
	while (symbol == COMMA) {
		//printMessage();   //输出,信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();
		ConstDef(index, block);  //常量定义
	}
	//printMessage();    //输出；信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //预读
	//退出循环前已经预读单词
	//outfile << "<常量说明>" << endl;
}
void ConstDef(int index, int block)			   //常量定义
{
	int value;
	vector<int> length; //记录每个维度大小
	//printMessage();   //输出标识符信息
	string name = token;	//保存常量名
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //预读
	int dimenson = 0;
	while (symbol == LBRACK) {
		dimenson++;
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();  //预读
		length.push_back(ConstExp());   //记录常量维度
		//printMessage();   //输出]信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //预读
	}
	symbolTable item = symbolTable(CONSTANT, INT, name, dimenson, block);
	item.setMatrixLength(length, index);
	total[index].push_back(item);
	names[index][name]++;
	//printMessage();   //输出=信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //预读


	//常量赋值前做好变量初始化相关工作
	offset = 0;
	matrixLength = length;
	Ndimension = 0;
	isNesting = 0;
	isBegin = 0;
	totalSize = 1;
	nodeName = name;
	for (int pp = 0; pp < matrixLength.size(); pp++) {
		totalSize = totalSize * matrixLength[pp];
	}
	irCodeType codetype;
	string b;
	string constValue = "";
	if (index != 0) {
		codetype = ALLOC;
		b = "%";
	}
	else {
		codetype = GLOBAL;
		b = "@";
	}
	if (matrixLength.size() > 0) {		//如果是数组，constValue没意义，放在ConstInitVal前面可以实现先声明数组后赋值的效果
		CodeItem citem = CodeItem(codetype, b + name, constValue, numToString(totalSize));
		citem.setFatherBlock(fatherBlock);
		codetotal[index].push_back(citem);
	}
	ConstInitVal(index);
	if (matrixLength.size() <= 0) {   //如果不是数组，必须放在ConstInitVal后面，这样才能知道constValue大小
		int size = total[index].size();
		constValue = numToString(total[index][size - 1].getIntValue()[0]);
		CodeItem citem = CodeItem(codetype, b + name, constValue, numToString(totalSize));
		citem.setFatherBlock(fatherBlock);
		codetotal[index].push_back(citem);
	}
	//退出循环前均已经预读单词
	//outfile << "<常量定义>" << endl;
}
int ConstExp()
{
	int valueL, valueR;
	valueL = ConstMulExp();  //退出前读了一个单词
	while (symbol == PLUS || symbol == MINU) {
		Memory tag = symbol;
		//printMessage();    //输出+或-信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		valueR = ConstMulExp();
		if (tag == PLUS) valueL = valueL + valueR;
		else valueL = valueL - valueR;
	}
	//退出前已经预读
	//outfile << "<常量表达式>" << endl;
	return valueL;
}
int ConstMulExp()
{
	int valueL, valueR;
	valueL = ConstUnaryExp();
	while (symbol == MULT || symbol == DIV_WORD || symbol == MOD) {
		Memory tag = symbol;
		//printMessage();    //输出*或/信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		valueR = ConstUnaryExp();
		if (tag == MULT) valueL = valueL * valueR;
		else if (tag == DIV_WORD) valueL = valueL / valueR;
		else valueL = valueL % valueR;
	}
	//退出前已经预读
	//outfile << "<常量项>" << endl;
	return valueL;
}
int ConstUnaryExp()	//'(' Exp ')' | LVal | Number | Ident '(' [FuncRParams] ')' | +|- UnaryExp 			
{				//Ident '(' [FuncRParams] ')'不可能有
	int value;
	if (symbol == LPARENT) {  //'(' Exp ')'
		//printMessage();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //预读
		value = ConstExp();
		//printMessage();			//输出)
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //预读
	}
	else if (symbol == INTCON) {      //symbol == INTCON
		value = wordAnalysis.getNumber();   //获取数值
		//printMessage();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //预读
	}
	else if (symbol == PLUS || symbol == MINU) {  //+|- UnaryExp
		int flag = 1;
		if (symbol == MINU) flag = -1;
		//printMessage();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //预读
		value = flag * ConstUnaryExp();
	}
	else {	//Ident{'[' Exp ']'}  要求1.Ident一定是常量 2.能直接求值(应该是ConstExp) 
		string name_tag = wordAnalysis.getToken();
		wordAnalysis.getsym();
		int totaloffset = 0;
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		symbolTable item = checkItem(name_tag);
		vector<int> dimenLength = item.getMatrixLength();
		int nowDimenson = 0;
		while (symbol == LBRACK) {
			nowDimenson++;
			//printMessage();//输出[信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
			value = ConstExp();
			int offset = 1;
			for (int j = nowDimenson; j < dimenLength.size(); j++) {
				offset = offset * dimenLength[j];
			}
			totaloffset += value * offset;
			//printMessage();//输出]信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
		}
		value = item.getIntValue()[totaloffset];
	}
	//outfile << "<常量因子>" << endl;
	return value;
}
void ConstInitVal(int index)	//ConstExp | '{' [ConstInitVal{ ',' ConstInitVal } ] '}'  
{
	/*
	matrixLength;		//当前数组每个维度大小
	offset;				//偏移量
	Ndimension;			//当前维度
	*/
	//positionRecord有两种情况可以增加，1是读到}，2是读到{ offset!=0
	int value;
	int flag = 0;
	if (symbol == LBRACE) {
		int mod = 1;     //偏移量
		if (isNesting == 1) {
			isNesting = 1;
			Ndimension++;
			for (int pp = Ndimension; pp < matrixLength.size(); pp++) {   //计算当前维度偏移量mod
				mod = mod * matrixLength[pp];
			}
		}
		else {
			if (isBegin == 0) {
				isBegin = 1;
				isNesting = 0;
			}
			else {
				isNesting = 1;
			}
			if (matrixLength.size() == 1) {		//一维数组偏移量单独算
				mod = matrixLength[0];
			}
			else {
				for (int pp = 1; pp < matrixLength.size(); pp++) {   //计算当前维度偏移量mod
					mod = mod * matrixLength[pp];
				}
			}
			Ndimension = 1;
			while (offset % mod != 0) {
				Ndimension++;
				mod = mod / matrixLength[Ndimension - 1];
			}
		}
		//printMessage();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //预读
		if (symbol != RBRACE) {
			ConstInitVal(index);
			while (symbol == COMMA) {   //读到逗号需要判断左右两个数的维度
				isNesting = 0;
				//printMessage();
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken(); //预读
				ConstInitVal(index);
			}
		}
		else {    //空情况
			flag = 1;
		}
		if (flag == 1) {
			string b = "@";
			symbolTable item = checkItem(nodeName);
			if (Range != 0) {
				b = "%";
			}
			if (b == "%") {		//全局变量不用出现STOREARR 0,因为后端对于全局默认没有就是0
				int zzzzz = offset + mod;
				while (offset < zzzzz) {
					offset++;
					string offset_string = numToString((offset - 1) * 4);
					string b = "@";
					symbolTable item = checkItem(nodeName);
					if (Range != 0) {
						b = "%";
					}
					CodeItem citem = CodeItem(STOREARR, "0", b + nodeName, offset_string);
					citem.setFatherBlock(fatherBlock);
					codetotal[index].push_back(citem);
				}
			}
			else {
				offset += mod;
			}
		}
		else {
			string b = "@";
			symbolTable item = checkItem(nodeName);
			if (Range != 0) {
				b = "%";
			}
			if (b == "@") {     //全局变量不用出现STOREARR 0,因为后端对于全局默认没有就是0
				while (offset % mod != 0) {
					offset++;
				}
			}
			else {
				while (offset % mod != 0) {
					offset++;
					string offset_string = numToString((offset - 1) * 4);
					string b = "@";
					symbolTable item = checkItem(nodeName);
					if (Range != 0) {
						b = "%";
					}
					CodeItem citem = CodeItem(STOREARR, "0", b + nodeName, offset_string);
					citem.setFatherBlock(fatherBlock);
					codetotal[index].push_back(citem);
				}
			}
		}
		//printMessage();   //输出 } 信息
		//每次读到 } ，偏移量offset变成lastDimension倍数即可
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //预读
	}
	else {
		isNesting = 0;
		offset++;
		value = ConstExp();
		int size = total[index].size() - 1;
		//cout << value << endl;
		if (matrixLength.size() == 0) {
			total[index][size].setIntValue(value, offset - 1); //赋值
		}
		else {
			if (offset <= totalSize) {
				total[index][size].setIntValue(value, offset - 1); //赋值
				string offset_string = numToString((offset - 1) * 4);
				string value_string = numToString(value);
				string b = "@";
				symbolTable item = checkItem(nodeName);
				if (Range != 0) {
					b = "%";
				}
				CodeItem citem = CodeItem(STOREARR, value_string, b + nodeName, offset_string);
				citem.setFatherBlock(fatherBlock);
				codetotal[index].push_back(citem);
			}
		}
	}
	//outfile << "<常量初值>" << endl;
}
void VarDecl(int index, int block)			   //变量说明
{
	//printMessage();   //输出int信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读 
	VarDef(index, block);   //已经预读一个单词，直接进入即可
	while (symbol == COMMA) {
		//printMessage();    //输出,信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读 
		VarDef(index, block);   //已经预读一个单词，直接进入即可
	}
	//printMessage();    //输出；信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读 
	//退出循环前均已经预读单词
	//outfile << "<变量说明>" << endl;
}
void VarDef(int index, int block)             //变量定义
{
	int value;
	vector<int> length; //记录每个维度大小
	//printMessage();	//输出变量名信息
	string name = token;	//记录变量名
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();  //预读
	int dimenson = 0;			//维度信息
	while (symbol == LBRACK) {
		dimenson++;
		//printMessage();   //输出[
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		length.push_back(ConstExp());   //记录常量维度
		//printMessage();   //输出]
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	if (dimenson > 0) {
		matrixUseCount[name] = 0;		//初始化计数次数
	}
	symbolTable item = symbolTable(VARIABLE, INT, name, dimenson, block);
	item.setMatrixLength(length, index);
	total[index].push_back(item);
	names[index][name]++;
	totalSize = 1;
	for (int pp = 0; pp < length.size(); pp++) {
		totalSize = totalSize * length[pp];
	}
	irCodeType codetype;
	string b;
	if (index != 0) {
		codetype = ALLOC;
		b = "%";
	}
	else {
		codetype = GLOBAL;
		b = "@";
	}
	if (b == "@" && dimenson == 0) {			//全局变量初值默认为0
		int size = total[index].size() - 1;
		total[index][size].setIntValue(0, 0); //赋值
		CodeItem citem = CodeItem(codetype, b + name, "0", numToString(totalSize));
		citem.setFatherBlock(fatherBlock);
		codetotal[index].push_back(citem);
	}
	else {		//局部变量或数组做变量，没有初值
		CodeItem citem = CodeItem(codetype, b + name, "", numToString(totalSize));
		citem.setFatherBlock(fatherBlock);
		codetotal[index].push_back(citem);
	}
	if (symbol == ASSIGN) {
		//printMessage();   //输出=
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读

		if (b == "%" && dimenson > 0) {		//局部数组调用初始化
			CodeItem citem = CodeItem(ARRAYINIT, "0", b + name, numToString(totalSize));
			citem.setFatherBlock(fatherBlock);
			codetotal[index].push_back(citem);
		}
		//变量赋值前做好变量初始化相关工作
		offset = 0;
		matrixLength = length;
		Ndimension = 0;
		isNesting = 0;
		isBegin = 0;
		nodeName = name;

		InitVal(index);
	}/*
	else {		//自动给未初始化的局部变量赋0，正常情况下不用加
		if (b == "%" && dimenson == 0) {
			CodeItem citem = CodeItem(STORE, "0", b + name, "");	//赋值单值
			citem.setFatherBlock(fatherBlock);
			codetotal[index].push_back(citem);
		}
	}*/

	//退出循环前已经预读单词
	//outfile << "<变量定义>" << endl;
}
void InitVal(int index)
{
	int flag = 0;
	if (symbol == LBRACE) {
		int mod = 1;
		if (isNesting == 1) {
			isNesting = 1;
			Ndimension++;
			for (int pp = Ndimension; pp < matrixLength.size(); pp++) {   //计算当前维度偏移量mod
				mod = mod * matrixLength[pp];
			}
		}
		else {
			if (isBegin == 0) {
				isBegin = 1;
				isNesting = 0;
			}
			else {
				isNesting = 1;
			}
			if (matrixLength.size() == 1) {		//一维数组偏移量单独算
				mod = matrixLength[0];
			}
			else {
				for (int pp = 1; pp < matrixLength.size(); pp++) {   //计算当前维度偏移量mod
					mod = mod * matrixLength[pp];
				}
			}
			Ndimension = 1;
			while (offset % mod != 0) {
				Ndimension++;
				mod = mod / matrixLength[Ndimension - 1];
			}
		}
		//printMessage();   //输出{
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		if (symbol != RBRACE) {
			InitVal(index);
			while (symbol == COMMA) {
				isNesting = 0;
				//printMessage();   //输出,
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//预读
				InitVal(index);
			}
		}
		else {    //空情况
			flag = 1;
		}
		if (flag == 1) {
			string b = "@";
			symbolTable item = checkItem(nodeName);
			if (Range != 0) {
				b = "%";
			}
			if (b == "%") {			//全局变量不用出现STOREARR 0,因为后端对于全局默认没有就是0
				int zzzzz = offset + mod;		//局部变量也不会出现STOREARR0，因为ARRAYINIT存在
			}
			else {     //全局变量往符号表里赋值
				offset += mod;
			}
		}
		else {
			string b = "@";
			symbolTable item = checkItem(nodeName);
			if (Range != 0) {
				b = "%";
			}
			if (b == "@") {			//全局变量不用出现STOREARR 0,因为后端对于全局默认没有就是0
				while (offset % mod != 0) {
					offset++;
				}
			}
			else {
				while (offset % mod != 0) {  //局部变量也不会出现STOREARR0，因为ARRAYINIT存在
					offset++;
				}
			}
		}
		//printMessage();   //输出}
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else {
		isNesting = 0;
		offset++;
		string b = "@";
		symbolTable item = checkItem(nodeName);
		if (Range != 0) {
			b = "%";
		}
		if (matrixLength.size() == 0) {
			if (b == "%") {
				Exp();
				CodeItem citem = CodeItem(STORE, interRegister, b + nodeName, "");	//赋值单值
				citem.setFatherBlock(fatherBlock);
				codetotal[index].push_back(citem);
			}
			else {  //全局变量且带初始化定义需改成 global     @b         value         1 的形式   
				Exp();
				//interRegister = numToString(ConstExp());
				int nowsize = codetotal[index].size(); //全局变量的Exp()一定是constExp()
				CodeItem citem1 = codetotal[index][nowsize - 1];
				citem1.changeContent(citem1.getResult(), interRegister, citem1.getOperand2());
				codetotal[index].pop_back();
				codetotal[index].push_back(citem1);
				int size = total[index].size() - 1;
				total[index][size].setIntValue(stringToNum(interRegister), 0); //赋值
			}
		}
		else {
			if (offset <= totalSize) {
				string offset_string = numToString((offset - 1) * 4);
				string b = "@";
				symbolTable item = checkItem(nodeName);
				if (Range != 0) {
					b = "%";
				}
				if (b == "%") {
					Exp();
				}
				else {
					Exp();
					//interRegister = numToString(ConstExp());
				}
				CodeItem citem = CodeItem(STOREARR, interRegister, b + nodeName, offset_string);
				citem.setFatherBlock(fatherBlock);
				codetotal[index].push_back(citem);
				if (b == "@") {		//全局变量赋值到符号表
					int size = total[index].size() - 1;
					total[index][size].setIntValue(stringToNum(interRegister), offset - 1); //赋值
				}
			}
		}
	}
	//outfile << "<变量初值>" << endl;
}
void valueFuncDef()    //有返回值函数定义
{
	//printMessage();    //输出int信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	string name = token;			//获取函数名
	//name = newFuncName(name);   //获取函数名并改名
	//printMessage();   //获得标识符并输出
	symbolTable item(FUNCTION, INT, name);
	total[Funcindex].push_back(item);
	CodeItem citem(DEFINE, "@" + name, "int", "");           //define @foo int
	codetotal[Funcindex].push_back(citem);
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	//printMessage();   //获得(并输出
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	if (symbol == RPARENT) {  //参数表为空
		total[Funcindex][0].setparaLength(0);   //函数参数为0
		//outfile << "<参数表>" << endl;
		//printMessage();    //输出)信息
	}
	else {
		fatherBlock.push_back(1);
		FuncFParams();  //进入前已经预读
		//printMessage();    //输出)信息
		fatherBlock.pop_back();
		//判断symbol=)
	}
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //预读
	Blockindex = 0;
	Block();
	//outfile << "<有返回值函数定义>" << endl;
}
void novalueFuncDef()  //无返回值函数定义
{
	//printMessage();    //输出void信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//获得标识符
	string name = token;  //保存函数名
	//name = newFuncName(name);		函数名改名
	symbolTable item(FUNCTION, VOID, name);
	total[Funcindex].push_back(item);
	CodeItem citem(DEFINE, "@" + name, "void", "");           //define @foo int
	codetotal[Funcindex].push_back(citem);
	//printMessage();   //输出标识符
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	//printMessage();   //获得(并输出
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	if (symbol == RPARENT) {
		total[Funcindex][0].setparaLength(0);   //函数参数为0
		//outfile << "<参数表>" << endl;
		//printMessage();    //先输出参数表后输出)信息
	}
	else {
		fatherBlock.push_back(1);
		FuncFParams();
		//printMessage();    //输出)信息
		fatherBlock.pop_back();
		//判断)
	}
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	Blockindex = 0;
	Block();
	//outfile << "<无返回值函数定义>" << endl;
}
void Block()      //语句块
{
	Blockindex++;
	fatherBlock.push_back(Blockindex);
	int block = Blockindex;
	//printMessage();    //输出{信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	while (symbol != RBRACE) {
		if (symbol == CONSTTK) {
			ConstDecl(Funcindex, block);
		}
		else if (symbol == INTTK) {
			VarDecl(Funcindex, block);
		}
		else {
			Stmt();
		}
	}
	//printMessage();    //输出}信息
	fatherBlock.pop_back();    //删除末尾元素
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	//outfile << "<语句块>" << endl;
}
void FuncFParams()         //参数表
{
	int paraLength = 1;//参数个数   
	FuncFParam();
	while (symbol == COMMA) {
		paraLength++;
		//printMessage();    //输出,信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();
		FuncFParam();
	}
	total[Funcindex][0].setparaLength(paraLength);
	//退出前已经预读
	//outfile << "<函数形参表>" << endl;
}
void FuncFParam()
{
	vector<int> length; //记录每个维度大小
	//printMessage();    //输出int信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	string name = token;   //保存参数名
	//printMessage();    //输出变量名信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	int dimenson = 0;
	if (symbol == LBRACK) {
		dimenson++;
		//printMessage();    //输出[信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();
		//printMessage();    //输出]信息
		length.push_back(0);
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();
		while (symbol == LBRACK) {
			dimenson++;
			//printMessage();    //输出[信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
			length.push_back(ConstExp());
			//printMessage();    //输出]信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();  //预读
		}
	}
	symbolTable item = symbolTable(PARAMETER, INT, name, dimenson, 1);
	item.setMatrixLength(length, Funcindex);
	total[Funcindex].push_back(item);
	names[Funcindex][name]++;
	string b = "int";
	if (dimenson > 0) {
		b = "int*";
	}
	CodeItem citem = CodeItem(PARA, "%" + name, b, numToString(dimenson));//函数形参
	citem.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem);
	//outfile << "<函数形参>" << endl;
}
void Exp()	//MulExp{('+'|'-')MulExp}
{
	Memory symbol_tag;
	string registerL, registerR;
	MulExp();  //退出前读了一个单词
	registerL = interRegister;
	while (symbol == PLUS || symbol == MINU) {
		symbol_tag = symbol;
		//printMessage();    //输出+或-信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		MulExp();
		registerR = interRegister;
		if (registerL[0] != '@' && registerL[0] != '%' && registerR[0] != '@' && registerR[0] != '%') {
			int value;
			int valueL = stringToNum(registerL);
			int valueR = stringToNum(registerR);
			if (symbol_tag == PLUS) value = valueL + valueR;
			else value = valueL - valueR;
			interRegister = numToString(value);
		}
		else {
			interRegister = "%" + numToString(Temp);
			Temp++;
			irCodeType codetype;
			if (symbol_tag == PLUS) codetype = ADD;
			else codetype = SUB;
			CodeItem citem = CodeItem(codetype, interRegister, registerL, registerR);
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
		registerL = interRegister;
	}
	//退出前已经预读
	//outfile << "<表达式>" << endl;
	return;
}
void MulExp()	//UnaryExp {('*'|'/'|'%') UnaryExp  }
{
	UnaryExp();
	string registerL = interRegister;	//获取左计算数
	while (symbol == MULT || symbol == DIV_WORD || symbol == MOD) {
		Memory symbol_tag = symbol;
		//printMessage();    //输出*或/信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		UnaryExp();
		string registerR = interRegister;	//获取右计算数，从此开始
		if (registerL[0] != '@' && registerL[0] != '%' && registerR[0] != '@' && registerR[0] != '%') {
			int value;
			int valueL = stringToNum(registerL);
			int valueR = stringToNum(registerR);
			if (symbol_tag == MULT) value = valueL * valueR;
			else if (symbol_tag == DIV_WORD) value = valueL / valueR;
			else value = valueL % valueR;
			interRegister = numToString(value);
		}
		else {
			interRegister = "%" + numToString(Temp);
			Temp++;
			irCodeType codetype;
			if (symbol_tag == MULT) codetype = MUL;
			else if (symbol_tag == DIV_WORD) codetype = DIV;
			else codetype = REM;
			CodeItem citem = CodeItem(codetype, interRegister, registerL, registerR);
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
		registerL = interRegister;
	}
	//退出前已经预读
	//outfile << "<项>" << endl;
}
void UnaryExp()			// '(' Exp ')' | LVal | Number | Ident '(' [FuncRParams] ')' | +|-|! UnaryExp 
{
	if (symbol == LPARENT) {   //'(' Exp ')'
		//printMessage();    //输出(信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		Exp();
		//printMessage();    //输出)信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else if (symbol == INTCON) {  //number
		//printMessage();
		interRegister = numToString(wordAnalysis.getNumber());		//将数值转换成字符串模式
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else if (symbol == IDENFR) {  //标识符 (函数实参表) | 标识符 {'['表达式']'}
		//printMessage();    //输出变量信息
		string name_tag = wordAnalysis.getToken();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		string registerL, registerR, registerA, registerC;
		int typeL, typeR, typeA, typeC;
		string Functionname;
		if (symbol == LPARENT) {  //标识符 (函数实参表)
			paraNum = 0;
			Functionname = name_tag;
			//Functionname = newFuncName(Functionname);	获取函数名并改名
			//printMessage();    //输出(信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
			if (Functionname == "putf" || Functionname == "starttime" || Functionname == "stoptime") {
				if (Functionname == "putf") {
					Functionname = "printf";
				}
				else if (Functionname == "starttime") {
					Functionname = "_sysy_starttime";
				}
				else {
					Functionname = "_sysy_stoptime";
				}
			}
			CodeItem citem1 = CodeItem(NOTE, "@" + Functionname, "func", "begin");          //call @foo %3 3
			citem1.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem1);//函数开始注释
			if (Functionname == "_sysy_starttime" || Functionname == "_sysy_stoptime") {
				CodeItem citem = CodeItem(PUSH, "int", numToString(wordAnalysis.getlineNumber()), "1");  //传参
				citem.setFatherBlock(fatherBlock);
				citem.setFuncName("@" + Functionname);
				codetotal[Funcindex].push_back(citem);
			}
			if (symbol != RPARENT) {
				FuncRParams("@" + Functionname);
			}
			//printMessage();    //输出)信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
			interRegister = "%" + numToString(Temp);		//存函数返回结果
			Temp++;
			CodeItem citem2 = CodeItem(CALL, interRegister, "@" + Functionname, numToString(paraNum));          //call @foo %3 3
			citem2.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem2);//函数引用
			if (Functionname != "printf" && Functionname != "_sysy_starttime" && Functionname != "_sysy_stoptime"
				&& Functionname != "putarray" && Functionname != "putch" && Functionname != "putint"
				&& Functionname != "getint" && Functionname != "getarray" && Functionname != "getch") {
				isinlineFunc = 0;  //当前函数不再是内联函数(即该函数调用除非特殊函数外的函数)  
			}
			/*CodeItem citem3 = CodeItem(MOV,"", interRegister, "%" + numToString(Temp));          //call @foo %3 3
			citem3.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem3);//函数引用
			interRegister = "%" + numToString(Temp);
			Temp++;*/
			CodeItem citem4 = CodeItem(NOTE, "@" + Functionname, "func", "end" + numToString(paraNum));          //call @foo %3 3
			citem4.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem4);//函数结束注释
		}
		else {  //标识符 {'['表达式']'}
			symbolTable item = checkItem(name_tag);
			int range = Range;
			int paraOrVar = item.getForm();    //数组做参数需要区分变量还是参数
			vector<int> dimenLength = item.getMatrixLength();
			if (dimenLength.size() > 0) {
				registerA = "0";	//偏移量为0
			}
			int nowDimenson = 0;   //记录当前维度
			string b = "%";
			if (range == 0) {
				b = "@";	//全局变量
			}
			CodeItem citem1 = CodeItem(NOTE, b + name_tag, "array", "begin");
			citem1.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem1);
			while (symbol == LBRACK) {
				nowDimenson++;
				//printMessage();    //输出[信息
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//预读
				Exp();		//q[n][m]
				registerL = interRegister;   //registerL = "n"	，该维度偏移量
				int offset = 1;
				for (int j = nowDimenson; j < dimenLength.size(); j++) {
					offset = offset * dimenLength[j];
				}
				offset = offset * 4;	//偏移量直接乘4，这样最后就少乘1个4
				registerR = numToString(offset);		//该维度值为1时大小(已乘4)
				if (registerL[0] != '@' && registerL[0] != '%') {   //该维度总大小，存在registerC中
					int valueL = stringToNum(registerL);
					valueL = valueL * offset;
					registerC = numToString(valueL);
				}
				else {
					registerC = "%" + numToString(Temp);
					Temp++;
					CodeItem citem = CodeItem(MUL, registerC, registerL, registerR);
					citem.setFatherBlock(fatherBlock);
					codetotal[Funcindex].push_back(citem);
				}
				if (registerC[0] != '@' && registerC[0] != '%' && registerA[0] != '@' && registerA[0] != '%') {	//当前偏移量registerA + 该维度总偏移量registerC
					int valueC = stringToNum(registerC);
					int valueA = stringToNum(registerA);
					registerA = numToString(valueC + valueA);
				}
				else {
					interRegister = "%" + numToString(Temp);
					Temp++;
					CodeItem citem = CodeItem(ADD, interRegister, registerA, registerC);
					citem.setFatherBlock(fatherBlock);
					codetotal[Funcindex].push_back(citem);
					registerA = interRegister;
				}
				//printMessage();    //输出]信息
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//预读
			}
			int nowSize = codetotal[Funcindex].size();
			if (codetotal[Funcindex][nowSize - 1].getOperand1() == "array" && codetotal[Funcindex][nowSize - 1].getOperand2() == "begin"
				&& codetotal[Funcindex][nowSize - 1].getResult() == b + name_tag) {	//计算偏移的注释没用
				codetotal[Funcindex].pop_back();
			}
			else {
				CodeItem citem2 = CodeItem(NOTE, b + name_tag, "array", "end");
				citem2.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem2);
			}
			if (item.getDimension() == nowDimenson && item.getDimension() > 0) {	//取多维数组的一个元素
				if (item.getForm() == CONSTANT && registerA[0] != '@' && registerA[0] != '%') {
					int offset = stringToNum(registerA) / 4;
					interRegister = numToString(item.getIntValue()[offset]);
				}
				else {
					interRegister = "%" + numToString(Temp);
					Temp++;
					CodeItem citem = CodeItem(LOADARR, interRegister, b + name_tag, registerA); //数组取值
					citem.setFatherBlock(fatherBlock);    //保存当前作用域
					codetotal[Funcindex].push_back(citem);
				}
			}
			else if (item.getDimension() > 0 && nowDimenson < item.getDimension()) {		//此时只可能是传参，传数组的一部分
				interRegister = "%" + numToString(Temp);
				Temp++;
				//registerA是偏移量
				string opr2;
				if (paraOrVar == PARAMETER) {
					opr2 = "para";
				}
				else {
					opr2 = "array";
				}
				CodeItem citem = CodeItem(LOAD, interRegister, b + name_tag, opr2); //一维变量、常量取值		获取数组首地址
				citem.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem);
				CodeItem citem2 = CodeItem(ADD, "%" + numToString(Temp), interRegister, registerA);
				citem.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem2);
				interRegister = "%" + numToString(Temp);
				Temp++;
				paraIntNode = 1;
			}
			else {		//此时是正常变量，不是数组变量
				if (item.getForm() == CONSTANT) {
					interRegister = numToString(item.getIntValue()[0]);		//将数值转换成字符串模式
				}
				else {
					interRegister = "%" + numToString(Temp);
					Temp++;
					string b = "%";
					if (range == 0) {
						b = "@";	//全局变量
					}
					CodeItem citem = CodeItem(LOAD, interRegister, b + name_tag, ""); //一维变量、常量取值
					citem.setFatherBlock(fatherBlock);
					codetotal[Funcindex].push_back(citem);
				}
			}
		}
	}
	else { //+| -| !  UnaryExp
		string op, registerL, registerR;
		int typeL, typeR;
		if (symbol == PLUS) {
			//printMessage();    //输出+信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
			UnaryExp();
		}
		else if (symbol == MINU) {
			//printMessage();    //输出-信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
			UnaryExp();
			registerR = interRegister;
			if (registerR[0] == '%' || registerR[0] == '@') {
				interRegister = "%" + numToString(Temp);
				Temp++;
				CodeItem citem = CodeItem(SUB, interRegister, "0", registerR);
				citem.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem);
			}
			else {
				int value = -1 * stringToNum(registerR);
				interRegister = numToString(value);
			}
		}
		else {
			//printMessage();    //输出!信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
			UnaryExp();
			registerL = interRegister;
			if (registerL[0] == '%' || registerL[0] == '@') {
				interRegister = "%" + numToString(Temp);
				Temp++;
				CodeItem citem = CodeItem(NOT, interRegister, registerL, "");//not res ope1
				citem.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem);
			}
			else {
				int value = stringToNum(registerL);
				if (value == 0) {
					value = 1;
				}
				else {
					value = 0;
				}
				interRegister = numToString(value);
			}
		}
	}
	//退出前已经预读
	//outfile << "<因子>" << endl;
}
void Stmt()              //语句
{
	if (symbol == IFTK) {  //＜条件语句＞
		ifStmt();
	}
	else if (symbol == WHILETK) {  //<循环语句>
		loopStmt();
	}
	else if (symbol == BREAKTK) {	//break ;
		//printMessage();    //输出break信息
		CodeItem citem = CodeItem(BR, "", whileLabel[whileLabel.size() - 2], ""); //br %while.cond 
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		//printMessage();    //输出;信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else if (symbol == CONTINUETK) {	//continue ;
		//printMessage();    //输出continue信息
		CodeItem citem = CodeItem(BR, "", whileLabel[whileLabel.size() - 1], "1"); //br %while.cond 
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		//printMessage();    //输出;信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else if (symbol == LBRACE) { //'{'｛声明|语句｝'}'
		Block();
	}
	else if (symbol == RETURNTK) { //＜返回语句＞;
		returnStmt();
		//printMessage();    //输出;信息
		//判断symbol=;
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else if (symbol == SEMICN) {
		//printMessage();    //输出;信息
		//判断symbol=;
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else if (symbol == IDENFR) {
		Memory sym_tag = symbol;
		string token_tag = wordAnalysis.getToken();
		int record_tag = wordAnalysis.getfRecord();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();  //预读
		while (symbol != SEMICN && symbol != ASSIGN) {
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
		}
		//恢复出厂设置
		wordAnalysis.setfRecord(record_tag);
		wordAnalysis.setSymbol(sym_tag);
		wordAnalysis.setToken(token_tag);
		if (symbol == ASSIGN) {
			symbol = sym_tag;
			assignStmt();
			//printMessage();    //输出;信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
		}
		else {
			symbol = sym_tag;
			int beforeSize = codetotal[Funcindex].size();		//获取当前中间代码下标
			Exp();					//CALL函数在EXP中存在
			//printMessage();  //输出分号
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();  //预读
			//开启检查，如果没有出现CALL类型的中间代码，这些中间代码均可删除
			int nouse = 1;
			int afterSize = codetotal[Funcindex].size();		//获取当前中间代码下标
			for (int aa = beforeSize; aa < afterSize; aa++) {
				if (codetotal[Funcindex][aa].getCodetype() == CALL) {
					nouse = 0;
					break;
				}
			}
			if (nouse == 1) {
				while (afterSize > beforeSize) {
					codetotal[Funcindex].pop_back();
					afterSize--;
				}
			}
		}
		/*
		if (symbol == LPARENT) {	//该语句为调用无返回值函数；
			wordAnalysis.setfRecord(record_tag);
			wordAnalysis.setSymbol(sym_tag);
			wordAnalysis.setToken(token_tag);
			symbol = sym_tag;
			Exp();					//CALL函数在EXP中存在
			//printMessage();  //输出分号
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();  //预读
		}
		else {
			while (symbol != SEMICN && symbol != ASSIGN) {
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
			}
			//恢复出厂设置
			wordAnalysis.setfRecord(record_tag);
			wordAnalysis.setSymbol(sym_tag);
			wordAnalysis.setToken(token_tag);
			if (symbol == ASSIGN) {  	//LVal = Exp;
				symbol = sym_tag;
				assignStmt();
				//printMessage();    //输出;信息
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//预读
			}
			else {		//Exp ;  这里直接跳过
				while (symbol != SEMICN) {
					wordAnalysis.getsym();
					symbol = wordAnalysis.getSymbol();
				}
				//printMessage();    //输出;信息
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//预读
			}
		}*/
	}
	else {						//Exp ; 这里直接跳过，没影响
		int beforeSize = codetotal[Funcindex].size();		//获取当前中间代码下标	
		Exp();
		//printMessage();  //输出分号
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();  //预读
		//开启检查，如果没有出现CALL类型的中间代码，这些中间代码均可删除
		int nouse = 1;
		int afterSize = codetotal[Funcindex].size();		//获取当前中间代码下标
		for (int aa = beforeSize; aa < afterSize; aa++) {
			if (codetotal[Funcindex][aa].getCodetype() == CALL) {
				nouse = 0;
				break;
			}
		}
		if (nouse == 1) {
			while (afterSize > beforeSize) {
				codetotal[Funcindex].pop_back();
				afterSize--;
			}
		}
		/*
		while (symbol != SEMICN) {
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
		}
		//printMessage();    //输出;信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		*/
	}
	//退出前均预读
	//outfile << "<语句>" << endl;
}
void assignStmt()        //赋值语句 LVal = Exp
{
	//printMessage();    //输出变量信息
	string name_tag = wordAnalysis.getToken();		//变量名
	string registerL, registerR, registerA, registerC;
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	symbolTable item = checkItem(name_tag);
	vector<int> dimenLength = item.getMatrixLength();
	string b = "%";
	if (Range == 0) {
		b = "@";	//全局变量
	}
	if (dimenLength.size() > 0) {
		registerA = "0";
	}
	int nowDimenson = 0;   //记录当前维度
	CodeItem citem1 = CodeItem(NOTE, b + name_tag, "array", "begin");
	citem1.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem1);
	while (symbol == LBRACK) {
		nowDimenson++;
		//printMessage();    //输出[信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		Exp();
		registerL = interRegister;		//该维度偏移量
		int offset = 1;
		for (int j = nowDimenson; j < dimenLength.size(); j++) {
			offset = offset * dimenLength[j];
		}
		offset = offset * 4;   //偏移量直接乘4(后面就不用乘了)
		registerR = numToString(offset);		//该维度值为1时大小(已乘4)
		if (registerL[0] != '@' && registerL[0] != '%') {   //该维度总大小，存在registerC中
			int valueL = stringToNum(registerL);
			valueL = valueL * offset;
			registerC = numToString(valueL);
		}
		else {
			registerC = "%" + numToString(Temp);
			Temp++;
			CodeItem citem = CodeItem(MUL, registerC, registerL, registerR);
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
		if (registerC[0] != '@' && registerC[0] != '%' && registerA[0] != '@' && registerA[0] != '%') {	//当前偏移量registerA + 该维度总偏移量registerC
			int valueC = stringToNum(registerC);
			int valueA = stringToNum(registerA);
			registerA = numToString(valueC + valueA);
		}
		else {
			interRegister = "%" + numToString(Temp);
			Temp++;
			CodeItem citem = CodeItem(ADD, interRegister, registerA, registerC);
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
			registerA = interRegister;
		}
		//printMessage();    //输出]信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	//printMessage();    //输出=信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	int nowSize = codetotal[Funcindex].size();
	if (codetotal[Funcindex][nowSize - 1].getOperand1() == "array" && codetotal[Funcindex][nowSize - 1].getOperand2() == "begin"
		&& codetotal[Funcindex][nowSize - 1].getResult() == b + name_tag) {	//计算偏移的注释没用
		codetotal[Funcindex].pop_back();
	}
	else {
		CodeItem citem1 = CodeItem(NOTE, b + name_tag, "array", "end");
		citem1.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem1);
	}
	Exp();
	if (dimenLength.size() > 0) {
		string tempRegister = interRegister;
		CodeItem citem = CodeItem(STOREARR, tempRegister, b + name_tag, registerA);		//数组某个变量赋值
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
	}
	else {
		CodeItem citem = CodeItem(STORE, interRegister, b + name_tag, "");		//给一维变量赋值
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
	}
	//outfile << "<赋值语句>" << endl;
}

void ifStmt()            //条件语句 + 短路逻辑
{
	//printMessage();    //输出if信息
	//判断symbol=if
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	//printMessage();   //获得(并输出
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //预读
	string if_then_label = "%if.then_" + numToString(iflabelIndex);
	string if_else_label = "%if.else_" + numToString(iflabelIndex);
	string if_end_label = "%if.end_" + numToString(iflabelIndex);
	iflabelIndex++;
	int condSize = codetotal[Funcindex].size();
	//Cond();
	Cond(if_then_label);		//短路逻辑
	//printMessage();    //输出)信息
	//判断symbol=)
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	CodeItem citem = CodeItem(BR, if_else_label, interRegister, if_then_label); //br 条件 %if.then_1 %if.else_1 
	citem.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem);
	CodeItem citem2 = CodeItem(LABEL, if_then_label, "", "");	//label if.then
	citem2.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem2);
	int nowIndex = codetotal[Funcindex].size() - 2;		//暂存 br 条件 %if.then_1 %if.else_1  下标，可能后续要把%if.else改成 %if.end
	string next_or_label = "%or.then_" + numToString(orlabelIndex - 1);
	Stmt();													//stmt1
	if (symbol == ELSETK) {
		CodeItem citem3 = CodeItem(BR, "", if_end_label, "");   //BR if.end
		citem3.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem3);
		CodeItem citem4 = CodeItem(LABEL, if_else_label, "", "");	//label if.else
		citem4.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem4);
		//printMessage();    //输出else信息
		//短路逻辑   1781~1787
		while (condSize < codetotal[Funcindex].size()) {
			if (codetotal[Funcindex][condSize].getResult() == next_or_label) {
				codetotal[Funcindex][condSize].setResult(if_else_label);
			}
			condSize++;
		}
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		Stmt();			//stmt2
	}
	else {	//只有if  没有else，只需要把br res %if.then, %if.else中的%if.else标签改成%if.end即可
		string oper1 = codetotal[Funcindex][nowIndex].getOperand1();
		codetotal[Funcindex][nowIndex].changeContent(if_end_label, oper1, if_then_label);
		//短路逻辑   1791~1803
		while (condSize < codetotal[Funcindex].size()) {
			if (codetotal[Funcindex][condSize].getResult() == next_or_label) {
				codetotal[Funcindex][condSize].setResult(if_end_label);
			}
			condSize++;
		}
	}
	CodeItem citem5 = CodeItem(BR, "", if_end_label, "");   //BR if.end
	citem5.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem5);
	CodeItem citem6 = CodeItem(LABEL, if_end_label, "", "");	//label if.else
	citem6.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem6);

	//退出前Stmt均预读
	//outfile << "<条件语句>" << endl;
}
/*
void ifStmt()            //条件语句+无短路逻辑
{
	//printMessage();    //输出if信息
	//判断symbol=if
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	//printMessage();   //获得(并输出
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //预读
	string if_then_label = "%if.then_" + numToString(iflabelIndex);
	string if_else_label = "%if.else_" + numToString(iflabelIndex);
	string if_end_label = "%if.end_" + numToString(iflabelIndex);
	iflabelIndex++;
	Cond();
	//Cond(if_then_label);		//短路逻辑
	//printMessage();    //输出)信息
	//判断symbol=)
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	CodeItem citem = CodeItem(BR, if_else_label, interRegister, if_then_label); //br 条件 %if.then_1 %if.else_1
	citem.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem);
	CodeItem citem2 = CodeItem(LABEL, if_then_label, "", "");	//label if.then
	citem2.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem2);
	int nowIndex = codetotal[Funcindex].size() - 2;		//暂存 br 条件 %if.then_1 %if.else_1  下标，可能后续要把%if.else改成 %if.end
	Stmt();													//stmt1
	if (symbol == ELSETK) {
		CodeItem citem3 = CodeItem(BR, "", if_end_label, "");   //BR if.end
		citem3.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem3);
		CodeItem citem4 = CodeItem(LABEL, if_else_label, "", "");	//label if.else
		citem4.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem4);
		//printMessage();    //输出else信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		Stmt();			//stmt2
	}
	else {	//只有if  没有else，只需要把br res %if.then, %if.else中的%if.else标签改成%if.end即可
		string oper1 = codetotal[Funcindex][nowIndex].getOperand1();
		codetotal[Funcindex][nowIndex].changeContent(if_end_label, oper1, if_then_label);
	}
	CodeItem citem5 = CodeItem(BR, "", if_end_label, "");   //BR if.end
	citem5.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem5);
	CodeItem citem6 = CodeItem(LABEL, if_end_label, "", "");	//label if.else
	citem6.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem6);
	//退出前Stmt均预读
	//outfile << "<条件语句>" << endl;
}


void Cond()              //条件表达式(逻辑或表达式)  LAndExp { '||' LAndExp}
{
	string registerL, registerR, op;
	int nowSize = codetotal[Funcindex].size();
	int flag = 0;	//flag为1说明多个||的条件中某个条件真值为1，中间代码不必要出现，可直接删除
	LAndExp();
	registerL = interRegister;
	while (symbol == OR_WORD) {
		Memory symbol_tag = symbol;
		//printMessage();    //输出逻辑运算符
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		LAndExp();
		registerR = interRegister;
		if (registerL[0] != '@' && registerL[0] != '%') { //立即数只能为0和1
			if (registerL == "0") {
				registerL = "0";
			}
			else {
				registerL = "1";
			}
		}
		if (registerR[0] != '@' && registerR[0] != '%') { //立即数只能为0和1
			if (registerR == "0") {
				registerR = "0";
			}
			else {
				registerR = "1";
			}
		}
		if (registerL[0] != '@' && registerL[0] != '%' && registerR[0] != '@' && registerR[0] != '%') {
			int value;
			int valueL = stringToNum(registerL);
			int valueR = stringToNum(registerR);
			if (valueL == 0 && valueR == 0) {
				value = 0;
			}
			else {
				value = 1;
			}
			interRegister = numToString(value);
		}
		else {
			interRegister = "%" + numToString(Temp);
			Temp++;
			CodeItem citem = CodeItem(OR, interRegister, registerL, registerR);
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
		registerL = interRegister;
	}
	//outfile << "<条件>" << endl;
}
void LAndExp()			  //逻辑与表达式   EqExp{'&&' EqExp }
{
	string registerL, registerR, op;
	int nowSize = codetotal[Funcindex].size();
	int flag = 0;	//flag为1说明多个&&的条件中某个条件真值为0，中间代码不必要出现，可直接删除
	EqExp();
	registerL = interRegister;
	while (symbol == AND_WORD) {
		Memory symbol_tag = symbol;
		//printMessage();    //输出逻辑运算符
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		EqExp();
		registerR = interRegister;
		if (registerL[0] != '@' && registerL[0] != '%') { //立即数只能为0和1
			if (registerL == "0") {
				registerL = "0";
			}
			else {
				registerL = "1";
			}
		}
		if (registerR[0] != '@' && registerR[0] != '%') { //立即数只能为0和1
			if (registerR == "0") {
				registerR = "0";
			}
			else {
				registerR = "1";
			}
		}
		if (registerL[0] != '@' && registerL[0] != '%' && registerR[0] != '@' && registerR[0] != '%') {
			int value;
			int valueL = stringToNum(registerL);
			int valueR = stringToNum(registerR);
			if (valueL == 0 || valueR == 0) {
				value = 0;
			}
			else {
				value = 1;
			}
			interRegister = numToString(value);
		}
		else {
			interRegister = "%" + numToString(Temp);
			Temp++;
			CodeItem citem = CodeItem(AND, interRegister, registerL, registerR);
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
		registerL = interRegister;
	}
}
*/
//短路逻辑

void Cond(string label)              //条件表达式(逻辑或表达式)  LAndExp { '||' LAndExp}
{
	string registerL, registerR, op;
	int nowSize = codetotal[Funcindex].size();
	int flag = 0;	//flag为1说明多个||的条件中某个条件真值为1，中间代码不必要出现，可直接删除
	string next_or_label = "%or.then_" + numToString(orlabelIndex);
	orlabelIndex++;
	LAndExp(next_or_label);
	if (symbol == OR_WORD) {
		if (interRegister[0] != '@' && interRegister[0] != '%') { //立即数只能为0和1
			if (interRegister == "0") {
				interRegister = "0";
			}
			else {
				interRegister = "1";
				CodeItem citem = CodeItem(BR, "", label, "");   //BR if.then
				citem.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem);
			}
		}
		else {
			string tempRegister = interRegister;
			interRegister = "%" + numToString(Temp);
			Temp++;
			CodeItem citem = CodeItem(NOT, interRegister, tempRegister, "");//not res ope1
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
			CodeItem citem1 = CodeItem(BR, label, interRegister, next_or_label); //br 条件 %or.next %if.then，必须保证成立的标签紧跟BR下一条 
			citem1.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem1);
			CodeItem citem2 = CodeItem(LABEL, next_or_label, "", "");	//label or.next
			citem2.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem2);
		}
	}
	while (symbol == OR_WORD) {
		Memory symbol_tag = symbol;
		//printMessage();    //输出逻辑运算符
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		next_or_label = "%or.then_" + numToString(orlabelIndex);
		orlabelIndex++;
		LAndExp(next_or_label);
		if (symbol == OR_WORD) {
			if (interRegister[0] != '@' && interRegister[0] != '%') { //立即数只能为0和1
				if (interRegister == "0") {
					interRegister = "0";
				}
				else {
					interRegister = "1";
					CodeItem citem = CodeItem(BR, "", label, "");   //BR if.then
					citem.setFatherBlock(fatherBlock);
					codetotal[Funcindex].push_back(citem);
				}
			}
			else {
				string tempRegister = interRegister;
				interRegister = "%" + numToString(Temp);
				Temp++;
				CodeItem citem = CodeItem(NOT, interRegister, tempRegister, "");//not res ope1
				citem.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem);
				CodeItem citem1 = CodeItem(BR, label, interRegister, next_or_label); //br 条件 %if.then_1 %if.else_1 
				citem1.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem1);
				CodeItem citem2 = CodeItem(LABEL, next_or_label, "", "");	//label if.then
				citem2.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem2);
			}
		}
	}
	//outfile << "<条件>" << endl;	注意最后一个或表达式不单独生成中间代码,但需生成一个标签
}
void LAndExp(string label)			  //逻辑与表达式   EqExp{'&&' EqExp }
{
	string registerL, registerR, op;
	int nowSize = codetotal[Funcindex].size();
	int flag = 0;	//flag为1说明多个&&的条件中某个条件真值为0，中间代码不必要出现，可直接删除
	string next_and_label;
	EqExp();
	if (symbol == AND_WORD) {
		if (interRegister[0] != '@' && interRegister[0] != '%') { //立即数只能为0和1
			if (interRegister == "0") {
				interRegister = "0";
				CodeItem citem = CodeItem(BR, "", label, "");   //BR or.next，出现1个0剩下的and也不用判定了
				citem.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem);
			}
			else {
				interRegister = "1";
			}
		}
		else {
			next_and_label = "%an.then_" + numToString(andlabelIndex);
			andlabelIndex++;
			CodeItem citem2 = CodeItem(BR, label, interRegister, next_and_label); //br 条件 %or.next %if.then,如果成立继续走and
			citem2.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem2);
			CodeItem citem3 = CodeItem(LABEL, next_and_label, "", "");	//label or.next
			citem3.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem3);
		}
	}
	while (symbol == AND_WORD) {
		Memory symbol_tag = symbol;
		//printMessage();    //输出逻辑运算符
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		EqExp();
		registerR = interRegister;
		if (symbol == AND_WORD) {
			if (interRegister[0] != '@' && interRegister[0] != '%') { //立即数只能为0和1
				if (interRegister == "0") {
					interRegister = "0";
					CodeItem citem = CodeItem(BR, "", label, "");   //BR or.next，出现1个0剩下的and也不用判定了
					citem.setFatherBlock(fatherBlock);
					codetotal[Funcindex].push_back(citem);
				}
				else {
					interRegister = "1";
				}
			}
			else {
				next_and_label = "%an.then_" + numToString(andlabelIndex);
				andlabelIndex++;
				CodeItem citem2 = CodeItem(BR, label, interRegister, next_and_label); //br 条件 %or.next %if.then,如果成立继续走and
				citem2.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem2);
				CodeItem citem3 = CodeItem(LABEL, next_and_label, "", "");	//label or.next
				citem3.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem3);
			}
		}
	}
	//注意最后一个与表达式不单独生成中间代码
}

void EqExp()		  //相等性表达式
{
	string registerL, registerR, op;
	RelExp();
	registerL = interRegister;
	while (symbol == EQL_WORD || symbol == NEQ_WORD) {
		Memory symbol_tag = symbol;
		//printMessage();    //输出逻辑运算符
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		RelExp();
		registerR = interRegister;
		if (registerL[0] != '@' && registerL[0] != '%' && registerR[0] != '@' && registerR[0] != '%') {
			int value;
			int valueL = stringToNum(registerL);
			int valueR = stringToNum(registerR);
			if (symbol_tag == EQL_WORD) {
				if (valueL == valueR) value = 1;
				else value = 0;
			}
			else {
				if (valueL != valueR) value = 1;
				else value = 0;
			}
			interRegister = numToString(value);
		}
		else {
			interRegister = "%" + numToString(Temp);
			Temp++;
			irCodeType codetype;
			if (symbol_tag == EQL_WORD) codetype = EQL;
			else codetype = NEQ;
			CodeItem citem = CodeItem(codetype, interRegister, registerL, registerR);
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
		registerL = interRegister;
	}
}
void RelExp()			  //关系表达式
{
	string registerL, registerR, op;
	Exp();
	registerL = interRegister;
	while (symbol == LSS || symbol == LEQ || symbol == GRE || symbol == GEQ) {
		Memory symbol_tag = symbol;
		//printMessage();    //输出逻辑运算符
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		Exp();
		registerR = interRegister;
		if (registerL[0] != '@' && registerL[0] != '%' && registerR[0] != '@' && registerR[0] != '%') {
			int value;
			int valueL = stringToNum(registerL);
			int valueR = stringToNum(registerR);
			if (symbol_tag == LSS) {
				if (valueL < valueR) value = 1;
				else value = 0;
			}
			else if (symbol_tag == LEQ) {
				if (valueL <= valueR) value = 1;
				else value = 0;
			}
			else if (symbol_tag == GRE) {
				if (valueL > valueR) value = 1;
				else value = 0;
			}
			else {
				if (valueL >= valueR) value = 1;
				else value = 0;
			}
			interRegister = numToString(value);
		}
		else {
			interRegister = "%" + numToString(Temp);
			Temp++;
			irCodeType codetype;
			if (symbol_tag == LSS) codetype = SLT;
			else if (symbol_tag == LEQ) codetype = SLE;
			else if (symbol_tag == GRE) codetype = SGT;
			else codetype = SGE;
			CodeItem citem = CodeItem(codetype, interRegister, registerL, registerR);
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
		registerL = interRegister;
	}
}
void loopStmt()          //循环语句+短路逻辑
{
	//?while '('＜条件＞')'＜语句＞
	//printMessage();    //输出while信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	//printMessage();   //获得(并输出信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	string while_cond_label = "%while.cond_" + numToString(whilelabelIndex);
	string while_body_label = "%while.body_" + numToString(whilelabelIndex);
	string while_end_label = "%while.end_" + numToString(whilelabelIndex);
	whilelabelIndex++;
	CodeItem citem1 = CodeItem(LABEL, while_cond_label, "", "");	//label %while.cond
	citem1.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem1);
	int condSize = codetotal[Funcindex].size();
	//Cond();						//cond
	Cond(while_body_label);		//短路逻辑
	//printMessage();    //输出)信息
	//短路逻辑  2179~2185
	string next_or_label = "%or.then_" + numToString(orlabelIndex - 1);
	while (condSize < codetotal[Funcindex].size()) {
		if (codetotal[Funcindex][condSize].getResult() == next_or_label) {
			codetotal[Funcindex][condSize].setResult(while_end_label);
		}
		condSize++;
	}
	CodeItem citem2 = CodeItem(BR, while_end_label, interRegister, while_body_label); //br 条件 %while.body %while.end  
	citem2.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem2);
	CodeItem citem3 = CodeItem(LABEL, while_body_label, "", "");	//label %while.body
	citem3.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem3);
	whileLabel.push_back(while_end_label);
	whileLabel.push_back(while_cond_label);
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	Stmt();
	whileLabel.pop_back();
	whileLabel.pop_back();
	CodeItem citem4 = CodeItem(BR, "", while_cond_label, "0"); //br %while.cond 
	citem4.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem4);
	CodeItem citem5 = CodeItem(LABEL, while_end_label, "", "");	//label %while.end
	citem5.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem5);
	//退出前均预读
	//outfile << "<循环语句>" << endl;
}
/*
void loopStmt()          //循环语句+无短路逻辑
{
	//?while '('＜条件＞')'＜语句＞
	//printMessage();    //输出while信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	//printMessage();   //获得(并输出信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	string while_cond_label = "%while.cond_" + numToString(whilelabelIndex);
	string while_body_label = "%while.body_" + numToString(whilelabelIndex);
	string while_end_label = "%while.end_" + numToString(whilelabelIndex);
	whilelabelIndex++;
	CodeItem citem1 = CodeItem(LABEL, while_cond_label, "", "");	//label %while.cond
	citem1.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem1);
	Cond();						//cond
	//Cond(while_body_label);		//短路逻辑
	//printMessage();    //输出)信息
	CodeItem citem2 = CodeItem(BR, while_end_label, interRegister, while_body_label); //br 条件 %while.body %while.end
	citem2.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem2);
	CodeItem citem3 = CodeItem(LABEL, while_body_label, "", "");	//label %while.body
	citem3.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem3);
	whileLabel.push_back(while_end_label);
	whileLabel.push_back(while_cond_label);
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	Stmt();
	whileLabel.pop_back();
	whileLabel.pop_back();
	CodeItem citem4 = CodeItem(BR, "", while_cond_label, ""); //br %while.cond
	citem4.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem4);
	CodeItem citem5 = CodeItem(LABEL, while_end_label, "", "");	//label %while.end
	citem5.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem5);
	//退出前均预读
	//outfile << "<循环语句>" << endl;
}
*/
void FuncRParams(string name)    //函数实参数表
{
	if (symbol == RPARENT) {
		return;
	}
	vector<vector<CodeItem>> save;  //保存当前函数每个参数所有的中间代码，一个参数占一个vector<CodeItem>
	int sizeBefore = codetotal[Funcindex].size();	//第一个参数push前的中间代码下标
	int sizeBegin = sizeBefore;		//函数实参最开始时中间代码下标
	vector<CodeItem> aa;
	if (symbol == STRING) {
		interRegister = wordAnalysis.getToken();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else {
		Exp();//退出前已经预读
	}
	int paranum = 1;
	if (paraIntNode == 0) {
		if (interRegister[0] == '\"') {
			int stringSize = interRegister.size();
			interRegister.erase(stringSize - 1, stringSize);
			interRegister = interRegister + "\\0\"";
			CodeItem citem = CodeItem(PUSH, "string", interRegister, numToString(paranum));  //传参
			citem.setFatherBlock(fatherBlock);
			citem.setFuncName(name);
			codetotal[Funcindex].push_back(citem);
		}
		else {
			CodeItem citem = CodeItem(PUSH, "int", interRegister, numToString(paranum));  //传参
			citem.setFatherBlock(fatherBlock);
			citem.setFuncName(name);
			codetotal[Funcindex].push_back(citem);
		}
	}
	else {
		CodeItem citem = CodeItem(PUSH, "int*", interRegister, numToString(paranum));  //传参
		citem.setFatherBlock(fatherBlock);
		citem.setFuncName(name);
		codetotal[Funcindex].push_back(citem);
		paraIntNode = 0;
	}
	int sizeAfter = codetotal[Funcindex].size();  //第一个参数push完后的中间代码
	for (int j = sizeBefore; j < sizeAfter; j++) {  //将第一个参数所有中间代码暂存
		aa.push_back(codetotal[Funcindex][j]);
	}
	save.push_back(aa);
	while (symbol == COMMA) {
		vector<CodeItem> aaa;
		//printMessage();    //输出,信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		sizeBefore = codetotal[Funcindex].size();	//第i个参数push前的中间代码
		Exp();
		paranum++;
		if (paraIntNode == 0) {
			CodeItem citem = CodeItem(PUSH, "int", interRegister, numToString(paranum));  //传参
			citem.setFatherBlock(fatherBlock);
			citem.setFuncName(name);
			codetotal[Funcindex].push_back(citem);
		}
		else {
			CodeItem citem = CodeItem(PUSH, "int*", interRegister, numToString(paranum));  //传参
			citem.setFatherBlock(fatherBlock);
			citem.setFuncName(name);
			codetotal[Funcindex].push_back(citem);
			paraIntNode = 0;
		}
		sizeAfter = codetotal[Funcindex].size();  //第i个参数push完后的中间代码
		for (int j = sizeBefore; j < sizeAfter; j++) {  //将第i个参数所有中间代码暂存
			aaa.push_back(codetotal[Funcindex][j]);
		}
		save.push_back(aaa);
	}
	paraNum = paranum;
	int start = codetotal[Funcindex].size();
	while (start > sizeBegin) {		//把跟参数有关所有中间代码全扔掉
		codetotal[Funcindex].pop_back();
		start--;
	}
	//开始倒叙保存中间代码，从save最后的vector倒叙复制
	int o, p, q;
	for (p = save.size() - 1; p >= 0; p--) {
		for (o = 0; o < save[p].size(); o++) {
			codetotal[Funcindex].push_back(save[p][o]);
		}
	}
	//退出循环前已经预读
	//outfile << "<函数实参数表>" << endl;
}
void returnStmt()        //返回语句
{
	//printMessage();    //输出return信息
	//判断symbol=return
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	if (symbol != SEMICN) {
		Exp();/*
		if (total[Funcindex][0].getName() == "main") {
			CodeItem citem1 = CodeItem(NOTE, "@putint", "func", "begin");          //call @foo %3 3
			citem1.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem1);//函数开始注释
			CodeItem citem2 = CodeItem(PUSH, "int", interRegister, numToString(1));  //传参
			citem2.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem2);
			CodeItem citem3 = CodeItem(CALL, "VOID", "@putint", numToString(1));          //call @foo %3 3
			citem3.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem3);//函数引用
			CodeItem citem4 = CodeItem(NOTE, "@putint", "func", "end");          //call @foo %3 3
			citem4.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem4);//函数开始注释
		}*/
		CodeItem citem = CodeItem(RET, "", interRegister, "int");  //有返回值返回
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
	}
	else {
		CodeItem citem = CodeItem(RET, "", "", "void");
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
	}
	//outfile << "<返回语句>" << endl;
}
symbolTable checkTable(string checkname, int function_number, vector<int> fatherBlock)  //function_number为函数下标,fatherBlock为代码所在作用域             
{
	int i, j, checkblock, block;
	string name;
	for (j = fatherBlock.size() - 1; j >= 0; j--) {
		checkblock = fatherBlock[j];
		for (i = 0; i < total[function_number].size(); i++) {   //先从本函数作用域找
			name = total[function_number][i].getName();
			block = total[function_number][i].getblockIndex();
			if (name == checkname && block == checkblock) {  //从最近的作用域找到了
				return total[function_number][i];
			}
		}
	}
	for (i = 0; i < total[0].size(); i++) {
		name = total[0][i].getName();
		if (name == checkname) {  //从最近的作用域找到了
			return total[0][i];
		}
	}
	exit(0);	// 新增by lzh
}
void change(int index)	//修改中间代码、符号表
{
	int i, j, k;
	//先修改中间代码
	vector<CodeItem> b = codetotal[index];
	codetotal.pop_back();
	for (i = 0; i < b.size(); i++) {
		irCodeType codetype = b[i].getCodetype();
		if (codetype == LABEL || codetype == GLOBAL || codetype == CALL || codetype == BR || codetype == DEFINE) {
			continue;
		}
		string res = b[i].getResult();
		string ope1 = b[i].getOperand1();
		string ope2 = b[i].getOperand2();
		if (res.size() > 0 && res[0] == '%' && (!isdigit(res[1]))) {  //res必须是变量
			res.erase(0, 1);  //去掉首字母
			if (names[index][res] > 1) {
				symbolTable item = checkTable(res, index, b[i].getFatherBlock());
				res = "%" + newName(res, item.getblockIndex());
			}
			else {
				res = "%" + res;
			}
		}
		if (ope1.size() > 0 && ope1[0] == '%' && (!isdigit(ope1[1]))) {  //res必须是变量
			ope1.erase(0, 1);  //去掉首字母
			if (names[index][ope1] > 1) {
				symbolTable item = checkTable(ope1, index, b[i].getFatherBlock());
				ope1 = "%" + newName(ope1, item.getblockIndex());
			}
			else {
				ope1 = "%" + ope1;
			}
		}
		if (ope2.size() > 0 && ope2[0] == '%' && (!isdigit(ope2[1]))) {  //res必须是变量
			ope2.erase(0, 1);  //去掉首字母
			if (names[index][ope2] > 1) {
				symbolTable item = checkTable(ope2, index, b[i].getFatherBlock());
				ope2 = "%" + newName(ope2, item.getblockIndex());
			}
			else {
				ope2 = "%" + ope2;
			}
		}
		b[i].changeContent(res, ope1, ope2);
	}
	codetotal.push_back(b);

	//再修改符号表
	vector<symbolTable> a = total[index];
	total.pop_back();
	for (i = 0; i < a.size(); i++) {
		if (a[i].getForm() == FUNCTION) {
			continue;
		}
		else {
			string name = a[i].getName();
			if (names[index][name] > 1) {       //某一变量在函数作用域内出现次数大于1，需要改名
				string newname = newName(name, a[i].getblockIndex());
				a[i].changeName(newname);
			}
		}
	}
	total.push_back(a);
}
void putAllocGlobalFirst()		//将中间代码中alloc类型前移，同时将CALL中调用无返回值函数对应的res转为"void"而非寄存器
{
	vector<vector<CodeItem>> temp = codetotal;
	codetotal.clear();
	int i, j, k;
	codetotal.push_back(temp[0]);
	for (i = 1; i < temp.size(); i++) {
		vector<CodeItem> a;
		for (j = 0; j < temp[i].size(); j++) {
			if (temp[i][j].getCodetype() == DEFINE || temp[i][j].getCodetype() == PARA || temp[i][j].getCodetype() == ALLOC || temp[i][j].getCodetype() == GLOBAL) {
				a.push_back(temp[i][j]);
			}
		}
		for (j = 0; j < temp[i].size(); j++) {
			if (temp[i][j].getCodetype() != DEFINE && temp[i][j].getCodetype() != PARA && temp[i][j].getCodetype() != ALLOC && temp[i][j].getCodetype() != GLOBAL) {
				if (temp[i][j].getCodetype() == CALL) {
					string funName = temp[i][j].getOperand1();
					if (funName == "@printf" || funName == "@_sysy_starttime" || funName == "@_sysy_stoptime" || funName == "@putarray"
						|| funName == "@putch" || funName == "@putint") {
						string paraNum = temp[i][j].getOperand2();
						temp[i][j].changeContent("void", funName, paraNum);
						a.push_back(temp[i][j]);
						//j++;	//CALL后面的MOV也没用了
					}
					else if (funName == "@getint" || funName == "@getarray" || funName == "@getch") {
						a.push_back(temp[i][j]);
					}
					else {		//前面都是特殊函数，这里是非特殊函数
						valueType type = VOID;
						for (k = 1; k < total.size(); k++) {		//查表
							if (funName == total[k][0].getName()) {
								type = total[k][0].getValuetype();
								break;
							}
						}
						if (type == INT) {
							a.push_back(temp[i][j]);
						}
						else {
							string paraNum = temp[i][j].getOperand2();
							temp[i][j].changeContent("void", funName, paraNum);
							a.push_back(temp[i][j]);
							//j++;	//CALL后面的MOV也没用了
						}
					}
				}
				else {
					a.push_back(temp[i][j]);
				}
			}
		}
		codetotal.push_back(a);
	}
}
string getValue(string var, string offset)	//var是变量，offset是偏移量
{
	int i = 0;
	string name;
	for (i = 0; i < total[0].size(); i++) {
		name = total[0][i].getName();
		if (name == var) {  //从最近的作用域找到了
			break;
		}
	}
	int offset_int = stringToNum(offset);
	int value = total[0][i].getIntValue()[offset_int];
	return numToString(value);
}
void changeForInline(int index)
{
	int i;
	string Funcname = total[index][0].getName(); //函数名
	//先修改中间代码中局部变量、参数名
	vector<CodeItem> b = codetotal[index];
	codetotal.pop_back();
	for (i = 0, noWhileLabel = 0, haveCall = 0; i < b.size(); i++) {
		irCodeType codetype = b[i].getCodetype();
		string res = b[i].getResult();
		string ope1 = b[i].getOperand1();
		string ope2 = b[i].getOperand2();
		if (codetype == CALL && isNoChangeFunc(ope1) == false) {
			haveCall = 1;		//出现调用，直接不做将全局变量变成局部变量的操作
		}
		if (res.size() > 0 && res[0] == '%' && (!isdigit(res[1]))) {  //res必须是变量或参数
			string aa = res;
			string bb = res;
			if (aa.size() > 7) {
				aa = aa.substr(0, 7);
			}
			if (bb.size() > 4) {
				bb = bb.substr(0, 4);
			}
			if (aa != "%while." && bb != "%if." && bb != "%an." && bb != "%or.") {
				res = newinlineName(res, Funcname);
			}
			if (aa == "%while.") {
				noWhileLabel = i;
			}
		}
		if (ope1.size() > 0 && ope1[0] == '%' && (!isdigit(ope1[1]))) {  //res必须是变量
			string aa = ope1;
			string bb = ope1;
			if (aa.size() > 7) {
				aa = aa.substr(0, 7);
			}
			if (bb.size() > 4) {
				bb = bb.substr(0, 4);
			}
			if (aa != "%while." && bb != "%if." && bb != "%an." && bb != "%or.") {
				ope1 = newinlineName(ope1, Funcname);
			}
			if (aa == "%while.") {
				noWhileLabel = i;
			}
		}
		if (ope2.size() > 0 && ope2[0] == '%' && (!isdigit(ope2[1]))) {  //res必须是变量
			string aa = ope2;
			string bb = ope2;
			if (aa.size() > 7) {
				aa = aa.substr(0, 7);
			}
			if (bb.size() > 4) {
				bb = bb.substr(0, 4);
			}
			if (aa != "%while." && bb != "%if." && bb != "%an." && bb != "%or.") {
				ope2 = newinlineName(ope2, Funcname);
			}
			if (aa == "%while.") {
				noWhileLabel = i;
			}
		}
		b[i].changeContent(res, ope1, ope2);
	}
	codetotal.push_back(b);
	//再修改符号表
	vector<symbolTable> a = total[index];
	total.pop_back();
	for (i = 0; i < a.size(); i++) {
		if (a[i].getForm() == FUNCTION) {
			continue;
		}
		else {		//常量、变量、参数名字都改
			string name = a[i].getName();
			string newname = newinlineName(name, Funcname);
			a[i].changeName(newname);
		}
	}
	total.push_back(a);
}
void youhuaDivCompare()
{
	int i, j, k;
	for (i = 1; i < codetotal.size(); i++) {
		for (j = 0; j < codetotal[i].size(); j++) {		//优化条件1：当前代码为除数而且下一条代码为比较类型代码
			if (codetotal[i][j].getCodetype() == DIV && isCompare(codetotal[i][j + 1].getCodetype())) {
				CodeItem c1 = codetotal[i][j];
				CodeItem c2 = codetotal[i][j + 1];
				string res = c1.getResult();
				string ope1 = c2.getOperand1();
				string ope2 = c2.getOperand2();			//比较的右操作数
				if (res == ope1 && ope2[0] != '%') {		//比较左操作数为临时变量，右操作数为整数
					int value = stringToNum(ope2);
					if (value >= 0) {
						string num1 = c1.getOperand1();		//DIV中的被除数
						string num2 = c1.getOperand2();		//DIV中的除数
						if (num2[0] == '%') {			//div中除数是临时变量
							codetotal[i].erase(codetotal[i].begin() + j);
							codetotal[i].erase(codetotal[i].begin() + j);
							if (c2.getCodetype() == SLT) {		//div %2  %1  %0；slt %3 %2 10000    小于                
								CodeItem citem1 = CodeItem(MUL, c1.getResult(), num2, ope2);
								codetotal[i].insert(codetotal[i].begin() + j, citem1);
								CodeItem citem2 = CodeItem(SLT, c2.getResult(), num1, c1.getResult());
								codetotal[i].insert(codetotal[i].begin() + j + 1, citem2);
							}
							else if (c2.getCodetype() == SLE) {  //div %2  %1  %0；sle %3 %2 10000    小于等于    
								CodeItem citem1 = CodeItem(MUL, c1.getResult(), num2, numToString(value + 1));
								codetotal[i].insert(codetotal[i].begin() + j, citem1);
								CodeItem citem2 = CodeItem(SLT, c2.getResult(), num1, c1.getResult());
								codetotal[i].insert(codetotal[i].begin() + j + 1, citem2);
							}
							else if (c2.getCodetype() == SGE) {  //div %2  %1  %0；sge %3 %2 10000    大于等于    
								CodeItem citem1 = CodeItem(MUL, c1.getResult(), num2, ope2);
								codetotal[i].insert(codetotal[i].begin() + j, citem1);
								CodeItem citem2 = CodeItem(SGE, c2.getResult(), num1, c1.getResult());
								codetotal[i].insert(codetotal[i].begin() + j + 1, citem2);
							}
							else if (c2.getCodetype() == SGT) {  //div %2  %1  %0；sgt %3 %2 10000    大于    
								CodeItem citem1 = CodeItem(MUL, c1.getResult(), num2, numToString(value + 1));
								codetotal[i].insert(codetotal[i].begin() + j, citem1);
								CodeItem citem2 = CodeItem(SGE, c2.getResult(), num1, c1.getResult());
								codetotal[i].insert(codetotal[i].begin() + j + 1, citem2);
							}
						}
						else {
							int value2 = stringToNum(num2);
							if (value2 > 0) {
								codetotal[i].erase(codetotal[i].begin() + j);
								codetotal[i].erase(codetotal[i].begin() + j);
								if (c2.getCodetype() == SLT) {		//div %2  %1  30；slt %3 %2 10000    小于                
									CodeItem citem = CodeItem(SLT, c2.getResult(), num1, numToString(value2 * value));
									codetotal[i].insert(codetotal[i].begin() + j, citem);
								}
								else if (c2.getCodetype() == SLE) {  //div %2  %1  30；sle %3 %2 10000    小于等于    
									CodeItem citem = CodeItem(SLT, c2.getResult(), num1, numToString(value2 * (value + 1)));
									codetotal[i].insert(codetotal[i].begin() + j, citem);
								}
								else if (c2.getCodetype() == SGE) {  //div %2  %1  30；sge %3 %2 10000    大于等于    
									CodeItem citem = CodeItem(SGE, c2.getResult(), num1, numToString(value2 * value));
									codetotal[i].insert(codetotal[i].begin() + j, citem);
								}
								else if (c2.getCodetype() == SGT) {  //div %2  %1  30；sle %3 %2 10000    大于    
									CodeItem citem = CodeItem(SGE, c2.getResult(), num1, numToString(value2 * (value + 1)));
									codetotal[i].insert(codetotal[i].begin() + j, citem);
								}
							}
						}
					}
				}
				else if (res == ope2 && ope1[0] != '%') {		//比较右操作数为临时变量，左操作数为整数
					int value = stringToNum(ope1);
					if (value >= 0) {
						string num1 = c1.getOperand1();		//DIV中的被除数
						string num2 = c1.getOperand2();		//DIV中的除数
						if (num2[0] == '%') {			//div中除数是临时变量
							codetotal[i].erase(codetotal[i].begin() + j);
							codetotal[i].erase(codetotal[i].begin() + j);
							if (c2.getCodetype() == SLT) {		//div %2  %1  %0；slt %3 10000 %2   小于                
								CodeItem citem1 = CodeItem(MUL, c1.getResult(), num2, numToString(value + 1));
								codetotal[i].insert(codetotal[i].begin() + j, citem1);
								CodeItem citem2 = CodeItem(SGE, c2.getResult(), num1, c1.getResult());
								codetotal[i].insert(codetotal[i].begin() + j + 1, citem2);
							}
							else if (c2.getCodetype() == SLE) {  //div %2  %1  %0；sle %3 %2 10000    小于等于    
								CodeItem citem1 = CodeItem(MUL, c1.getResult(), num2, ope1);
								codetotal[i].insert(codetotal[i].begin() + j, citem1);
								CodeItem citem2 = CodeItem(SGE, c2.getResult(), num1, c1.getResult());
								codetotal[i].insert(codetotal[i].begin() + j + 1, citem2);
							}
							else if (c2.getCodetype() == SGE) {  //div %2  %1  %0；sge %3 10000 %2    大于等于    
								CodeItem citem1 = CodeItem(MUL, c1.getResult(), num2, numToString(value + 1));
								codetotal[i].insert(codetotal[i].begin() + j, citem1);
								CodeItem citem2 = CodeItem(SLT, c2.getResult(), num1, c1.getResult());
								codetotal[i].insert(codetotal[i].begin() + j + 1, citem2);
							}
							else if (c2.getCodetype() == SGT) {  //div %2  %1  %0；sgt %3 10000 %2    大于    
								CodeItem citem1 = CodeItem(MUL, c1.getResult(), num2, ope1);
								codetotal[i].insert(codetotal[i].begin() + j, citem1);
								CodeItem citem2 = CodeItem(SLT, c2.getResult(), num1, c1.getResult());
								codetotal[i].insert(codetotal[i].begin() + j + 1, citem2);
							}
						}
						else {
							int value2 = stringToNum(num2);
							if (value2 > 0) {
								codetotal[i].erase(codetotal[i].begin() + j);
								codetotal[i].erase(codetotal[i].begin() + j);
								if (c2.getCodetype() == SLT) {		//div %2  %1  30；slt %3 10000  %2   小于                
									CodeItem citem = CodeItem(SGE, c2.getResult(), num1, numToString(value2 * (value + 1)));
									codetotal[i].insert(codetotal[i].begin() + j, citem);
								}
								else if (c2.getCodetype() == SLE) {  //div %2  %1  30；sle %3 10000 %2    小于等于    
									CodeItem citem = CodeItem(SGE, c2.getResult(), num1, numToString(value2 * value));
									codetotal[i].insert(codetotal[i].begin() + j, citem);
								}
								else if (c2.getCodetype() == SGE) {  //div %2  %1  30；sge %3 10000 %2    大于等于    
									CodeItem citem = CodeItem(SLT, c2.getResult(), num1, numToString(value2 * (value + 1)));
									codetotal[i].insert(codetotal[i].begin() + j, citem);
								}
								else if (c2.getCodetype() == SGT) {  //div %2  %1  30；sle %3 10000 %2    大于    
									CodeItem citem = CodeItem(SLT, c2.getResult(), num1, numToString(value2 * value));
									codetotal[i].insert(codetotal[i].begin() + j, citem);
								}
							}
						}
					}
				}
			}
		}
	}
}
bool isCompare(irCodeType type)
{
	if (type == SLT || type == SGT || type == SGE || type == SLE) {
		return true;
	}
	return false;
}
void deleteSameExp(int index)
{
	int i;
	for (i = noWhileLabel + 1; i < codetotal[index].size(); i++) {		//保证从noWhileLabel+1 到最后一定是顺序执行的
		if (codetotal[index][i].getCodetype() == CALL && isNoChangeFunc(codetotal[index][i].getFuncName()) == false) {
			return;		//如果还会跳到别的函数，直接不做了
		}
	}
	//从此开始找相同表达式
	set<string> varName;
	record.clear();
	maxLine.clear();
	newVarName.clear();
	for (i = noWhileLabel + 1; i < codetotal[index].size(); i++) {		//保证从noWhileLabel+1 到最后一定是顺序执行的
		if (codetotal[index][i].getCodetype() == STORE || codetotal[index][i].getCodetype() == STOREARR) {
			if (!maxLine.count(codetotal[index][i].getOperand1()) > 0) {
				maxLine[codetotal[index][i].getOperand1()] = i; //涉及此类变量求出下限
			}
		}
		if (codetotal[index][i].getCodetype() == LOAD || codetotal[index][i].getCodetype() == LOADARR) {
			varName.insert(codetotal[index][i].getOperand1());				//涉及此类变量先记录下来，存在表达式相同的可能
			record[codetotal[index][i].getOperand1()] = record[codetotal[index][i].getOperand1()] + 1;
		}
	}
	while (true) {
		int flag = 0;
		for (string str : varName) {		//该变量至少load3次
			if (record[str] <= 2) {
				varName.erase(str);
				flag = 1;
				break;
			}
		}
		if (flag == 0) {
			break;
		}
	}
	if (varName.size() == 0) {
		return;			//没有变量符合公共表达式删除，直接退出
	}
	int j, i1, j1, i2, j2;
	newVarName.clear();
	for (string str : varName) {
		if (!maxLine.count(str) > 0) {
			maxLine[str] = codetotal[index].size();
		}
		for (i = noWhileLabel + 1; i < maxLine[str]; i++) {		//保证从noWhileLabel+1 到最后一定是顺序执行的
			if (codetotal[index][i].getCodetype() == LOAD || codetotal[index][i].getCodetype() == LOADARR) {
				if (str == codetotal[index][i].getOperand1()) {		//找到该变量第一次出现的位置
					j = i;
					break;
				}
			}
		}
		for (i = i + 1; i < maxLine[str]; i++) {
			if (codetotal[index][i].getCodetype() == LOAD || codetotal[index][i].getCodetype() == LOADARR) {
				if (str == codetotal[index][i].getOperand1()) {		//找到该变量第二次出现的位置
					j1 = j; i1 = i; j2 = j; i2 = i;
					while (j1 > noWhileLabel && codetotal[index][j1].isequal(codetotal[index][i1]) && j < i1) {
						j1--;		i1--;
					}
					while (j2 < i && codetotal[index][j2].isequal(codetotal[index][i2]) && j2 < i1) {
						j2++;		i2++;
					}		//上下搜索公共子表达式
					j2 = j2 - 1; i2 = i2 - 1; j1 = j1 + 1; i1 = i1 + 1;
					if (j2 - j1 < 6 || isTemp(codetotal[index][i2].getResult()) == false || isTemp(codetotal[index][j2].getResult()) == false
						|| codetotal[index][j1].getCodetype() != NOTE) {
						continue;	//公共子表达式要大于6条而且最后一条的res字段应该是临时变量
					}
					else {		//可以删除了
						/*
						cout << codetotal[index][j1].getContent() << endl;
						cout << codetotal[index][j2].getContent() << endl;
						*/
						string xiabiao = numToString(j1) + "-" + numToString(j2);
						if (!(newVarName.count(xiabiao) > 0)) {		//第一次找到公共子表达式
							string replace = "%Rep" + numToString(j1) + "-" + numToString(j2) + "+" + total[index][0].getName();
							newVarName[xiabiao] = replace;
							string newTemp;
							int i4 = i1;
							while (i1 < i2) {		//找一个可用的临时变量
								if (isTemp(codetotal[index][i1].getResult())) {
									newTemp = codetotal[index][i1].getResult(); break;
								}
								if (isTemp(codetotal[index][i1].getOperand1())) {
									newTemp = codetotal[index][i1].getOperand1(); break;
								}
								if (isTemp(codetotal[index][i1].getOperand2())) {
									newTemp = codetotal[index][i1].getOperand2(); break;
								}
								i1++;
							}
							i1 = i4;
							//同时往j2处插入新的中间代码
							string oldTemp = codetotal[index][j2].getResult();
							codetotal[index][j2].setResult(newTemp);	//改成新的
							CodeItem citem1 = CodeItem(STORE, newTemp, newVarName[xiabiao], "");	//赋值单值
							CodeItem citem2 = CodeItem(LOAD, oldTemp, newVarName[xiabiao], "");
							codetotal[index].insert(codetotal[index].begin() + j2 + 1, citem1);
							codetotal[index].insert(codetotal[index].begin() + j2 + 2, citem2);
							i2 = i2 + 2; i1 = i1 + 2; maxLine[str] = maxLine[str] + 2;
						}
						CodeItem citem = CodeItem(LOAD, codetotal[index][i2].getResult(), newVarName[xiabiao], "");
						int i3 = i1;
						while (i1 <= i2) {		//删除公共子表达式
							codetotal[index].erase(codetotal[index].begin() + i3);
							i1++;
						}
						codetotal[index].insert(codetotal[index].begin() + i3, citem);	//生成新的子表达式
						maxLine[str] = maxLine[str] - (i2 - i3);
						for (; i < maxLine[str]; i++) {	//找到一个基础上继续找相同的
							if (codetotal[index][i].isequal(codetotal[index][j1])) {
								int i5 = i; int j5 = j1;
								while (j5 <= j2) {
									if (!codetotal[index][i5].isequal(codetotal[index][j5])) {
										break;
									}
									j5++;  i5++;
								}
								j5 = j5 - 1; i5 = i5 - 1;
								if (j5 == j2) {	//找到相同的
									CodeItem citem = CodeItem(LOAD, codetotal[index][i5].getResult(), newVarName[xiabiao], "");
									int i6 = i5 - (j2 - j1);
									int i7 = i6;
									while (i6 <= i5) {		//删除公共子表达式
										codetotal[index].erase(codetotal[index].begin() + i7);
										i6++;
									}
									codetotal[index].insert(codetotal[index].begin() + i7, citem);	//生成新的子表达式
									maxLine[str] = maxLine[str] - (j2 - j1);
									i = i6;
								}
								else {
									continue;
								}
							}
						}
						i = i3;		//恢复循环变量值
					}
				}
			}
		}
	}
	//最后插入符号表同时生成ALLOC代码
	map <  string, string>::iterator iter;
	for (iter = newVarName.begin(); iter != newVarName.end(); iter++)
	{
		string ppp = iter->second.substr(1, iter->second.size());
		symbolTable item = symbolTable(VARIABLE, INT, ppp, 0, 0);
		total[index].push_back(item);
		CodeItem citem = CodeItem(ALLOC, iter->second, "0", "1");
		codetotal[index].insert(codetotal[index].begin() + codetotal[index].size() - 2, citem);
	}
}
bool isNoChangeFunc(string a)
{
	if (a == "@printf" || a == "@_sysy_starttime" || a == "@_sysy_stoptime" || a == "@putarray"
		|| a == "@putch" || a == "@putint") {
		return true;
	}
	return false;
}
bool isTemp(string a)
{
	if (a[0] == '%' && isdigit(a[1])) {
		return true;
	}
	return false;
}
void changeGlobalToAlloc(int index)
{
	if (haveCall == 1) {	//出现调用，直接不做将全局变量变成局部变量的操作  或者  全局变量只在该函数内部出现过，而且函数不会调用自身
		return;
	}
	int i, j, k;
	map<string, int> globalTimes;	//统计全局变量出现次数
	set<string> globalNames;		//统计全局变量名
	for (i = 1; i < codetotal[index].size(); i++) {		//先遍历中间代码统计全局变量出现次数
		string res = codetotal[index][i].getResult();
		string ope1 = codetotal[index][i].getOperand1();
		string ope2 = codetotal[index][i].getOperand2();
		if (codetotal[index][i].getCodetype() == NOTE) {			//注释类的中间代码不管
			continue;
		}
		if (res[0] == '@') {
			globalTimes[res] = globalTimes[res] + 1;
			globalNames.insert(res);
		}
		if (ope1[0] == '@') {
			globalTimes[ope1] = globalTimes[ope1] + 1;
			globalNames.insert(ope1);
		}
		if (ope2[0] == '@') {
			globalTimes[ope2] = globalTimes[ope2] + 1;
			globalNames.insert(ope2);
		}
	}
	while (true) {
		int flag = 0;
		for (string str : globalNames) {
			if (globalTimes[str] <= 4) {		//该变量至少出现5次
				globalNames.erase(str);
				flag = 1;
				break;
			}
			for (j = 0; j < total[0].size(); j++) {
				string gName = str.substr(1, str.size());
				if (total[0][j].getName() == gName) {		//此时符号表中变量还没有@，所以要去掉首字符
					break;
				}
			}
			if ( j==total[0].size() || total[0][j].getDimension() > 0) {  //没查到或者查到了该变量不能是全局数组
				globalNames.erase(str);
				flag = 1;
				break;
			}
		}
		if (flag == 0) {
			break;
		}
	}
	//此时globalNames中剩下符合要求的全局变量
	if (globalNames.size() == 0) {
		return;
	}
	if (total[index][0].getValuetype() == VOID) {		//VOID函数没有ret自动补齐
		int size = codetotal[index].size();
		if (codetotal[index][size - 1].getCodetype() != RET) {
			CodeItem citem = CodeItem(RET, "", "", "void");
			codetotal[index].push_back(citem);
		}
	}
	for (string str : globalNames) {
		string newName = str.substr(1, str.size());
		newName = "%Glo-All-" + newName + "+" + total[index][0].getName();	//该全局变量对应局部变量新名字
		for (i = 1; i < codetotal[index].size(); i++) {		//遍历中间代码，更改成分
			if (codetotal[index][i].getResult() == str) {
				codetotal[index][i].setResult(newName);
			}
			if (codetotal[index][i].getOperand1() == str) {
				codetotal[index][i].setOperand1(newName);
			}
			if (codetotal[index][i].getOperand2() == str) {
				codetotal[index][i].setOperand2(newName);
			}
		}
		//在符号表中插入新变量
		symbolTable item = symbolTable(VARIABLE, INT, newName.substr(1, newName.size()), 0, 0);
		total[index].push_back(item);
		//在中间代码加入新内容
		//先获取临时变量序号,共需要retNum+1个
		for (j = 1; j < codetotal[index].size(); j++) {
			if (codetotal[index][j].getCodetype() == PARA || codetotal[index][j].getCodetype() == ALLOC) {
				continue;
			}
			else {
				CodeItem citem = CodeItem(ALLOC, newName, "0", "1");
				codetotal[index].insert(codetotal[index].begin() + j, citem);
				CodeItem citem1 = CodeItem(LOAD, '%' + numToString(Temp), str, "");
				codetotal[index].insert(codetotal[index].begin() + j + 1, citem1);
				CodeItem citem2 = CodeItem(STORE, '%' + numToString(Temp), newName, "");	//赋值单值
				codetotal[index].insert(codetotal[index].begin() + j + 2, citem2);
				j = j + 3;
				Temp++;
				break;
			}
		}
		for (; j < codetotal[index].size(); j++) {
			if (codetotal[index][j].getCodetype() == RET) {
				CodeItem citem1 = CodeItem(LOAD, '%' + numToString(Temp), newName, "");
				codetotal[index].insert(codetotal[index].begin() + j, citem1);
				CodeItem citem2 = CodeItem(STORE, '%' + numToString(Temp), str, "");	//赋值单值
				codetotal[index].insert(codetotal[index].begin() + j + 1, citem2);
				j = j + 2;
				Temp++;
			}
		}
	}
}
void addUseCount(int index)
{
	matrixUseCount.clear();
	int i, j, k;
	for (i = 1; i < total[index].size(); i++) {
		if (total[index][i].getForm()==VARIABLE && total[index][i].getDimension() > 0 ) {
			matrixUseCount[total[index][i].getName()] = 0;
		}
	}
	for (i = 0; i < codetotal[index].size(); i++) {
		if (codetotal[index][i].getCodetype() == LOADARR) {
			//CodeItem citem = CodeItem(LOADARR, interRegister, b + name_tag, registerA); //数组取值
			string matrixname = codetotal[index][i].getOperand1();
			if (matrixname[0] == '%') {
				string name2 = matrixname.substr(1, matrixname.size());
				if (matrixUseCount.count(name2) > 0) {
					matrixUseCount[name2]++;
				}
			}
		}
		if (codetotal[index][i].getCodetype() == STOREARR) {
			//CodeItem citem = CodeItem(STOREARR, "0", b + nodeName, offset_string);
			string matrixname = codetotal[index][i].getOperand1();
			if (matrixname[0] == '%') {
				string name2 = matrixname.substr(1, matrixname.size());
				if (matrixUseCount.count(name2) > 0) {
					matrixUseCount[name2]++;
				}
			}
		}
	}
	for (i = 1; i < total[index].size(); i++) {
		if (total[index][i].getForm() == VARIABLE && total[index][i].getDimension() > 0) {
			total[index][i].setUseCount(matrixUseCount[total[index][i].getName()]);
			//cout << matrixUseCount[total[index][i].getName()] << " " << total[index][i].getName() << endl;  检测统计正确性输出
		}
	}
}
void changeGlobalToAlloc2(int index)
{
	int i, j, k;
	int jilu = 0;
	map<string, int> globalTimes;	//统计全局变量出现次数
	set<string> globalNames;		//统计全局变量名
	for (i = 1; i < codetotal[index].size(); i++) {		//先遍历中间代码统计全局变量出现次数
		string res = codetotal[index][i].getResult();
		string ope1 = codetotal[index][i].getOperand1();
		string ope2 = codetotal[index][i].getOperand2();
		if (codetotal[index][i].getCodetype() == NOTE) {			//注释类的中间代码不管
			continue;
		}
		if (res[0] == '@') {
			globalTimes[res] = globalTimes[res] + 1;
			globalNames.insert(res);
		}
		if (ope1[0] == '@') {
			globalTimes[ope1] = globalTimes[ope1] + 1;
			globalNames.insert(ope1);
		}
		if (ope2[0] == '@') {
			globalTimes[ope2] = globalTimes[ope2] + 1;
			globalNames.insert(ope2);
		}
		if (codetotal[index][i].getCodetype() == CALL && ope1 == total[index][0].getName()) {
			jilu = 1;		//不能调用函数本身
		}
	}
	if (jilu == 1 || globalNames.size()==0 ) {
		return;
	}
	while (true) {
		int flag = 0;
		for (string str : globalNames) {
			if (globalTimes[str] <= 4) {		//该变量至少出现5次
				globalNames.erase(str);
				flag = 1;
				break;
			}
			for (j = 0; j < total[0].size(); j++) {
				if (total[0][j].getName() == str) {		//此时符号表中变量还没有@，所以要去掉首字符
					break;
				}
			}
			if (j == total[0].size() || total[0][j].getDimension() > 0 || total[0][j].getFuncindexSize() > 1) {  //没查到或者查到了该变量不能是全局数组
				globalNames.erase(str);
				flag = 1;
				break;
			}
		}
		if (flag == 0) {
			break;
		}
	}
	//此时globalNames中剩下符合要求的全局变量
	if (globalNames.size() == 0) {
		return;
	}
	if (total[index][0].getValuetype() == VOID) {		//VOID函数没有ret自动补齐
		int size = codetotal[index].size();
		if (codetotal[index][size - 1].getCodetype() != RET) {
			CodeItem citem = CodeItem(RET, "", "", "void");
			codetotal[index].push_back(citem);
		}
	}
	for (string str : globalNames) {
		string newName = str.substr(1, str.size());
		newName = "%Glo-All-" + newName + "+" + total[index][0].getName().substr(1,total[index][0].getName().size());	//该全局变量对应局部变量新名字
		for (i = 1; i < codetotal[index].size(); i++) {		//遍历中间代码，更改成分
			if (codetotal[index][i].getResult() == str) {
				codetotal[index][i].setResult(newName);
			}
			if (codetotal[index][i].getOperand1() == str) {
				codetotal[index][i].setOperand1(newName);
			}
			if (codetotal[index][i].getOperand2() == str) {
				codetotal[index][i].setOperand2(newName);
			}
		}
		//在符号表中插入新变量
		symbolTable item = symbolTable(VARIABLE, INT, newName, 0, 0);
		total[index].push_back(item);
		//在中间代码加入新内容
		//先获取临时变量序号,共需要retNum+1个
		for (j = 1; j < codetotal[index].size(); j++) {
			if (codetotal[index][j].getCodetype() == PARA || codetotal[index][j].getCodetype() == ALLOC) {
				continue;
			}
			else {
				CodeItem citem = CodeItem(ALLOC, newName, "0", "1");
				codetotal[index].insert(codetotal[index].begin() + j, citem);
				CodeItem citem1 = CodeItem(LOAD, '%' + numToString(Temp), str, "");
				codetotal[index].insert(codetotal[index].begin() + j + 1, citem1);
				CodeItem citem2 = CodeItem(STORE, '%' + numToString(Temp), newName, "");	//赋值单值
				codetotal[index].insert(codetotal[index].begin() + j + 2, citem2);
				j = j + 3;
				Temp++;
				break;
			}
		}
		for (; j < codetotal[index].size(); j++) {
			if (codetotal[index][j].getCodetype() == RET) {
				CodeItem citem1 = CodeItem(LOAD, '%' + numToString(Temp), newName, "");
				codetotal[index].insert(codetotal[index].begin() + j, citem1);
				CodeItem citem2 = CodeItem(STORE, '%' + numToString(Temp), str, "");	//赋值单值
				codetotal[index].insert(codetotal[index].begin() + j + 1, citem2);
				j = j + 2;
				Temp++;
			}
		}
	}
}