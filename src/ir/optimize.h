#ifndef _OPTIMIZE_H_
#define _OPTIMIZE_H_

#include "../util/meow.h"

#include <vector>
#include <string>

using namespace std;

class RegsiterAllocation {

public:
	
};

//变量的生存周期
class Interval {
	string var;
	string reg;
	vector<Range> ranges;
	vector<UsePosition> positions;
	Interval* split_parent = nullptr;
	vector<Interval> split_children;
	Interval* register_hint = nullptr;
public:
	Interval();
	Interval(string var);
	~Interval();
	
	void add_range(int from, int to);
	void add_usePosition(int pos, int use_kind);
	Interval split(int op_id);
};

//变量的范围
struct Range {
	int from;
	int to;
	Range(int _from, int _to) : from(_from), to(_to) {}
};

//变量使用的位置
struct UsePosition {
	int position;
	int use_kind; //1: regster, 2: memory
	UsePosition(int pos, int kind) : position(pos), use_kind(kind) {}
};
#endif // !_OPTIMIZE_H_
