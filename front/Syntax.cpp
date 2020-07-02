#include<string>
#include<fstream>
#include<iostream>
#include "word.h"
#include "symboltable.h"
#include "intermediatecode.h"
#include<vector>
#include<sstream>
#include<map>
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
//加入自定义函数
string Functionname;	//记录当前函数名

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
			Range = 0;
			return total[0][i];
		}
	}
}
//生成目标语言引入变量
int paraIntNode = 0;		  //记录数组是否作为参数
string matrixName;			  //记录数组名

//词法、语法分析引入变量
string token;   //分析出来的单词
enum Memory symbol;  //分析出来的单词类别
ofstream outfile;
Word wordAnalysis("testexample.txt");
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
	outfile << message << endl;
}
void CompUnit();			   //编译单元
void ConstDecl(int index,int block);		       //常量说明
void ConstDef(int index,int block);			   //常量定义
void VarDecl(int index,int block);			   //变量说明
void VarDef(int index,int block);             //变量定义
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
void Cond();              //条件表达式(逻辑或表达式)
void loopStmt();          //循环语句
void FuncRParams();		  //值参数表
void returnStmt();        //返回语句
void LAndExp();			  //逻辑与表达式
void EqExp();			  //相等性表达式
void RelExp();			  //关系表达式

string newName(string name, int blockindex)
{
	stringstream trans;          //数字和字符串相互转化渠道
	trans << blockindex;
	return name + "-" + trans.str();
}
symbolTable checkTable(string checkname, int function_number, vector<int> fatherBlock);					//查表：改进中间代码和符号表时使用
void change(int index);				//修改中间代码、符号表


int main()
{
	outfile.open("output.txt");
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
					CodeItem citem = CodeItem(RET,"","void","");
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
		string b = "%";
		if (i == 0) b = "@";
		for (int j = 0; j < total[i].size(); j++) {
			total[i][j].changeName(b + total[i][j].getName());
		}
	}
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
	//检测中间代码正确性
	for (int i = 0; i < codetotal.size(); i++) {
		vector<CodeItem> item = codetotal[i];
		for (int j = 0; j < item.size(); j++) {
			cout << item[j].getContent() << endl;
		}
		cout << "\n";
	}
	//优化中间代码

	//后端运行


	outfile.close();
	return 0;
}
void CompUnit()
{
	vector<symbolTable> global;
	total.push_back(global);
	vector<CodeItem> global1;
	codetotal.push_back(global1);
	map<string, int> globalll;
	names.push_back(globalll);
	while (symbol == INTTK or symbol == CONSTTK or symbol == VOIDTK) {
		if (symbol == CONSTTK) {    //常量声明
			ConstDecl(0,0);
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
					Temp = 0;
					vector<symbolTable> item;
					total.push_back(item);
					vector<CodeItem> item1;
					codetotal.push_back(item1);
					map<string, int> item2;
					names.push_back(item2);
					valueFuncDef();
					change(Funcindex);		//修改中间代码、符号表
				}
				else {
					symbol = sym_tag;
					VarDecl(0,0);
				}
			}
			else {
				Funcindex++;
				Temp = 0;
				vector<symbolTable> item;
				total.push_back(item);
				vector<CodeItem> item1;
				codetotal.push_back(item1);
				map<string, int> item2;
				names.push_back(item2);
				novalueFuncDef();
				change(Funcindex);			//修改中间代码、符号表
			}
		}
	}
	outfile<<"<编译单元>"<<endl;
}
void ConstDecl(int index,int block) 		   //常量说明
{
	printMessage();    //输出const信息
	//判断symbol==CONSTTK
	wordAnalysis.getsym();  
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	printMessage();    //输出int信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //预读
	ConstDef(index,block);  //常量定义
	//判断symbol=;
	while (symbol == COMMA) {
		printMessage();   //输出,信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();
		ConstDef(index,block);  //常量定义
	}
	printMessage();    //输出；信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //预读
	//退出循环前已经预读单词
	outfile << "<常量说明>" << endl;
}
void ConstDef(int index,int block)			   //常量定义
{
	int value;
	vector<int> length; //记录每个维度大小
	printMessage();   //输出标识符信息
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
		printMessage();   //输出]信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //预读
	}
	symbolTable item = symbolTable(CONSTANT, INT, name,dimenson,block);
	item.setMatrixLength(length);
	total[index].push_back(item);
	names[index][name]++;
	printMessage();   //输出=信息
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
	outfile << "<常量定义>" << endl;
}
int ConstExp()
{
	int valueL, valueR;
	valueL= ConstMulExp();  //退出前读了一个单词
	while (symbol == PLUS || symbol == MINU) {
		Memory tag = symbol;
		printMessage();    //输出+或-信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		valueR = ConstMulExp();
		if (tag == PLUS) valueL = valueL + valueR;
		else valueL = valueL - valueR;
	}
	//退出前已经预读
	outfile << "<常量表达式>" << endl;
	return valueL;
}
int ConstMulExp()
{
	int valueL, valueR;
	valueL = ConstUnaryExp();
	while (symbol == MULT || symbol == DIV_WORD || symbol == MOD) {
		Memory tag = symbol;
		printMessage();    //输出*或/信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		valueR = ConstUnaryExp();
		if (tag == MULT) valueL = valueL * valueR;
		else if (tag == DIV_WORD) valueL = valueL / valueR;
		else valueL = valueL % valueR;
	}
	//退出前已经预读
	outfile << "<常量项>" << endl;
	return valueL;
}
int ConstUnaryExp()	//'(' Exp ')' | LVal | Number | Ident '(' [FuncRParams] ')' | +|- UnaryExp 			
{				//Ident '(' [FuncRParams] ')'不可能有
	int value;
	if (symbol == LPARENT) {  //'(' Exp ')'
		printMessage();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //预读
		value = ConstExp();
		printMessage();			//输出)
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //预读
	}
	else if(symbol==INTCON){      //symbol == INTCON
		value = wordAnalysis.getNumber();   //获取数值
		printMessage();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //预读
	}
	else if (symbol == PLUS || symbol == MINU) {  //+|- UnaryExp
		int flag = 1;
		if (symbol == MINU) flag = -1;
		printMessage();
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
			printMessage();//输出[信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
			value = ConstExp();
			int offset = 1;
			for (int j = nowDimenson; j < dimenLength.size(); j++) {
				offset = offset * dimenLength[j];
			}
			totaloffset += value*offset;
			printMessage();//输出]信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
		}
		value = item.getIntValue()[totaloffset];
	}
	outfile << "<常量因子>" << endl;
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
		if(isNesting == 1){
			isNesting = 1;
			Ndimension++;
			for (int pp = Ndimension; pp < matrixLength.size();pp++) {   //计算当前维度偏移量mod
				mod = mod * matrixLength[pp];
			}
		}else{
			if(isBegin == 0){
			    isBegin = 1;
				isNesting = 0;
			}else{
				isNesting = 1;
			}
			for (int pp = 1; pp < matrixLength.size();pp++) {   //计算当前维度偏移量mod
				mod = mod * matrixLength[pp];
			}
			Ndimension = 1;
			while(offset%mod!=0){
				Ndimension++;
				mod = mod / matrixLength[Ndimension-1];
			}
		}
		printMessage();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //预读
		if (symbol != RBRACE) {
			ConstInitVal(index);
			while (symbol == COMMA) {   //读到逗号需要判断左右两个数的维度
				isNesting = 0;
				printMessage();
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken(); //预读
				ConstInitVal(index);
			}
		}
		else {    //空情况
			flag = 1;
		}
		if (flag == 1) offset += mod;						//{}，直接加当前维度大小
		else {												//补齐当前维度
			while (offset % mod != 0) {           
				offset++;
			}
		}
		printMessage();   //输出 } 信息
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
				CodeItem citem = CodeItem(STORE, value_string, b+nodeName, offset_string);
				citem.setFatherBlock(fatherBlock);
				codetotal[index].push_back(citem);
			}
		}
	}
	outfile << "<常量初值>" << endl;
}
void VarDecl(int index,int block)			   //变量说明
{
	printMessage();   //输出int信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读 
	VarDef(index,block);   //已经预读一个单词，直接进入即可
	while (symbol == COMMA) {
		printMessage();    //输出,信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读 
		VarDef(index,block);   //已经预读一个单词，直接进入即可
	}
	printMessage();    //输出；信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读 
	//退出循环前均已经预读单词
	outfile << "<变量说明>" << endl;
}
void VarDef(int index,int block)             //变量定义
{
	int value;
	vector<int> length; //记录每个维度大小
	printMessage();	//输出变量名信息
	string name = token;	//记录变量名
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();  //预读
	int dimenson = 0;			//维度信息
	while (symbol == LBRACK) {
		dimenson++;
		printMessage();   //输出[
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		length.push_back(ConstExp());   //记录常量维度
		printMessage();   //输出]
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	symbolTable item = symbolTable(VARIABLE, INT, name,dimenson,block);
	item.setMatrixLength(length);
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
	CodeItem citem = CodeItem(codetype, b + name, "", numToString(totalSize));
	citem.setFatherBlock(fatherBlock);
	codetotal[index].push_back(citem);
	if (symbol == ASSIGN) {
		printMessage();   //输出=
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读


		//变量赋值前做好变量初始化相关工作
		offset = 0;
		matrixLength = length;
		Ndimension = 0;
		isNesting = 0;
		isBegin = 0;
		nodeName = name;
		
		InitVal(index);
	}
	//退出循环前已经预读单词
	outfile << "<变量定义>" << endl;
}
void InitVal(int index)
{
	int flag = 0;
	if (symbol == LBRACE) {
		int mod = 1;
		if(isNesting == 1){
			isNesting = 1;
			Ndimension++;
			for (int pp = Ndimension; pp < matrixLength.size();pp++) {   //计算当前维度偏移量mod
				mod = mod * matrixLength[pp];
			}
		}else{
			if(isBegin == 0){
			    isBegin = 1;
				isNesting = 0;
			}else{
				isNesting = 1;
			}
			for (int pp = 1; pp < matrixLength.size();pp++) {   //计算当前维度偏移量mod
				mod = mod * matrixLength[pp];
			}
			Ndimension = 1;
			while(offset%mod!=0){
				Ndimension++;
				mod = mod / matrixLength[Ndimension-1];
			}
		}
		printMessage();   //输出{
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		if (symbol != RBRACE) {
			InitVal(index);
			while (symbol == COMMA) {
				isNesting = 0;
				printMessage();   //输出,
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//预读
				InitVal(index);
			}
		}
		else {    //空情况
			flag = 1;
		}
		if (flag == 1) offset += mod;
		else {
			while (offset % mod != 0) {
				offset++;
			}
		}
		printMessage();   //输出}
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else {
		isNesting = 0;
		offset++;
		Exp();
		if (matrixLength.size() == 0) {
			string b = "@";
			symbolTable item = checkItem(nodeName);
			if (Range != 0) {
				b = "%";
			}
			CodeItem citem = CodeItem(STORE,interRegister,b+nodeName,"0");	//赋值单值
			citem.setFatherBlock(fatherBlock);
			codetotal[index].push_back(citem);
		}
		else {
			if (offset <= totalSize) {
				string offset_string = numToString((offset - 1)*4);
				string b = "@";
				symbolTable item = checkItem(nodeName);
				if (Range != 0) {
					b = "%";
				}
				CodeItem citem = CodeItem(STORE, interRegister, b + nodeName, offset_string);
				citem.setFatherBlock(fatherBlock);
				codetotal[index].push_back(citem);
			}
		}
	}
	outfile << "<变量初值>" << endl;
}
void valueFuncDef()    //有返回值函数定义
{
	printMessage();    //输出int信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	string name = token;   //获取函数名
	printMessage();   //获得标识符并输出
	symbolTable item(FUNCTION, INT, name);
	total[Funcindex].push_back(item);
	CodeItem citem(DEFINE, "@"+name,"int","");           //define @foo int
	codetotal[Funcindex].push_back(citem);
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	printMessage();   //获得(并输出
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	if (symbol == RPARENT) {  //参数表为空
		total[Funcindex][0].setparaLength(0);   //函数参数为0
		outfile << "<参数表>" << endl;
		printMessage();    //输出)信息
	}
	else {
		FuncFParams();  //进入前已经预读
		printMessage();    //输出)信息
		//判断symbol=)
	}
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //预读
	Blockindex = 0;
	Block();
	outfile << "<有返回值函数定义>" << endl;
}
void novalueFuncDef()  //无返回值函数定义
{
	printMessage();    //输出void信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//获得标识符
	string name = token;  //保存函数名
	symbolTable item(FUNCTION, VOID, name);
	total[Funcindex].push_back(item);
	CodeItem citem(DEFINE, "@" + name, "void", "");           //define @foo int
	codetotal[Funcindex].push_back(citem);
	printMessage();   //输出标识符
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	printMessage();   //获得(并输出
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	if (symbol == RPARENT) {
		total[Funcindex][0].setparaLength(0);   //函数参数为0
		outfile << "<参数表>" << endl;
		printMessage();    //先输出参数表后输出)信息
	}
	else {
		FuncFParams();
		printMessage();    //输出)信息
		//判断)
	}
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	Blockindex = 0;
	Block();
	outfile << "<无返回值函数定义>" << endl;
}
void Block()      //语句块
{
	Blockindex++;
	fatherBlock.push_back(Blockindex);
	int block = Blockindex;
	printMessage();    //输出{信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	while (symbol != RBRACE) {
		if (symbol == CONSTTK) {
			ConstDecl(Funcindex,block);
		}
		else if (symbol == INTTK) {
			VarDecl(Funcindex,block);
		}
		else {
			Stmt();
		}
	}
	printMessage();    //输出}信息
	fatherBlock.pop_back();    //删除末尾元素
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	outfile << "<语句块>" << endl;
}
void FuncFParams()         //参数表
{
	int paraLength = 1;//参数个数   
	FuncFParam();
	while (symbol == COMMA) {
		paraLength++;
		printMessage();    //输出,信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();
		FuncFParam();
	}
	total[Funcindex][0].setparaLength(paraLength);
	//退出前已经预读
	outfile << "<函数形参表>" << endl;
}
void FuncFParam()
{
	vector<int> length; //记录每个维度大小
	printMessage();    //输出int信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	string name = token;   //保存参数名
	printMessage();    //输出变量名信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	int dimenson = 0;
	if (symbol == LBRACK) {
		dimenson++;
		printMessage();    //输出[信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();
		printMessage();    //输出]信息
		length.push_back(0);
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();
		while (symbol == LBRACK) {
			dimenson++;
			printMessage();    //输出[信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
			length.push_back(ConstExp());
			printMessage();    //输出]信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();  //预读
		}
	}
	symbolTable item = symbolTable(PARAMETER, INT, name, dimenson,1);
	item.setMatrixLength(length);
	total[Funcindex].push_back(item);
	names[Funcindex][name]++;
	string b = "int";
	if (dimenson > 0) {
		b = "int*";
	}
	CodeItem citem = CodeItem(PARA,"%"+name,b,numToString(dimenson));//函数形参
	citem.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem);
	outfile << "<函数形参>" << endl;
}
void Exp()	//MulExp{('+'|'-')MulExp}
{
	Memory symbol_tag;
	string registerL, registerR;
	MulExp();  //退出前读了一个单词
	registerL = interRegister;
	while (symbol == PLUS || symbol == MINU) {
		symbol_tag = symbol;
		printMessage();    //输出+或-信息
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
		}else{
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
	outfile << "<表达式>" << endl;
	return ;
}
void MulExp()	//UnaryExp {('*'|'/'|'%') UnaryExp  }
{
	UnaryExp();
	string registerL = interRegister;	//获取左计算数
	while (symbol == MULT || symbol == DIV_WORD || symbol == MOD) {
		Memory symbol_tag = symbol;
		printMessage();    //输出*或/信息
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
	outfile << "<项>" << endl;
}
void UnaryExp()			// '(' Exp ')' | LVal | Number | Ident '(' [FuncRParams] ')' | +|-|! UnaryExp 
{
	if (symbol == LPARENT) {   //'(' Exp ')'
		printMessage();    //输出(信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		Exp();
		printMessage();    //输出)信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else if (symbol == INTCON){  //number
		printMessage();
		interRegister = numToString(wordAnalysis.getNumber());		//将数值转换成字符串模式
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else if (symbol == IDENFR) {  //标识符 (函数实参表) | 标识符 {'['表达式']'}
		printMessage();    //输出变量信息
		string name_tag = wordAnalysis.getToken();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		string registerL,registerR,registerA,registerC;
		int typeL,typeR,typeA,typeC;
		if (symbol == LPARENT) {  //标识符 (函数实参表)
			paraNum = 0;
			Functionname = name_tag;
			printMessage();    //输出(信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
			if (symbol != RPARENT) {
				FuncRParams();
			}
			printMessage();    //输出)信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
			interRegister = "%" + numToString(Temp);		//存函数返回结果
			Temp++;
			CodeItem citem = CodeItem(CALL,"@"+Functionname,interRegister,numToString(paraNum));          //call @foo %3 3
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);//函数引用
		}
		else {  //标识符 {'['表达式']'}
			symbolTable item = checkItem(name_tag);
			int range = Range;
			vector<int> dimenLength = item.getMatrixLength();
			if (dimenLength.size() > 0 && symbol == LBRACK) {
				registerA = "0";	//偏移量为0
			}
			int nowDimenson = 0;   //记录当前维度
			while (symbol == LBRACK) {
				nowDimenson++;
				printMessage();    //输出[信息
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//预读
				Exp();		//q[n][m]
				registerL = interRegister;   //registerL = "n"	，该维度偏移量
				int offset = 1;
				for (int j = nowDimenson; j < dimenLength.size(); j++) {
					offset = offset * dimenLength[j];
				}
				registerR = numToString(offset);		//该维度值为1时大小
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
				printMessage();    //输出]信息
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//预读
			}
			if (item.getDimension() == nowDimenson && item.getDimension() > 0) {	//取多维数组的一个元素
				if (registerA[0] != '@' && registerA[0] != '%') {
					int index = stringToNum(registerA);
					registerA = numToString(index * 4);
				}
				else {
					interRegister = "%" + numToString(Temp);
					Temp++;
					CodeItem citem = CodeItem(MUL, interRegister, registerA, "4");
					citem.setFatherBlock(fatherBlock);
					codetotal[Funcindex].push_back(citem);
					registerA = interRegister;
				}
				if (item.getForm() == CONSTANT && registerA[0] != '@' && registerA[0] != '%') {
					int offset = stringToNum(registerA) / 4;
					interRegister = numToString(item.getIntValue()[offset]);
				}
				else {
					interRegister = "%" + numToString(Temp);
					Temp++;
					string b = "%";
					if (range == 0) {
						b = "@";	//全局变量
					}
					CodeItem citem = CodeItem(LOAD, interRegister, b + name_tag, registerA); //数组取值
					citem.setFatherBlock(fatherBlock);    //保存当前作用域
					codetotal[Funcindex].push_back(citem);
				}
			}
			else if (item.getDimension() > 0 && nowDimenson < item.getDimension()) {		//此时只可能是传参，传数组的一部分
				paraIntNode = 1;
				string b = "%";
				if (range == 0) {
					b = "@";	//全局变量
				}
				interRegister = b+name_tag;
			}
			else {
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
					CodeItem citem = CodeItem(LOAD, interRegister, b+name_tag, "0"); //一维变量、常量取值
					citem.setFatherBlock(fatherBlock);
					codetotal[Funcindex].push_back(citem);
				}
			}
		}
	}
	else { //+| -| !  UnaryExp
		string op,registerL,registerR;
		int typeL,typeR;
		if (symbol == PLUS){
			printMessage();    //输出+信息
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//预读
			UnaryExp();
		}
		else if (symbol == MINU) {
			printMessage();    //输出-信息
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
			printMessage();    //输出!信息
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
	outfile << "<因子>" << endl;
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
		printMessage();    //输出break信息
		CodeItem citem = CodeItem(BR, whileLabel[whileLabel.size() - 2], "", ""); //br %while.cond 
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		printMessage();    //输出;信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else if (symbol == CONTINUETK) {	//continue ;
		printMessage();    //输出continue信息
		CodeItem citem = CodeItem(BR, whileLabel[whileLabel.size() - 1], "", ""); //br %while.cond 
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		printMessage();    //输出;信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else if (symbol == LBRACE) { //'{'｛声明|语句｝'}'
		Block();
	}
	else if (symbol == RETURNTK) { //＜返回语句＞;
		returnStmt();
		printMessage();    //输出;信息
		//判断symbol=;
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else if (symbol == SEMICN) {
		printMessage();    //输出;信息
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
		if (symbol == LPARENT) {	//该语句为调用无返回值函数；
			wordAnalysis.setfRecord(record_tag);
			wordAnalysis.setSymbol(sym_tag);
			wordAnalysis.setToken(token_tag);
			symbol = sym_tag;
			Exp();					//CALL函数在EXP中存在
			printMessage();  //输出分号
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();  //预读
		}
		else {
			while (symbol != SEMICN and symbol != ASSIGN) {
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
				printMessage();    //输出;信息
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//预读
			}
			else {		//Exp ;  这里直接跳过
				while (symbol != SEMICN) {
					wordAnalysis.getsym();
					symbol = wordAnalysis.getSymbol();
				}
				printMessage();    //输出;信息
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//预读
			}
		}
	}
	else {						//Exp ; 这里直接跳过，没影响
		while (symbol != SEMICN) {
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
		}
		printMessage();    //输出;信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	//退出前均预读
	outfile << "<语句>" << endl;
}
void assignStmt()        //赋值语句 LVal = Exp
{
	printMessage();    //输出变量信息
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
	while (symbol == LBRACK) {
		nowDimenson++;
		printMessage();    //输出[信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		Exp();
		registerL = interRegister;		//该维度偏移量
		int offset = 1;
		for (int j = nowDimenson; j < dimenLength.size(); j++) {
			offset = offset * dimenLength[j];
		}
		registerR = numToString(offset);		//该维度值为1时大小
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
		printMessage();    //输出]信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	printMessage();    //输出=信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	Exp();
	if (dimenLength.size() > 0) {
		string tempRegister = interRegister;
		if (registerA[0] != '@' && registerA[0] != '%') {
			int index = stringToNum(registerA);
			registerA = numToString(index * 4);
		}
		else {
			interRegister = "%" + numToString(Temp);
			Temp++;
			CodeItem citem = CodeItem(MUL, interRegister, registerA, "4");
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
			registerA = interRegister;
		}
		CodeItem citem = CodeItem(STORE, tempRegister, b + name_tag, registerA);		//数组某个变量赋值
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
	}
	else {
		CodeItem citem = CodeItem(STORE, interRegister, b+name_tag, "0");		//给一维变量赋值
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
	}
	outfile << "<赋值语句>" << endl;
}
void ifStmt()            //条件语句
{
	printMessage();    //输出if信息
	//判断symbol=if
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	printMessage();   //获得(并输出
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //预读
	Cond();
	printMessage();    //输出)信息
	//判断symbol=)
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	string if_then_label = "%if.then_" + numToString(iflabelIndex);
	string if_else_label = "%if.else_" + numToString(iflabelIndex);
	string if_end_label = "%if.end_" + numToString(iflabelIndex);
	iflabelIndex++;
	CodeItem citem = CodeItem(BR,interRegister,if_then_label,if_else_label); //br 条件 %if.then_1 %if.else_1 
	citem.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem);
	CodeItem citem2 = CodeItem(LABEL,if_then_label,"","");	//label if.then
	citem2.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem2);
	int nowIndex = codetotal[Funcindex].size() - 2;		//暂存 br 条件 %if.then_1 %if.else_1  下标，可能后续要把%if.else改成 %if.end
	Stmt();													//stmt1
	if (symbol == ELSETK) {
		CodeItem citem3 = CodeItem(BR, if_end_label, "", "");   //BR if.end
		citem3.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem3);
		CodeItem citem4 = CodeItem(LABEL, if_else_label, "", "");	//label if.else
		citem4.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem4);
		printMessage();    //输出else信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		Stmt();			//stmt2
	}
	else {	//只有if  没有else，只需要把br res %if.then, %if.else中的%if.else标签改成%if.end即可
		string res = codetotal[Funcindex][nowIndex].getResult();
		codetotal[Funcindex][nowIndex].changeContent(res, if_then_label, if_end_label);
	}
	CodeItem citem5 = CodeItem(BR, if_end_label, "", "");   //BR if.end
	citem5.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem5);
	CodeItem citem6 = CodeItem(LABEL, if_end_label, "", "");	//label if.else
	citem6.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem6);
	//退出前Stmt均预读
	outfile << "<条件语句>" << endl;
}
void Cond()              //条件表达式(逻辑或表达式)  LAndExp { '||' LAndExp}
{
	string registerL, registerR,op;
	LAndExp();
	registerL = interRegister;
	while (symbol == OR_WORD) {
		Memory symbol_tag = symbol;
		printMessage();    //输出逻辑运算符
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		LAndExp();
		registerR = interRegister;
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
	outfile << "<条件>" << endl;
}
void LAndExp()			  //逻辑与表达式   EqExp{'&&' EqExp }
{
	string registerL, registerR, op;
	EqExp();
	registerL = interRegister;
	while (symbol == AND_WORD) {
		Memory symbol_tag = symbol;
		printMessage();    //输出逻辑运算符
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		EqExp();
		registerR = interRegister;
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

void EqExp()		  //相等性表达式
{
	string registerL, registerR, op;
	RelExp();
	registerL = interRegister;
	while (symbol == EQL_WORD || symbol == NEQ_WORD) {
		Memory symbol_tag = symbol;
		printMessage();    //输出逻辑运算符
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
	while (symbol == LSS || symbol == LEQ||symbol==GRE||symbol==GEQ){
		Memory symbol_tag = symbol;
		printMessage();    //输出逻辑运算符
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
void loopStmt()          //循环语句
{
	//?while '('＜条件＞')'＜语句＞
	printMessage();    //输出while信息
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	printMessage();   //获得(并输出信息
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
	printMessage();    //输出)信息
	CodeItem citem2 = CodeItem(BR, interRegister, while_body_label, while_end_label); //br 条件 %while.body %while.end  
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
	CodeItem citem4 = CodeItem(BR, while_cond_label,"",""); //br %while.cond 
	citem4.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem4);
	CodeItem citem5 = CodeItem(LABEL, while_end_label, "", "");	//label %while.end
	citem5.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem5);
	//退出前均预读
	outfile << "<循环语句>" << endl;
}
void FuncRParams()    //函数实参数表
{
	if (symbol == RPARENT) {
		return;
	}
	if (symbol == STRING) {
		interRegister = wordAnalysis.getToken();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
	}
	else {
		Exp();//退出前已经预读
	}
	paraNum++;
	if (paraIntNode == 0) {
		if(interRegister[0]=='\"'){
			CodeItem citem = CodeItem(PUSH, interRegister, "", "string");  //传参
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
		else {
			CodeItem citem = CodeItem(PUSH, interRegister, "", "int");  //传参
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
	}
	else {
		CodeItem citem = CodeItem(PUSH, interRegister, "", "int*");  //传参
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
		paraIntNode = 0;
	}
	while (symbol == COMMA) {
		printMessage();    //输出,信息
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//预读
		Exp();
		paraNum++;
		if (paraIntNode == 0) {
			CodeItem citem = CodeItem(PUSH, interRegister, "", "int");  //传参
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
		else {
			CodeItem citem = CodeItem(PUSH, interRegister, "", "int*");  //传参
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
			paraIntNode = 0;
		}
	}
	//退出循环前已经预读
	outfile << "<函数实参数表>" << endl;
}
void returnStmt()        //返回语句
{
	printMessage();    //输出return信息
	//判断symbol=return
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//预读
	if (symbol != SEMICN) {
		Exp();
		CodeItem citem = CodeItem(RET, interRegister, "int","");  //有返回值返回
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
	}
	else {
		CodeItem citem = CodeItem(RET,"","void","");
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
	}
	outfile << "<返回语句>" << endl;
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
		if (res.size()>0 && res[0] == '%' && (!isdigit(res[1])) ) {  //res必须是变量
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