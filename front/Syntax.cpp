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
const char* print[25] = {
	"IDENFR ","INTCON ","PLUS ","MINU ","MULT ","DIV1 ","MOD ","AND1 ","OR1 ",
	"LSS ","LEQ ","GRE ","GEQ ","EQL1 ","NEQ1 ","REV ","ASSIGN ","SEMICN ","COMMA ",
	"LPARENT ","RPARENT ","LBRACK ","RBRACK ","LBRACE ","RBRACE "
};
const char* premain[10] = {
	"CONSTTK", "INTTK", "VOIDTK", "MAINTK", "IFTK", "ELSETK",
	"WHILETK", "BREAKTK", "CONTINUETK", "RETURNTK"
};
//�����Զ��庯��
string Functionname;	//��¼��ǰ������

//���ű��������
int Funcindex = 0;		//�������ű���table����λ��
symbolTable searchItem; //���ҵķ��ű�
vector<symbolTable> table;      //�ַ��ű�
vector<vector<symbolTable>> total;	//�ܷ��ű�
vector<int> fatherBlock;	//���и�block��
int Blockindex;			//�����е�ǰblock��,���������Ͳ���block��Ϊ1
vector< map<string, int> > names;	//���б�����������������

//���鸳ֵ�������
int offset;						//��ǰά����ƫ����
int totalSize;					//�ܴ�С
vector<int> matrixLength;		//��ǰ����ÿ��ά��ֵ
int Ndimension;					//��ǰǶ��ά��
string nodeName;				//������
int isNesting;                  //�Ƿ�Ƕ�ף���{{
int isBegin;					//�Ƿ�ͷ����ǰ���{����

int Temp = 0;				//�м������ʼ���
int iflabelIndex;          //if��ǩ�±�
int whilelabelIndex;		//while��ǩ�±�
vector<string> whileLabel;      //����break��continue��Ӧ�ı�ǩ
int paraNum;				//���ú���ʱ��¼��������

int interIndex;          //�м�����±�
string interRegister;        //�м�����ַ���
int interType;				 //�м��������

vector<vector<CodeItem>> codetotal;   //���м����
string numToString(int a)
{
	stringstream trans;          //���ֺ��ַ����໥ת������
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
int Range;									//������  0��ʾȫ��   ��0��ʾ�ֲ�
symbolTable checkItem(string checkname)             //����ڴ���ʱ�жϱ����Ƿ�Ϊ����ʱʹ��
{
	int i, j, checkblock, block;
	string name;
	for (j = fatherBlock.size() - 1; j >= 0; j--) {
		checkblock = fatherBlock[j];
		for (i = 0; i < total[Funcindex].size(); i++) {   //�ȴӱ�������������
			name = total[Funcindex][i].getName();
			block = total[Funcindex][i].getblockIndex();
			if (name == checkname && block == checkblock) {  //��������������ҵ���
				Range = Funcindex;
				return total[Funcindex][i];
			}
		}
	}
	for (i = 0; i < total[0].size(); i++) {
		name = total[0][i].getName();
		if (name == checkname) {  //��������������ҵ���
			Range = 0;
			return total[0][i];
		}
	}
}
//����Ŀ�������������
int paraIntNode = 0;		  //��¼�����Ƿ���Ϊ����
string matrixName;			  //��¼������

//�ʷ����﷨�����������
string token;   //���������ĵ���
enum Memory symbol;  //���������ĵ������
ofstream outfile;
Word wordAnalysis("testexample.txt");
void printMessage()
{
	string message;
	if (symbol > 9 && symbol != 35) {
		token = wordAnalysis.getToken();
		message = print[symbol - 10] + token;
	}
	else if (symbol >= 0 && symbol <= 9) {
		message = premain[symbol] + remain[symbol];
	}
	outfile << message << endl;
}
void CompUnit();			   //���뵥Ԫ
void ConstDecl(int index,int block);		       //����˵��
void ConstDef(int index,int block);			   //��������
void VarDecl(int index,int block);			   //����˵��
void VarDef(int index,int block);             //��������
void InitVal(int index);				//������ֵ
void valueFuncDef();    //�з���ֵ��������
void novalueFuncDef();  //�޷���ֵ��������
void Block();      //�������
int ConstExp();			//�������ʽ         �Ͳ�ֱ��ʹ�ñ��ʽ��
int ConstMulExp();			//������
int ConstUnaryExp();			//��������
void ConstInitVal(int index);      //������ֵ������ά��ֵ
void FuncFParams();         //������
void FuncFParam();         //�����β�
void Exp();					    //���ʽ(�Ӽ����ʽ)
void MulExp();					//�˳�ģ���ʽ
void UnaryExp();					//һԪ���ʽ
void Stmt();              //���
void assignStmt();        //��ֵ���
void ifStmt();            //�������
void Cond();              //�������ʽ(�߼�����ʽ)
void loopStmt();          //ѭ�����
void FuncRParams();		  //ֵ������
void returnStmt();        //�������
void LAndExp();			  //�߼�����ʽ
void EqExp();			  //����Ա��ʽ
void RelExp();			  //��ϵ���ʽ

string newName(string name, int blockindex)
{
	stringstream trans;          //���ֺ��ַ����໥ת������
	trans << blockindex;
	return name + "-" + trans.str();
}
symbolTable checkTable(string checkname, int function_number, vector<int> fatherBlock);					//����Ľ��м����ͷ��ű�ʱʹ��
void change(int index);				//�޸��м���롢���ű�


int main()
{
	outfile.open("output.txt");
	//�ȶ�һ������Ȼ������������
	wordAnalysis.getsym();   //���ﲻ��Ҫʹ���䷵�ص�bool����ֵ
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	CompUnit();
	//Ϊ����VOID���ͺ������û��return�������return
	for (int i = 0; i < codetotal.size(); i++) {
		vector<CodeItem> item = codetotal[i];
		int size = item.size();
		if (size > 0) {
			if (item[0].getCodetype() == DEFINE && item[0].getOperand1() == "void") {
				if (item[size - 1].getCodetype() != RET) {
					CodeItem citem = CodeItem(RET,"","","");
					citem.setFatherBlock(fatherBlock);
					codetotal[i].push_back(citem);
				}
			}
		}
	}
	//�Զ�Ϊ�������ϵ�һά��С
	for (int i = 0; i < total.size(); i++) {
		vector<symbolTable> item = total[i];
		for (int j = 0; j < item.size(); j++) {
			if (item[j].getForm() == PARAMETER) {
				int size = item[j].getDimension();
				if (size > 0) total[i][j].setfirstNodesize(0);
			}
		}
	}
	//���ű�����ּ�"@"��"%"
	for (int i = 0; i < total.size(); i++) {
		string b = "%";
		if (i == 0) b = "@";
		for (int j = 0; j < total[i].size(); j++) {
			total[i][j].changeName(b + total[i][j].getName());
		}
	}
	//�����ű�����
	/*
	cout << "���� " << "Block�±� " << "���� 0Con 1Var 2Para 3Func " << "ά�� " << endl;
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
		cout << "\n";   //һ��������һ��
	}
	*/
	//������鸳ֵ��ȷ�� testcase8
	/*
	for (int i = 0; i < total[0].size(); i++) {
		vector<int> values = total[0][i].getIntValue();
		for (int j = 0; j < values.size(); j++) {
			cout << values[j] << " ";
		}
		cout << "\n";
	}
	*/
	//����м������ȷ��
	for (int i = 0; i < codetotal.size(); i++) {
		vector<CodeItem> item = codetotal[i];
		for (int j = 0; j < item.size(); j++) {
			cout << item[j].getContent() << endl;
		}
		cout << "\n";
	}
	//�Ż��м����

	//�������


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
		if (symbol == CONSTTK) {    //��������
			ConstDecl(0,0);
		}
		else {   //�����������ߺ�������                
			if (symbol == INTTK) {
				Memory sym_tag = symbol;
				string token_tag = wordAnalysis.getToken();
				int record_tag = wordAnalysis.getfRecord();
				wordAnalysis.getsym();   //��ʶ��
				wordAnalysis.getsym();   //��(��Ϊ�з���ֵ��������
				symbol = wordAnalysis.getSymbol();
				//�ָ���������
				wordAnalysis.setfRecord(record_tag);
				wordAnalysis.setSymbol(sym_tag);
				wordAnalysis.setToken(token_tag);
				if (symbol == LPARENT) {   //��������
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
					change(Funcindex);		//�޸��м���롢���ű�
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
				change(Funcindex);			//�޸��м���롢���ű�
			}
		}
	}
	outfile<<"<���뵥Ԫ>"<<endl;
}
void ConstDecl(int index,int block) 		   //����˵��
{
	printMessage();    //���const��Ϣ
	//�ж�symbol==CONSTTK
	wordAnalysis.getsym();  
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	printMessage();    //���int��Ϣ
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //Ԥ��
	ConstDef(index,block);  //��������
	//�ж�symbol=;
	while (symbol == COMMA) {
		printMessage();   //���,��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();
		ConstDef(index,block);  //��������
	}
	printMessage();    //�������Ϣ
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //Ԥ��
	//�˳�ѭ��ǰ�Ѿ�Ԥ������
	outfile << "<����˵��>" << endl;
}
void ConstDef(int index,int block)			   //��������
{
	int value;
	vector<int> length; //��¼ÿ��ά�ȴ�С
	printMessage();   //�����ʶ����Ϣ
	string name = token;	//���泣����
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //Ԥ��
	int dimenson = 0;
	while (symbol == LBRACK) {
		dimenson++;
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();  //Ԥ��
		length.push_back(ConstExp());   //��¼����ά��
		printMessage();   //���]��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //Ԥ��
	}
	symbolTable item = symbolTable(CONSTANT, INT, name,dimenson,block);
	item.setMatrixLength(length);
	total[index].push_back(item);
	names[index][name]++;
	printMessage();   //���=��Ϣ
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //Ԥ��

	
	//������ֵǰ���ñ�����ʼ����ع���
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
		b = "@";
	}
	else {
		codetype = GLOBAL;
		b = "%";
	}
	if (totalSize == 1) {
		int size = total[index].size();
		constValue = numToString(total[index][size - 1].getIntValue()[0]);
	}
	CodeItem citem = CodeItem(codetype, b + name, constValue, numToString(totalSize));
	citem.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem);
	ConstInitVal(index);
	//�˳�ѭ��ǰ���Ѿ�Ԥ������
	outfile << "<��������>" << endl;
}
int ConstExp()
{
	int valueL, valueR;
	valueL= ConstMulExp();  //�˳�ǰ����һ������
	while (symbol == PLUS || symbol == MINU) {
		Memory tag = symbol;
		printMessage();    //���+��-��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		valueR = ConstMulExp();
		if (tag == PLUS) valueL = valueL + valueR;
		else valueL = valueL - valueR;
	}
	//�˳�ǰ�Ѿ�Ԥ��
	outfile << "<�������ʽ>" << endl;
	return valueL;
}
int ConstMulExp()
{
	int valueL, valueR;
	valueL = ConstUnaryExp();
	while (symbol == MULT || symbol == DIV1 || symbol == MOD) {
		Memory tag = symbol;
		printMessage();    //���*��/��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		valueR = ConstUnaryExp();
		if (tag == MULT) valueL = valueL * valueR;
		else if (tag == DIV) valueL = valueL / valueR;
		else valueL = valueL % valueR;
	}
	//�˳�ǰ�Ѿ�Ԥ��
	outfile << "<������>" << endl;
	return valueL;
}
int ConstUnaryExp()	//'(' Exp ')' | LVal | Number | Ident '(' [FuncRParams] ')' | +|- UnaryExp 			
{				//Ident '(' [FuncRParams] ')'��������
	int value;
	if (symbol == LPARENT) {  //'(' Exp ')'
		printMessage();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //Ԥ��
		value = ConstExp();
		printMessage();			//���)
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //Ԥ��
	}
	else if(symbol==INTCON){      //symbol == INTCON
		value = wordAnalysis.getNumber();   //��ȡ��ֵ
		printMessage();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //Ԥ��
	}
	else if (symbol == PLUS || symbol == MINU) {  //+|- UnaryExp
		int flag = 1;
		if (symbol == MINU) flag = -1;
		printMessage();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //Ԥ��
		value = flag * ConstUnaryExp();
	}
	else {	//Ident{'[' Exp ']'}  Ҫ��1.Identһ���ǳ��� 2.��ֱ����ֵ(Ӧ����ConstExp) 
		string name_tag = wordAnalysis.getToken();
		wordAnalysis.getsym();
		int totaloffset = 0;
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		symbolTable item = checkItem(name_tag);
		vector<int> dimenLength = item.getMatrixLength();
		int nowDimenson = 0;
		while (symbol == LBRACK) {
			nowDimenson++;
			printMessage();//���[��Ϣ
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//Ԥ��
			value = ConstExp();
			int offset = 1;
			for (int j = nowDimenson; j < dimenLength.size(); j++) {
				offset = offset * dimenLength[j];
			}
			totaloffset += value*offset;
			printMessage();//���]��Ϣ
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//Ԥ��
		}
		value = item.getIntValue()[totaloffset];
	}
	outfile << "<��������>" << endl;
	return value;
}
void ConstInitVal(int index)	//ConstExp | '{' [ConstInitVal{ ',' ConstInitVal } ] '}'  
{
	/*
	matrixLength;		//��ǰ����ÿ��ά�ȴ�С
	offset;				//ƫ����
	Ndimension;			//��ǰά��
	*/
	//positionRecord����������������ӣ�1�Ƕ���}��2�Ƕ���{ offset!=0
	int value;
	int flag = 0;
	if (symbol == LBRACE) {
		int mod = 1;     //ƫ����
		if(isNesting == 1){
			isNesting = 1;
			Ndimension++;
			for (int pp = Ndimension; pp < matrixLength.size();pp++) {   //���㵱ǰά��ƫ����mod
				mod = mod * matrixLength[pp];
			}
		}else{
			if(isBegin == 0){
			    isBegin = 1;
				isNesting = 0;
			}else{
				isNesting = 1;
			}
			for (int pp = 1; pp < matrixLength.size();pp++) {   //���㵱ǰά��ƫ����mod
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
		token = wordAnalysis.getToken(); //Ԥ��
		if (symbol != RBRACE) {
			ConstInitVal(index);
			while (symbol == COMMA) {   //����������Ҫ�ж�������������ά��
				isNesting = 0;
				printMessage();
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken(); //Ԥ��
				ConstInitVal(index);
			}
		}
		else {    //�����
			flag = 1;
		}
		if (flag == 1) offset += mod;						//{}��ֱ�Ӽӵ�ǰά�ȴ�С
		else {												//���뵱ǰά��
			while (offset % mod != 0) {           
				offset++;
			}
		}
		printMessage();   //��� } ��Ϣ
		//ÿ�ζ��� } ��ƫ����offset���lastDimension��������
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken(); //Ԥ��
	}
	else {
		isNesting = 0;
		offset++;
		value = ConstExp();
		int size = total[index].size() - 1;
		//cout << value << endl;
		if (matrixLength.size() == 0) {
			total[index][size].setIntValue(value, offset - 1); //��ֵ
		}
		else {
			if (offset <= totalSize) {
				total[index][size].setIntValue(value, offset - 1); //��ֵ
				string offset_string = numToString(offset-1);
				string value_string = numToString(value);
				string b = "@";
				symbolTable item = checkItem(nodeName);
				if (Range != 0) {
					b = "%";
				}
				CodeItem citem = CodeItem(STORE, value_string, b+nodeName, offset_string);
				citem.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem);
			}
		}
	}
	outfile << "<������ֵ>" << endl;
}
void VarDecl(int index,int block)			   //����˵��
{
	printMessage();   //���int��Ϣ
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//Ԥ�� 
	VarDef(index,block);   //�Ѿ�Ԥ��һ�����ʣ�ֱ�ӽ��뼴��
	while (symbol == COMMA) {
		printMessage();    //���,��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ�� 
		VarDef(index,block);   //�Ѿ�Ԥ��һ�����ʣ�ֱ�ӽ��뼴��
	}
	printMessage();    //�������Ϣ
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//Ԥ�� 
	//�˳�ѭ��ǰ���Ѿ�Ԥ������
	outfile << "<����˵��>" << endl;
}
void VarDef(int index,int block)             //��������
{
	int value;
	vector<int> length; //��¼ÿ��ά�ȴ�С
	printMessage();	//�����������Ϣ
	string name = token;	//��¼������
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();  //Ԥ��
	int dimenson = 0;			//ά����Ϣ
	while (symbol == LBRACK) {
		dimenson++;
		printMessage();   //���[
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		length.push_back(ConstExp());   //��¼����ά��
		printMessage();   //���]
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
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
		b = "@";
	}
	else {
		codetype = GLOBAL;
		b = "%";
	}
	CodeItem citem = CodeItem(codetype, b + name, "", numToString(totalSize));
	citem.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem);
	if (symbol == ASSIGN) {
		printMessage();   //���=
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��


		//������ֵǰ���ñ�����ʼ����ع���
		offset = 0;
		matrixLength = length;
		Ndimension = 0;
		isNesting = 0;
		isBegin = 0;
		nodeName = name;
		
		InitVal(index);
	}
	//�˳�ѭ��ǰ�Ѿ�Ԥ������
	outfile << "<��������>" << endl;
}
void InitVal(int index)
{
	int flag = 0;
	if (symbol == LBRACE) {
		int mod = 1;
		if(isNesting == 1){
			isNesting = 1;
			Ndimension++;
			for (int pp = Ndimension; pp < matrixLength.size();pp++) {   //���㵱ǰά��ƫ����mod
				mod = mod * matrixLength[pp];
			}
		}else{
			if(isBegin == 0){
			    isBegin = 1;
				isNesting = 0;
			}else{
				isNesting = 1;
			}
			for (int pp = 1; pp < matrixLength.size();pp++) {   //���㵱ǰά��ƫ����mod
				mod = mod * matrixLength[pp];
			}
			Ndimension = 1;
			while(offset%mod!=0){
				Ndimension++;
				mod = mod / matrixLength[Ndimension-1];
			}
		}
		printMessage();   //���{
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		if (symbol != RBRACE) {
			InitVal(index);
			while (symbol == COMMA) {
				isNesting = 0;
				printMessage();   //���,
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//Ԥ��
				InitVal(index);
			}
		}
		else {    //�����
			flag = 1;
		}
		if (flag == 1) offset += mod;
		else {
			while (offset % mod != 0) {
				offset++;
			}
		}
		printMessage();   //���}
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
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
			CodeItem citem = CodeItem(STORE,interRegister,b+nodeName,"0");	//��ֵ��ֵ
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
		else {
			if (offset <= totalSize) {
				string offset_string = numToString(offset - 1);
				string b = "@";
				symbolTable item = checkItem(nodeName);
				if (Range != 0) {
					b = "%";
				}
				CodeItem citem = CodeItem(STORE, interRegister, b + nodeName, offset_string);
				citem.setFatherBlock(fatherBlock);
				codetotal[Funcindex].push_back(citem);
			}
		}
	}
	outfile << "<������ֵ>" << endl;
}
void valueFuncDef()    //�з���ֵ��������
{
	printMessage();    //���int��Ϣ
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	string name = token;   //��ȡ������
	printMessage();   //��ñ�ʶ�������
	symbolTable item(FUNCTION, INT, name);
	total[Funcindex].push_back(item);
	CodeItem citem(DEFINE, "@"+name,"int","");           //define @foo int
	codetotal[Funcindex].push_back(citem);
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	printMessage();   //���(�����
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//Ԥ��
	if (symbol == RPARENT) {  //������Ϊ��
		total[Funcindex][0].setparaLength(0);   //��������Ϊ0
		outfile << "<������>" << endl;
		printMessage();    //���)��Ϣ
	}
	else {
		FuncFParams();  //����ǰ�Ѿ�Ԥ��
		printMessage();    //���)��Ϣ
		//�ж�symbol=)
	}
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //Ԥ��
	Blockindex = 0;
	Block();
	outfile << "<�з���ֵ��������>" << endl;
}
void novalueFuncDef()  //�޷���ֵ��������
{
	printMessage();    //���void��Ϣ
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//��ñ�ʶ��
	string name = token;  //���溯����
	symbolTable item(FUNCTION, VOID, name);
	total[Funcindex].push_back(item);
	CodeItem citem(DEFINE, "@" + name, "void", "");           //define @foo int
	codetotal[Funcindex].push_back(citem);
	printMessage();   //�����ʶ��
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	printMessage();   //���(�����
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//Ԥ��
	if (symbol == RPARENT) {
		total[Funcindex][0].setparaLength(0);   //��������Ϊ0
		outfile << "<������>" << endl;
		printMessage();    //���������������)��Ϣ
	}
	else {
		FuncFParams();
		printMessage();    //���)��Ϣ
		//�ж�)
	}
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//Ԥ��
	Blockindex = 0;
	Block();
	outfile << "<�޷���ֵ��������>" << endl;
}
void Block()      //����
{
	Blockindex++;
	fatherBlock.push_back(Blockindex);
	int block = Blockindex;
	printMessage();    //���{��Ϣ
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//Ԥ��
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
	printMessage();    //���}��Ϣ
	fatherBlock.pop_back();    //ɾ��ĩβԪ��
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//Ԥ��
	outfile << "<����>" << endl;
}
void FuncFParams()         //������
{
	int paraLength = 1;//��������   
	FuncFParam();
	while (symbol == COMMA) {
		paraLength++;
		printMessage();    //���,��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();
		FuncFParam();
	}
	total[Funcindex][0].setparaLength(paraLength);
	//�˳�ǰ�Ѿ�Ԥ��
	outfile << "<�����βα�>" << endl;
}
void FuncFParam()
{
	vector<int> length; //��¼ÿ��ά�ȴ�С
	printMessage();    //���int��Ϣ
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	string name = token;   //���������
	printMessage();    //�����������Ϣ
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	int dimenson = 0;
	if (symbol == LBRACK) {
		dimenson++;
		printMessage();    //���[��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();
		printMessage();    //���]��Ϣ
		length.push_back(0);
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();
		while (symbol == LBRACK) {
			dimenson++;
			printMessage();    //���[��Ϣ
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//Ԥ��
			length.push_back(ConstExp());
			printMessage();    //���]��Ϣ
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();  //Ԥ��
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
	CodeItem citem = CodeItem(PARA,"%"+name,b,numToString(dimenson));//�����β�
	citem.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem);
	outfile << "<�����β�>" << endl;
}
void Exp()	//MulExp{('+'|'-')MulExp}
{
	Memory symbol_tag;
	string registerL, registerR;
	MulExp();  //�˳�ǰ����һ������
	registerL = interRegister;
	while (symbol == PLUS || symbol == MINU) {
		symbol_tag = symbol;
		printMessage();    //���+��-��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
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
	//�˳�ǰ�Ѿ�Ԥ��
	outfile << "<���ʽ>" << endl;
	return ;
}
void MulExp()	//UnaryExp {('*'|'/'|'%') UnaryExp  }
{
	UnaryExp();
	string registerL = interRegister;	//��ȡ�������
	while (symbol == MULT || symbol == DIV1 || symbol == MOD) {
		Memory symbol_tag = symbol;
		printMessage();    //���*��/��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		UnaryExp();
		string registerR = interRegister;	//��ȡ�Ҽ��������Ӵ˿�ʼ
		if (registerL[0] != '@' && registerL[0] != '%' && registerR[0] != '@' && registerR[0] != '%') {
			int value;
			int valueL = stringToNum(registerL);
			int valueR = stringToNum(registerR);
			if (symbol_tag == MULT) value = valueL * valueR;
			else if (symbol_tag == DIV) value = valueL / valueR;
			else value = valueL % valueR;
			interRegister = numToString(value);
		}
		else {
			interRegister = "%" + numToString(Temp);
			Temp++;
			irCodeType codetype;
			if (symbol_tag == MULT) codetype = MUL;
			else if (symbol_tag == DIV) codetype = DIV;
			else codetype = REM;
			CodeItem citem = CodeItem(codetype, interRegister, registerL, registerR);
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
		registerL = interRegister;
	}
	//�˳�ǰ�Ѿ�Ԥ��
	outfile << "<��>" << endl;
}
void UnaryExp()			// '(' Exp ')' | LVal | Number | Ident '(' [FuncRParams] ')' | +|-|! UnaryExp 
{
	if (symbol == LPARENT) {   //'(' Exp ')'
		printMessage();    //���(��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		Exp();
		printMessage();    //���)��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
	}
	else if (symbol == INTCON){  //number
		printMessage();
		interRegister = numToString(wordAnalysis.getNumber());		//����ֵת�����ַ���ģʽ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
	}
	else if (symbol == IDENFR) {  //��ʶ�� (����ʵ�α�) | ��ʶ�� {'['���ʽ']'}
		printMessage();    //���������Ϣ
		string name_tag = wordAnalysis.getToken();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		string registerL,registerR,registerA,registerC;
		int typeL,typeR,typeA,typeC;
		if (symbol == LPARENT) {  //��ʶ�� (����ʵ�α�)
			paraNum = 0;
			Functionname = name_tag;
			printMessage();    //���(��Ϣ
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//Ԥ��
			if (symbol != RPARENT) {
				FuncRParams();
			}
			printMessage();    //���)��Ϣ
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//Ԥ��
			interRegister = "%" + numToString(Temp);		//�溯�����ؽ��
			Temp++;
			CodeItem citem = CodeItem(CALL,"@"+Functionname,interRegister,numToString(paraNum));          //call @foo %3 3
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);//��������
		}
		else {  //��ʶ�� {'['���ʽ']'}
			symbolTable item = checkItem(name_tag);
			vector<int> dimenLength = item.getMatrixLength();
			if (dimenLength.size() > 0 && symbol == LBRACK) {
				registerA = "0";	//ƫ����Ϊ0
			}
			int nowDimenson = 0;   //��¼��ǰά��
			while (symbol == LBRACK) {
				nowDimenson++;
				printMessage();    //���[��Ϣ
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//Ԥ��
				Exp();		//q[n][m]
				registerL = interRegister;   //registerL = "n"	����ά��ƫ����
				int offset = 1;
				for (int j = nowDimenson; j < dimenLength.size(); j++) {
					offset = offset * dimenLength[j];
				}
				registerR = numToString(offset);		//��ά��ֵΪ1ʱ��С
				if (registerL[0] != '@' && registerL[0] != '%') {   //��ά���ܴ�С������registerC��
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
				if (registerC[0] != '@' && registerC[0] != '%' && registerA[0] != '@' && registerA[0] != '%') {	//��ǰƫ����registerA + ��ά����ƫ����registerC
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
				printMessage();    //���]��Ϣ
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//Ԥ��
			}
			if (item.getDimension() == nowDimenson && nowDimenson > 0) {	//ȡ��ά�����һ��Ԫ��
				interRegister = "%" + numToString(Temp);
				Temp++;
				string b = "%";
				if (Range == 0) {
					b = "@";	//ȫ�ֱ���
				}
				CodeItem citem = CodeItem(LOAD, interRegister,b+name_tag,registerA); //����ȡֵ
				citem.setFatherBlock(fatherBlock);    //���浱ǰ������
				codetotal[Funcindex].push_back(citem); 
			}
			else if (nowDimenson > 0 && nowDimenson < item.getDimension()) {		//��ʱֻ�����Ǵ��Σ��������һ����
				paraIntNode = 1;
				string b = "%";
				if (Range == 0) {
					b = "@";	//ȫ�ֱ���
				}
				interRegister = b+name_tag;
			}
			else {
				if (item.getForm() == CONSTANT) {
					interRegister = numToString(item.getIntValue()[0]);		//����ֵת�����ַ���ģʽ
				}
				else {
					interRegister = "%" + numToString(Temp);
					Temp++;
					string b = "%";
					if (Range == 0) {
						b = "@";	//ȫ�ֱ���
					}
					CodeItem citem = CodeItem(LOAD, interRegister, b+name_tag, "0"); //һά����������ȡֵ
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
			printMessage();    //���+��Ϣ
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//Ԥ��
			UnaryExp();
		}
		else if (symbol == MINU) {
			printMessage();    //���-��Ϣ
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//Ԥ��
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
			printMessage();    //���!��Ϣ
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();//Ԥ��
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
	//�˳�ǰ�Ѿ�Ԥ��
	outfile << "<����>" << endl;
}
void Stmt()              //���
{
	if (symbol == IFTK) {  //��������䣾
		ifStmt();
	}
	else if (symbol == WHILETK) {  //<ѭ�����>
		loopStmt();
	}
	else if (symbol == BREAKTK) {	//break ;
		printMessage();    //���break��Ϣ
		CodeItem citem = CodeItem(BR, whileLabel[whileLabel.size() - 2], "", ""); //br %while.cond 
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		printMessage();    //���;��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
	}
	else if (symbol == CONTINUETK) {	//continue ;
		printMessage();    //���continue��Ϣ
		CodeItem citem = CodeItem(BR, whileLabel[whileLabel.size() - 1], "", ""); //br %while.cond 
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		printMessage();    //���;��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
	}
	else if (symbol == LBRACE) { //'{'������|����'}'
		Block();
	}
	else if (symbol == RETURNTK) { //��������䣾;
		returnStmt();
		printMessage();    //���;��Ϣ
		//�ж�symbol=;
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
	}
	else if (symbol == SEMICN) {
		printMessage();    //���;��Ϣ
		//�ж�symbol=;
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
	}
	else if (symbol == IDENFR) {
		Memory sym_tag = symbol;
		string token_tag = wordAnalysis.getToken();
		int record_tag = wordAnalysis.getfRecord();
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();  //Ԥ��
		if (symbol == LPARENT) {	//�����Ϊ�����޷���ֵ������
			wordAnalysis.setfRecord(record_tag);
			wordAnalysis.setSymbol(sym_tag);
			wordAnalysis.setToken(token_tag);
			symbol = sym_tag;
			Exp();					//CALL������EXP�д���
			printMessage();  //����ֺ�
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
			token = wordAnalysis.getToken();  //Ԥ��
		}
		else {
			while (symbol != SEMICN and symbol != ASSIGN) {
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
			}
			//�ָ���������
			wordAnalysis.setfRecord(record_tag);
			wordAnalysis.setSymbol(sym_tag);
			wordAnalysis.setToken(token_tag);
			if (symbol == ASSIGN) {  	//LVal = Exp; 
				symbol = sym_tag;
				assignStmt();
				printMessage();    //���;��Ϣ
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//Ԥ��
			}
			else {		//Exp ;  ����ֱ������
				while (symbol != SEMICN) {
					wordAnalysis.getsym();
					symbol = wordAnalysis.getSymbol();
				}
				printMessage();    //���;��Ϣ
				wordAnalysis.getsym();
				symbol = wordAnalysis.getSymbol();
				token = wordAnalysis.getToken();//Ԥ��
			}
		}
	}
	else {						//Exp ; ����ֱ��������ûӰ��
		while (symbol != SEMICN) {
			wordAnalysis.getsym();
			symbol = wordAnalysis.getSymbol();
		}
		printMessage();    //���;��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
	}
	//�˳�ǰ��Ԥ��
	outfile << "<���>" << endl;
}
void assignStmt()        //��ֵ��� LVal = Exp
{
	printMessage();    //���������Ϣ
	string name_tag = wordAnalysis.getToken();		//������
	string registerL, registerR, registerA, registerC;
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//Ԥ��
	symbolTable item = checkItem(name_tag);
	vector<int> dimenLength = item.getMatrixLength();
	string b = "%";
	if (Range == 0) {
		b = "@";	//ȫ�ֱ���
	}
	if (dimenLength.size() > 0) {
		registerA = "0";
	}
	int nowDimenson = 0;   //��¼��ǰά��
	while (symbol == LBRACK) {
		nowDimenson++;
		printMessage();    //���[��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		Exp();
		registerL = interRegister;		//��ά��ƫ����
		int offset = 1;
		for (int j = nowDimenson; j < dimenLength.size(); j++) {
			offset = offset * dimenLength[j];
		}
		registerR = numToString(offset);		//��ά��ֵΪ1ʱ��С
		if (registerL[0] != '@' && registerL[0] != '%') {   //��ά���ܴ�С������registerC��
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
		if (registerC[0] != '@' && registerC[0] != '%' && registerA[0] != '@' && registerA[0] != '%') {	//��ǰƫ����registerA + ��ά����ƫ����registerC
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
		printMessage();    //���]��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
	}
	printMessage();    //���=��Ϣ
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//Ԥ��
	Exp();
	if (dimenLength.size() > 0) {
		CodeItem citem = CodeItem(STORE, interRegister, b + name_tag, registerA);		//����ĳ��������ֵ
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
	}
	else {
		CodeItem citem = CodeItem(STORE, interRegister, b+name_tag, "0");		//��һά������ֵ
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
	}
	outfile << "<��ֵ���>" << endl;
}
void ifStmt()            //�������
{
	printMessage();    //���if��Ϣ
	//�ж�symbol=if
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	printMessage();   //���(�����
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken(); //Ԥ��
	Cond();
	printMessage();    //���)��Ϣ
	//�ж�symbol=)
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//Ԥ��
	string if_then_label = "%if.then_" + numToString(iflabelIndex);
	string if_else_label = "%if.else_" + numToString(iflabelIndex);
	string if_end_label = "%if.end_" + numToString(iflabelIndex);
	iflabelIndex++;
	CodeItem citem = CodeItem(BR,interRegister,if_then_label,if_else_label); //br ���� %if.then_1 %if.else_1 
	citem.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem);
	CodeItem citem2 = CodeItem(LABEL,if_then_label,"","");	//label if.then
	citem2.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem2);
	Stmt();													//stmt1
	CodeItem citem3 = CodeItem(BR, if_end_label, "", "");   //BR if.end
	citem3.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem3);
	CodeItem citem4 = CodeItem(LABEL, if_else_label, "", "");	//label if.else
	citem4.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem4);
	if (symbol == ELSETK) {
		printMessage();    //���else��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		Stmt();			//stmt2
	}
	CodeItem citem5 = CodeItem(BR, if_end_label, "", "");   //BR if.end
	citem5.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem5);
	CodeItem citem6 = CodeItem(LABEL, if_end_label, "", "");	//label if.else
	citem6.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem6);
	//�˳�ǰStmt��Ԥ��
	outfile << "<�������>" << endl;
}
void Cond()              //�������ʽ(�߼�����ʽ)  LAndExp { '||' LAndExp}
{
	string registerL, registerR,op;
	LAndExp();
	registerL = interRegister;
	while (symbol == OR1) {
		Memory symbol_tag = symbol;
		printMessage();    //����߼������
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
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
	outfile << "<����>" << endl;
}
void LAndExp()			  //�߼�����ʽ   EqExp{'&&' EqExp }
{
	string registerL, registerR, op;
	EqExp();
	registerL = interRegister;
	while (symbol == AND1) {
		Memory symbol_tag = symbol;
		printMessage();    //����߼������
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
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

void EqExp()		  //����Ա��ʽ
{
	string registerL, registerR, op;
	RelExp();
	registerL = interRegister;
	while (symbol == EQL1 || symbol == NEQ1) {
		Memory symbol_tag = symbol;
		printMessage();    //����߼������
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		RelExp();
		registerR = interRegister;
		if (registerL[0] != '@' && registerL[0] != '%' && registerR[0] != '@' && registerR[0] != '%') {
			int value;
			int valueL = stringToNum(registerL);
			int valueR = stringToNum(registerR);
			if (symbol_tag == EQL) {
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
			if (symbol_tag == EQL) codetype = EQL;
			else codetype = NEQ;
			CodeItem citem = CodeItem(codetype, interRegister, registerL, registerR);
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
		registerL = interRegister;
	}
}
void RelExp()			  //��ϵ���ʽ
{
	string registerL, registerR, op;
	Exp();
	registerL = interRegister;
	while (symbol == LSS || symbol == LEQ||symbol==GRE||symbol==GEQ){
		Memory symbol_tag = symbol;
		printMessage();    //����߼������
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
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
void loopStmt()          //ѭ�����
{
	//?while '('��������')'����䣾
	printMessage();    //���while��Ϣ
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();
	printMessage();   //���(�������Ϣ
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//Ԥ��
	string while_cond_label = "%while.cond_" + numToString(whilelabelIndex);
	string while_body_label = "%while.body_" + numToString(whilelabelIndex);
	string while_end_label = "%while.end_" + numToString(whilelabelIndex);
	whilelabelIndex++;
	CodeItem citem1 = CodeItem(LABEL, while_cond_label, "", "");	//label %while.cond
	citem1.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem1);
	Cond();						//cond
	printMessage();    //���)��Ϣ
	CodeItem citem2 = CodeItem(BR, interRegister, while_body_label, while_end_label); //br ���� %while.body %while.end  
	citem2.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem2);
	CodeItem citem3 = CodeItem(LABEL, while_body_label, "", "");	//label %while.body
	citem3.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem3);
	whileLabel.push_back(while_end_label);		
	whileLabel.push_back(while_cond_label);
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//Ԥ��
	Stmt();
	whileLabel.pop_back();
	whileLabel.pop_back();
	CodeItem citem4 = CodeItem(BR, while_cond_label,"",""); //br %while.cond 
	citem4.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem4);
	CodeItem citem5 = CodeItem(LABEL, while_end_label, "", "");	//label %while.end
	citem5.setFatherBlock(fatherBlock);
	codetotal[Funcindex].push_back(citem5);
	//�˳�ǰ��Ԥ��
	outfile << "<ѭ�����>" << endl;
}
void FuncRParams()    //����ʵ������
{
	if (symbol == RPARENT) {
		return;
	}
	Exp();//�˳�ǰ�Ѿ�Ԥ��
	paraNum++;
	if (paraIntNode == 0) {
		CodeItem citem = CodeItem(PUSH,interRegister,"","int");  //����
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
	}
	else {
		CodeItem citem = CodeItem(PUSH, interRegister, "", "int*");  //����
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
		paraIntNode = 0;
	}
	while (symbol == COMMA) {
		printMessage();    //���,��Ϣ
		wordAnalysis.getsym();
		symbol = wordAnalysis.getSymbol();
		token = wordAnalysis.getToken();//Ԥ��
		Exp();
		paraNum++;
		if (paraIntNode == 0) {
			CodeItem citem = CodeItem(PUSH, interRegister, "", "int");  //����
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
		}
		else {
			CodeItem citem = CodeItem(PUSH, interRegister, "", "int*");  //����
			citem.setFatherBlock(fatherBlock);
			codetotal[Funcindex].push_back(citem);
			paraIntNode = 0;
		}
	}
	//�˳�ѭ��ǰ�Ѿ�Ԥ��
	outfile << "<����ʵ������>" << endl;
}
void returnStmt()        //�������
{
	printMessage();    //���return��Ϣ
	//�ж�symbol=return
	wordAnalysis.getsym();
	symbol = wordAnalysis.getSymbol();
	token = wordAnalysis.getToken();//Ԥ��
	if (symbol != SEMICN) {
		Exp();
		CodeItem citem = CodeItem(RET, interRegister, "","");  //�з���ֵ����
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
	}
	else {
		CodeItem citem = CodeItem(RET,"","","");
		citem.setFatherBlock(fatherBlock);
		codetotal[Funcindex].push_back(citem);
	}
	outfile << "<�������>" << endl;
}
symbolTable checkTable(string checkname, int function_number, vector<int> fatherBlock)  //function_numberΪ�����±�,fatherBlockΪ��������������             
{
	int i, j, checkblock, block;
	string name;
	for (j = fatherBlock.size() - 1; j >= 0; j--) {
		checkblock = fatherBlock[j];
		for (i = 0; i < total[function_number].size(); i++) {   //�ȴӱ�������������
			name = total[function_number][i].getName();
			block = total[function_number][i].getblockIndex();
			if (name == checkname && block == checkblock) {  //��������������ҵ���
				return total[function_number][i];
			}
		}
	}
	for (i = 0; i < total[0].size(); i++) {
		name = total[0][i].getName();
		if (name == checkname) {  //��������������ҵ���
			return total[0][i];
		}
	}
}
void change(int index)	//�޸��м���롢���ű�
{
	int i, j, k;
	//���޸��м����
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
		if (res.size()>0 && res[0] == '%' && (!isdigit(res[1])) ) {  //res�����Ǳ���
			if (names[index][res] > 1) {
				res.erase(0, 1);  //ȥ������ĸ
				symbolTable item = checkTable(res, index, b[i].getFatherBlock());
				res = "%" + newName(res, item.getblockIndex());
			}
		}
		if (ope1.size() > 0 && ope1[0] == '%' && (!isdigit(ope1[1]))) {  //res�����Ǳ���
			if (names[index][ope1] > 1) {
				ope1.erase(0, 1);  //ȥ������ĸ
				symbolTable item = checkTable(ope1, index, b[i].getFatherBlock());
				ope1 = "%" + newName(ope1, item.getblockIndex());
			}
		}
		if (ope2.size() > 0 && ope2[0] == '%' && (!isdigit(ope2[1]))) {  //res�����Ǳ���
			if (names[index][ope2] > 1) {
				ope2.erase(0, 1);  //ȥ������ĸ
				symbolTable item = checkTable(ope2, index, b[i].getFatherBlock());
				ope2 = "%" + newName(ope2, item.getblockIndex());
			}
		}
		b[i].changeContent(res, ope1, ope2);
	}
	codetotal.push_back(b);

	//���޸ķ��ű�
	vector<symbolTable> a = total[index];
	total.pop_back();
	for (i = 0; i < a.size(); i++) {
		if (a[i].getForm() == FUNCTION) {
			continue;
		}
		else {
			string name = a[i].getName();
			if (names[index][name] > 1) {       //ĳһ�����ں����������ڳ��ִ�������1����Ҫ����
				string newname = newName(name, a[i].getblockIndex());
				a[i].changeName(newname);
			}
		}
	}
	total.push_back(a);
}