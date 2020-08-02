#ifndef _REGISTER_H_
#define _REGISTER_H_

#include "../util/meow.h"
#include "../ir/ssa.h"

#include <vector>
#include <string>
#include <map>
#include <queue>
#include <set>
using namespace std;

//=================================================================
//1. �Ĵ���ֱ��ָ�ɣ���ʱ�Ĵ���ʹ�üĴ�����
//=================================================================

class RegPool {
	vector<string> pool;
	map<string, bool> reg2avail;			//��¼�Ĵ����Ƿ�ʹ�ã�true��ʾ���ã�false��ʾ��ռ��
	map<string, string> vreg2reg;			//��¼���������Ĵ���������Ĵ���

	map<string, bool> vreg2spill;			//��¼�Ƿ����
	vector<string> vreguse;					//��¼����Ĵ��������ʱ�䣬������ռ����Ĵ������˳�����
public:
	RegPool(vector<string> regs) : pool(regs) {
		for (int i = 0; i < regs.size(); i++) {
			reg2avail[regs.at(i)] = true;
		}
	}
	string getReg(string vreg);
	void markParaReg(string reg, string Vreg);
	string getAndAllocReg(string vreg);
	void releaseVreg(string vreg);
	pair<string, string> spillReg();				//���� <�Ĵ���, ����Ĵ���> 
	vector<pair<string, string>> spillAllRegs();
private:
	string haveAvailReg();
	string allocReg();
};

class GlobalRegPool {
	vector<string> pool;			//ȫ�ּĴ�����
	map<string, bool> reg2avail;	//��¼�Ĵ����Ƿ��ܹ�ʹ��
	map<string, string> var2reg;	//��¼������Ӧ�ļĴ���

	map<string, bool> used;		//������¼ȫ�ּĴ����Ƿ�ʹ�ù�
public:
	GlobalRegPool(vector<string> regs) : pool(regs) {
		for (int i = 0; i < regs.size(); i++) {
			reg2avail[regs.at(i)] = true;
			used[regs.at(i)] = false;
		}
	}
	string getReg(string var);		//��ȡһ�������ļĴ��������û�з���"memory"
	string allocReg(string var);	//��һ����������ȫ�ּĴ��������û��ȫ�ּĴ������÷���"memory"
	void releaseReg(string var);	//�ͷ����������ȫ�ּĴ���
	void releaseNorActRegs(set<string> inVars, set<string> outVars);
	vector<int> getUsedRegs();
	void noteRegRelations(vector<CodeItem>& func);
};


void registerAllocation();

void registerAllocation2(vector<vector<basicBlock>>& lir);

#endif // _REGISTER_H_
