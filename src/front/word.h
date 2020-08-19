#include <string>
using namespace std;
#ifndef _WORD_H
#define _WORD_H
#define keyword 10
enum Memory {
	CONSTTK, INTTK, VOIDTK, MAINTK, IFTK, ELSETK,
	WHILETK, BREAKTK, CONTINUETK, RETURNTK,  
	IDENFR, INTCON, PLUS, MINU, MULT, DIV_WORD,MOD,AND_WORD,OR_WORD,
	LSS, LEQ, GRE, GEQ, EQL_WORD, NEQ_WORD,REV, ASSIGN, SEMICN, COMMA,
	LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE, STRING,FINISH   
};
const string remain[keyword] = {
	"const","int","void","main","if","else","while",
	"break","continue","return"
};
class Word
{
public:
	Word();
	bool getsym();
	void setSymbol(Memory symbol);
	Memory getSymbol();
	int getNumber();
	string getToken();
	void setFilepath(string filename);
	void setToken(string Token);
	void setfRecord(int record);
	int getlineNumber();
	int getfRecord();
private:
	enum Memory symbol;
	char mark;   //当前读的字符
	string file;  //外界文件信息
	int fRecord;   //文件下标
	string token;  //当前读的单词
	int number;   
	int lineNumber; //行号
	void getmark();
	void clearToken();
	void catToken();  //拼接 
	void retract();   //回退
	void transNum10(); 
	void transNum16();
	void transNum8();
	//用自带atoi函数代替也可以 
};

//别的文件不会引用的函数设置为private类型的函数




#endif // !_WORD_H

