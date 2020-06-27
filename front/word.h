#include <string>
using namespace std;
#ifndef _WORD_H
#define _WORD_H
#define keyword 10
enum Memory {
	CONSTTK, INTTK, VOIDTK, MAINTK, IFTK, ELSETK,
	WHILETK, BREAKTK, CONTINUETK, RETURNTK,
	IDENFR, INTCON, PLUS, MINU, MULT, DIV1,MOD,AND1,OR1,
	LSS, LEQ, GRE, GEQ, EQL1, NEQ1,REV, ASSIGN, SEMICN, COMMA,
	LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE, FINISH
};
const string remain[keyword] = {
	"const","int","void","main","if","else","while",
	"break","continue","return"
};
class Word
{
public:
	Word(string filename);
	bool getsym();
	void setSymbol(Memory symbol);
	Memory getSymbol();
	int getNumber();
	string getToken();
	void setToken(string Token);
	void setfRecord(int record);
	int getfRecord();
private:
	enum Memory symbol;
	char mark;   //��ǰ�����ַ�
	string file;  //����ļ���Ϣ
	int fRecord;   //�ļ��±�
	string token;  //��ǰ���ĵ���
	int number;   
	void getmark();
	void clearToken();
	void catToken();  //ƴ�� 
	void retract();   //����
	void transNum10(); 
	void transNum16();
	void transNum8();
	//���Դ�atoi��������Ҳ���� 
};

//����ļ��������õĺ�������Ϊprivate���͵ĺ���




#endif // !_WORD_H

