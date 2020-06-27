#include<string>
#include<vector>
using namespace std;
#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H
enum formType {
	CONSTANT, VARIABLE, PARAMETER, FUNCTION
};  //����������������������
enum valueType {  //ALL�����������ͣ������ALL�Ͳ���Ҫ�ټ��returnvaluetype�ˣ�ALLһ���ǵ���δ����
	INT,VOID
};
class symbolTable
{
public:
	symbolTable();
	symbolTable(formType a, valueType b, string c);  //���庯��
	symbolTable(formType a, valueType b, string c, int d, int e);     //�������������彨��c��name��d��ά�ȣ�e��blockIndex��f��fatherblock
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
	void changeName(string newname);			//�޸ı�����
	vector<int> getIntValue();
	int getvarLength();          //��ñ�����Ԫ�ظ���
private:
	formType form;  //���ű���ʽ����
	valueType valuetype;  //���ű�ֵ���࣬���˺���������VOIDʣ�¶���INT
	int paraLength; //�Ժ������ԣ���������
	int dimension;  //����ά��
	vector<int> matrixLength;  //����ÿ��ά�ȴ�С,matrixLength[i]��ʾ��iά����
	vector<int> intValue; //��������������ֵ(���ۼ�ά����ô��)
	string name;    //����
	//string range;   ������Ҫ�ˣ���Ϊʹ�ö���vector���ڶ���vector����һ��������
	int blockIndex;    //��ǰblock��
};


#endif 

