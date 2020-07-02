#ifndef _INTERMEDIATECODE_H
#define _INTERMEDIATECODE_H

#include <string>
#include "symboltable.h"

using namespace std;

//�м��������SSA��ʽ v1
enum irCodeType {
	ADD, SUB, DIV, MUL, REM,  		//��������
	AND, OR, NOT,					//�߼�����
	EQL, NEQ, SGT, SGE, SLT, SLE,	//��ϵ����
	ALLOC,							//ջ�ռ�����
	STORE,							//��ֵ���ڴ�
	LOAD,							//���ڴ�ȡֵ
	INDEX,							//���������
	CALL,							//��������
	RET,							//��������
	PUSH,							//ѹջ
	POP,							//��ջ
	LABEL,							//��ǩ
	BR,								//ֱ����ת + ������ת
	DEFINE,							//��������
	PARA,							//��������
	GLOBAL							//ȫ������������+����
};

class CodeItem
{
public:
	CodeItem(irCodeType type, string res, string ope1, string ope2);
	irCodeType getCodetype();
	string getOperand1();
	string getOperand2();
	string getResult();
	void setOperand1(string ope1);
	void setOperand2(string ope2);
	void setResult(string res);
	void setFatherBlock(vector<int> a);        //����set��λ�ã����б������������ֵĵط�
	void changeContent(string res, string ope1, string ope2);
	vector<int> getFatherBlock();
	string getContent();
private:   
	irCodeType codetype;     //�м��������
	string result;				//���
	string operand1;			//�������
	string operand2;			//�Ҳ�����
	vector<int> fatherBlock;	//��ǰ�м��������������
	string content;				//debug��ʾ���ݣ����м��������
};

#endif 
