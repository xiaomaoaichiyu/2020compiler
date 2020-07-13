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
	int stackOffset = 0;
	map<string, int> vreg2Offset;			//��¼��spiil����ʱ������ջ�е�ƫ��
public:
	RegPool(vector<string> regs) : pool(regs) {
		for (int i = 0; i < regs.size(); i++) {
			reg2avail[regs.at(i)] = true;
		}
	}
	string getReg(string vreg);
	string getAndAllocReg(string vreg);
	void releaseReg(string vreg);
	CodeItem loadVreg(string vreg);
private:
	string haveAvailReg();
	string allocReg();
	pair<string, string> spillReg();	//���� <�Ĵ���, ����Ĵ���> 
};

struct Allocation {
	int from;		//ָ��id��ʼ
	int to;			//ָ��id����
	string reg;		//���������Ĵ��������ڴ�λ��
};

void registerAllocation(vector<CodeItem>& func);

//=============================================
//2. ����ɨ���㷨
//=============================================

//1. ���������Ի�
//class BlockOrder {
//	//loop dectectionѭ�����
//	map<int, basicBlock> blocks;
//	map<int, int> blk2index;
//	map<int, int> blk2depth;
//
//	vector<pair<int, int>> loop_end_lists;			//pair<int, int> : loop_end -> loop_header
//	int index = 0;
//	vector<pair<int, int>> loop_header_index;		//loop_header index
//	vector<int> orders;	//����������Ի�˳��
//
//public:
//	BlockOrder(vector<basicBlock> blks);
//	void loop_detection(int nblks);
//	void sortInstr();
//};
//
////�����ķ�Χ
//struct Range {
//	int from;
//	int to;
//	Range(int _from, int _to) : from(_from), to(_to) {}
//};
//
////����ʹ�õ�λ��
//struct UsePosition {
//	int position;
//	int use_kind; //1: regster, 2: memory
//	UsePosition(int pos, int kind) : position(pos), use_kind(kind) {}
//};
//
////��������������
//class Interval {
//	string var;
//	string reg;
//	vector<Range> ranges;
//	vector<UsePosition> positions;
//	Interval* split_parent = nullptr;
//	vector<Interval> split_children;
//	Interval* register_hint = nullptr;
//public:
//	Interval();
//	Interval(string var);
//	~Interval();
//
//	void add_range(int from, int to);
//	void add_usePosition(int pos, int use_kind);
//	Interval split(int op_id);
//};
//
//
//
//class RegsiterAllocation {
//
//public:
//
//};


#endif // _REGISTER_H_
