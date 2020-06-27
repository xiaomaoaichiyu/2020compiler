#include<string>
#include<vector>
using namespace std;
#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H
enum formType {
	CONSTANT, VARIABLE, PARAMETER, FUNCTION
};  //常量、变量、参数、函数
enum valueType {  //ALL代表所有类型，如果是ALL就不需要再检查returnvaluetype了，ALL一般是调用未定义
	INT,VOID
};
class symbolTable
{
public:
	symbolTable();
	symbolTable(formType a, valueType b, string c);  //定义函数
	symbolTable(formType a, valueType b, string c, int d, int e);     //变量、常量定义建表，c是name，d是维度，e是blockIndex，f是fatherblock
	formType getForm();
	string getName();
	int getparaLength();
	void setparaLength(int a);
	int getblockIndex();
	int getDimension();
	void setMatrixLength(vector<int> a);
	vector<int> getMatrixLength();
	void setIntValue(int a,int offset);
	void setfirstNodesize(int a);
	void changeName(string newname);			//修改变量名
	vector<int> getIntValue();
	int getvarLength();          //获得变量总元素个数
private:
	formType form;  //符号表形式种类
	valueType valuetype;  //符号表值种类，除了函数可能有VOID剩下都是INT
	int paraLength; //对函数而言：参数个数
	int dimension;  //数组维度
	vector<int> matrixLength;  //数组每个维度大小,matrixLength[i]表示第i维长度
	vector<int> intValue; //变量、常量具体值(无论几维都这么存)
	string name;    //名字
	//string range;   作用域不要了，因为使用二级vector，第二层vector代表一个作用域
	int blockIndex;    //当前block号
};


#endif 

