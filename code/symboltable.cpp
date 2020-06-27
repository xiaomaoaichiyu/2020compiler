#include<string>
#include<vector>
#include<iostream>
#include "symboltable.h"
symbolTable::symbolTable(formType a, valueType b, string c)
{
	this->form = a;
	this->valuetype = b;
	this->name = c;
}
symbolTable::symbolTable()
{

}
symbolTable::symbolTable(formType a, valueType b, string c,int d,int e)
{
	this->form = a;
	this->valuetype = b;
	this->name = c;
	this->dimension = d;
	this->blockIndex = e;
}
formType symbolTable::getForm()
{
	return this->form;
}
string symbolTable::getName()
{
	return this->name;
}
int symbolTable::getblockIndex()
{
	return this->blockIndex;
}
void symbolTable::setMatrixLength(vector<int> a)
{
	this->matrixLength = a;
	if (this->dimension > 0) {
		int i = 1;
		for (int j = 0; j < a.size(); j++) {
			i = i * a[j];
		}
		vector<int> b(i,0);  //初始化b大小为i而且每个元素值均为0
		this->intValue = b;
	}
}
vector<int> symbolTable::getMatrixLength()
{
	return this->matrixLength;
}
int symbolTable::getDimension()
{
	return this->dimension;
}
int symbolTable::getparaLength()
{
	return this->paraLength;
}
void symbolTable::setparaLength(int a)
{
	this->paraLength = a;
}
void symbolTable::setIntValue(int a,int offset)
{
	if (this->intValue.size() == 0) {
		this->intValue.push_back(a);
	}
	else {
		this->intValue[offset] = a;
	}
}
vector<int> symbolTable::getIntValue()
{
	return this->intValue;
}
int symbolTable::getvarLength()
{
	if (this->dimension == 0) {
		return 1;
	}
	else {
		int varLength = 1;
		for (int i = 0; i < this->matrixLength.size(); i++) {
			varLength = varLength * this->matrixLength[i];
		}
		return varLength;
	}
}
void symbolTable::setfirstNodesize(int a)
{
	this->matrixLength[0] = a;
}
void symbolTable::changeName(string newname)
{
	this->name = newname;
}