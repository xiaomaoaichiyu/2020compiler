#include "../ir/dataAnalyse.h"

//=================================================================
//到达定义数据流分析
//=================================================================

void UDchain::init() {
	int size = CFG.size();
	for (int i = 0; i < size; i++) {
		in.push_back(set<Node>());
		out.push_back(set<Node>());
		gen.push_back(set<Node>());
	}
}

//根据CFG计算每个基本块的gen集合、kill集合
void UDchain::count_gen() {
	for (int i = 0; i < CFG.size(); i++) {
		auto blkIr = CFG.at(i).Ir;
		for (int j = 0; j < blkIr.size(); j++) {
			auto instr = blkIr.at(j);
			auto res = instr.getResult();
			auto ope1 = instr.getOperand1();
			auto ope2 = instr.getOperand2();
			switch (instr.getCodetype())
			{
			case ADD: case SUB: case MUL: case DIV: case REM:
			case AND: case OR: case NOT: 
			case EQL: case NEQ: case SLT: case SLE: case SGT: case SGE: 
			case LOAD: case LOADARR:
			{
				Node def1(i, j, res);
				gen.at(i).insert(def1);
				def2var[def1] = res;
				if (var2defs.find(res) != var2defs.end()) {
					var2defs[res].insert(def1);
				}
				else {
					var2defs[res] = set<Node>();
					var2defs[res].insert(def1);
				}
				duchains[pair<string, Node>(res, def1)] = Node();
				break;
			} case STORE: {
				if (!isGlobal(ope1)) {
					Node def1(i, j, ope1);
					gen.at(i).insert(def1);
					def2var[def1] = ope1;
					if (var2defs.find(ope1) != var2defs.end()) {
						var2defs[ope1].insert(def1);
					}
					else {
						var2defs[ope1] = set<Node>();
						var2defs[ope1].insert(def1);
					}
				}
				break;
			} case CALL: {
				if (isTmp(res)) {
					Node def1(i, j, res);
					gen.at(i).insert(def1);
					def2var[def1] = res;
					if (var2defs.find(res) != var2defs.end()) {
						var2defs[res].insert(def1);
					}
					else {
						var2defs[res] = set<Node>();
						var2defs[res].insert(def1);
					}
					duchains[pair<string, Node>(res, def1)] = Node();
				}
				break;
			} case PHI: {
				if (!isGlobal(res)) {
					Node def1(i, j, res);
					gen.at(i).insert(def1);
					def2var[def1] = res;
					if (var2defs.find(res) != var2defs.end()) {
						var2defs[res].insert(def1);
					}
					else {
						var2defs[res] = set<Node>();
						var2defs[res].insert(def1);
					}
					//duchains[pair<string, Node>(res, def1)] = Node();
				}
				break;
			} case PARA: {
				if (ope2 == "0") {	//单值变量
					Node def1(i, j, res);
					gen.at(i).insert(def1);
					def2var[def1] = res;
					if (var2defs.find(res) != var2defs.end()) {
						var2defs[res].insert(def1);
					}
					else {
						var2defs[res] = set<Node>();
						var2defs[res].insert(def1);
					}
					//duchains[pair<string, Node>(res, def1)] = Node();
				}
				break;
			}
			default:
				break;
			}
		}
	}
}

//合并b集合的变量
set<Node> merge(set<Node> & a, set<Node> & b) {
	a.insert(b.begin(), b.end());
	return a;
}

bool UDchain::find_var_def(set<Node> container, string var) {
	for (auto def : container) {
		try {
			if (def2var[def] == var) {
				return true;
			}
		}
		catch (exception e) {
			WARN_MSG("???");
		}
	}
	return false;
}

void UDchain::erase_defs(set<Node>& container, string var) {
	try {
		for (auto it = container.begin(); it != container.end();) {
			if (def2var[*it] == var) {
				it = container.erase(it);
				continue;
			}
			it++;
		}
	}
	catch (exception e) {
		WARN_MSG("wow!! wrong!");
	}
}


void UDchain::iterate_in_out() {
	for (auto blk : CFG) {
		auto preds = blk.pred;
		// in = out of preds
		for (auto idx : preds) { 
			in.at(blk.number) = merge(in.at(blk.number), out.at(idx));
		}
		// out = in
		out.at(blk.number) = in.at(blk.number);
		// out = out - kill
		for (auto one : gen.at(blk.number)) {
			auto defVar = def2var[one];			
			if (find_var_def(out.at(blk.number), defVar)) {
				erase_defs(out.at(blk.number), defVar);
			}
		}
		// out = out U gen
		out.at(blk.number) = merge(out.at(blk.number), gen.at(blk.number));
	}
}

//根据gen、kill计算in，out
void UDchain::count_in_and_out() {
	while (true) {
		auto in_old = in;
		auto out_old = out;
		
		iterate_in_out();

		if (in_old == in && out_old == out) {
			break;
		}
	}
}

bool UDchain::checkPhi(string var, int blkNum) {
	for (int i = 0; i < CFG.size(); i++) {
		for (int j = 0; j < CFG.at(i).phi.size(); j++) {
			if (CFG.at(i).phi.at(j).name == var) {
				return true;
			}
		}
	}
	return false;
}

//记录临时变量的d-u链
void UDchain::add_A_DUChain(string var, const Node& use) {
	for (auto def : duchains) {
		if (def.first.first == var) {
			duchains[def.first] = use;
			break;
		}
	}
}

void UDchain::add_A_UDChain(string var, const Node& use) {
	int i = use.bIdx;
	int j = use.lIdx;
	pair<string, Node> tmp(var, use);
	//遍历基本块in结合的def
	for (auto def : in.at(i)) {
		if (def2var[def] == var) {	//定义点找到
			if (chains.find(tmp) != chains.end() && chains[tmp] != def && !checkPhi(var, i)) {
				auto tmp = FORMAT("use ({} <{},{}>) have def over one time!", var, i, j);
				WARN_MSG(tmp.c_str());
			}
			chains[tmp] = def;
		}
	}
	//遍历基本块内部的def是否是变量的定义
	for (auto def : gen.at(i)) {
		if (def2var[def] == var) {
			if (chains.find(tmp) != chains.end()) {	//还没有var的使用-定义链
				if (chains.find(tmp) != chains.end() && chains[tmp] != def && !checkPhi(var, i)) {
					auto tmp = FORMAT("use ({} <{},{}>) have def over one time!", var, i, j);
					WARN_MSG(tmp.c_str());
				}
				if (def.lIdx < j) {
					chains[tmp] = def;
				}
			}
			else if (def.lIdx < j) {	//已经有了var的使用-定义链
				if (chains.find(tmp) != chains.end() && chains[tmp] != def && !checkPhi(var, i)) {
					auto tmp = FORMAT("use ({} <{},{}>) have def over one time!", var, i, j);
					WARN_MSG(tmp.c_str());
				}
				chains[tmp] = def;
			}
			else {
				//nothing!!
			}
		}
	}
}


//理论上除了phi函数定义的变量都只有一个定义点！！
//计算整个流程图的使用-定义链
void UDchain::count_UDchain() {
	// i 代表第几个基本块
	for (int i = 0; i < CFG.size(); i++) {
		auto blkIr = CFG.at(i).Ir;
		//遍历指令找Use
		for (int j = 0; j < blkIr.size(); j++) {
			auto instr = blkIr.at(j);
			auto op = instr.getCodetype();
			auto res = instr.getResult();
			auto ope1 = instr.getOperand1();
			auto ope2 = instr.getOperand2();
			switch (op)
			{
			case ADD: case SUB: case DIV: case MUL: case REM:
			case AND: case OR:
			case EQL: case NEQ: case SGT: case SGE: case SLT: case SLE: {
				if (isTmp(ope1)) {
					add_A_UDChain(ope1, Node(i, j, ope1));
					add_A_DUChain(ope1, Node(i, j, ope1));
				}
				if (isTmp(ope2)) {
					add_A_UDChain(ope2, Node(i, j, ope2));
					add_A_DUChain(ope2, Node(i, j, ope2));
				}
				break;
			}
			case NOT: {
				add_A_UDChain(ope1, Node(i, j, ope1));
				add_A_DUChain(ope1, Node(i, j, ope1));
				break;
			}
			case STORE: {
				if (isTmp(res)) {
					add_A_UDChain(res, Node(i, j, res));
					add_A_DUChain(res, Node(i, j, res));
				}
				break;
			}
			case STOREARR: {
				if (isTmp(res)) {
					add_A_UDChain(res, Node(i, j, res));
					add_A_DUChain(res, Node(i, j, res));
				}
				if (isTmp(ope2)) {
					add_A_UDChain(ope2, Node(i, j, ope2));
					add_A_DUChain(ope2, Node(i, j, ope2));
				}
				break;
			}
			case LOAD: {	//全局变量如何处理
				if (ope2 != "para" && ope2 != "array") {
					add_A_UDChain(ope1, Node(i, j, ope1));
				}
				break;
			}
			case LOADARR: {
				if (isTmp(ope2)) {
					add_A_UDChain(ope2, Node(i, j, ope2));
					add_A_DUChain(ope2, Node(i, j, ope2));
				}
				break;
			}
			case RET: case PUSH: case BR: {
				if (isTmp(ope1)) {
					add_A_UDChain(ope1, Node(i, j, ope1));
					add_A_DUChain(ope1, Node(i, j, ope1));
				}
				break;
			}
			default:
				break;
			}
		}
	}
}


void UDchain::printUDchain(ofstream& ud) {
	if (TIJIAO) {
		ud << "func's UDChains" << endl;
		for (auto chain : chains) {
			ud << '\t' << chain.first.first << " = USE: <" << chain.first.second.bIdx << ", " << chain.first.second.lIdx << ">,    DEF: " << chain.second.display() << endl;
		}
		ud << "func's DUChains" << endl;
		
		for (auto chain : duchains) {
			ud << '\t' << chain.first.first << " = DEF: <" << chain.first.second.bIdx << ", " << chain.first.second.lIdx << ">,    USE: " << chain.second.display() << endl;
		}

		ud << "var 2 defs:" << endl;
		for (auto one : var2defs) {
			ud << one.first << " : ";
			for (auto def : one.second) {
				ud << def.display() << " ";
			}
			ud << endl;
		}

		ud << endl;
		ud << "in:" << endl;
		for (int i = 0; i < in.size(); i++) {
			ud << "B" << i << ":  ";
			for (auto one : in.at(i)) {
				ud << one.display() << " ";
			}
			ud << endl;
		}
		ud << "out:" << endl;
		for (int i = 0; i < out.size(); i++) {
			ud << "B" << i << ":  ";
			for (auto one : out.at(i)) {
				ud << one.display() << " ";
			}
			ud << endl;
		}
		ud << "gen:" << endl;
		for (int i = 0; i < gen.size(); i++) {
			ud << "B" << i << ":  ";
			for (auto one : gen.at(i)) {
				ud << one.display() << " ";
			}
			ud << endl;
		}
		ud << "def -> var" << endl;
		for (auto def : def2var) {
			ud << def.first.display() << " ";
		}
		ud << endl;
	}
}

//===================================================================
// udchain1
//===================================================================

void UDchain1::init() {
	int size = CFG.size();
	for (int i = 0; i < size; i++) {
		in.push_back(set<Node>());
		out.push_back(set<Node>());
		gen.push_back(set<Node>());
	}
}

//根据CFG计算每个基本块的gen集合、kill集合
void UDchain1::count_gen() {
	for (int i = 0; i < CFG.size(); i++) {
		auto blkIr = CFG.at(i).Ir;
		for (int j = 0; j < blkIr.size(); j++) {
			auto instr = blkIr.at(j);
			auto res = instr.getResult();
			auto ope1 = instr.getOperand1();
			auto ope2 = instr.getOperand2();
			switch (instr.getCodetype())
			{
			case ADD: case SUB: case MUL: case DIV: case REM:
			case AND: case OR: case NOT:
			case EQL: case NEQ: case SLT: case SLE: case SGT: case SGE:
			case LOAD: case LOADARR:
			{
				Node def1(i, j, res);
				gen.at(i).insert(def1);
				def2var[def1] = res;
				if (var2defs.find(res) != var2defs.end()) {
					var2defs[res].insert(def1);
				}
				else {
					var2defs[res] = set<Node>();
					var2defs[res].insert(def1);
				}
				//duchains[pair<string, Node>(res, def1)] = Node();
				break;
			} case STORE: {
				if (!isGlobal(ope1)) {
					Node def1(i, j, ope1);
					gen.at(i).insert(def1);
					def2var[def1] = ope1;
					if (var2defs.find(ope1) != var2defs.end()) {
						var2defs[ope1].insert(def1);
					}
					else {
						var2defs[ope1] = set<Node>();
						var2defs[ope1].insert(def1);
					}
				}
				break;
			} case STOREARR: {
				if (!isGlobal(ope1)) {	//非全局数组
					Node def1(i, j, ope1);
					gen.at(i).insert(def1);
					if (var2defs.find(ope1) != var2defs.end()) {
						var2defs[ope1].insert(def1);
					}
					else {
						var2defs[ope1] = set<Node>();
						var2defs[ope1].insert(def1);
					}
				}
				break;
			} case CALL: {
				if (isTmp(res)) {
					Node def1(i, j, res);
					gen.at(i).insert(def1);
					def2var[def1] = res;
					if (var2defs.find(res) != var2defs.end()) {
						var2defs[res].insert(def1);
					}
					else {
						var2defs[res] = set<Node>();
						var2defs[res].insert(def1);
					}
					//duchains[pair<string, Node>(res, def1)] = Node();
				}
				break;
			} case PHI: {
				if (!isGlobal(res)) {
					Node def1(i, j, res);
					gen.at(i).insert(def1);
					def2var[def1] = res;
					if (var2defs.find(res) != var2defs.end()) {
						var2defs[res].insert(def1);
					}
					else {
						var2defs[res] = set<Node>();
						var2defs[res].insert(def1);
					}
					//duchains[pair<string, Node>(res, def1)] = Node();
				}
				break;
			} case PARA: {
				if (ope2 == "0") {	//单值变量
					Node def1(i, j, res);
					gen.at(i).insert(def1);
					def2var[def1] = res;
					if (var2defs.find(res) != var2defs.end()) {
						var2defs[res].insert(def1);
					}
					else {
						var2defs[res] = set<Node>();
						var2defs[res].insert(def1);
					}
					//duchains[pair<string, Node>(res, def1)] = Node();
				}
				break;
			}
			default:
				break;
			}
		}
	}
}

bool UDchain1::find_var_def(set<Node> container, string var) {
	for (auto def : container) {
		try {
			if (def2var[def] == var) {
				return true;
			}
		}
		catch (exception e) {
			WARN_MSG("???");
		}
	}
	return false;
}

void UDchain1::erase_defs(set<Node>& container, string var) {
	try {
		for (auto it = container.begin(); it != container.end();) {
			if (def2var[*it] == var) {
				it = container.erase(it);
				continue;
			}
			it++;
		}
	}
	catch (exception e) {
		WARN_MSG("wow!! wrong!");
	}
}


void UDchain1::iterate_in_out() {
	for (auto blk : CFG) {
		auto preds = blk.pred;
		// in = out of preds
		for (auto idx : preds) {
			in.at(blk.number) = merge(in.at(blk.number), out.at(idx));
		}
		// out = in
		out.at(blk.number) = in.at(blk.number);
		// out = out - kill
		for (auto one : gen.at(blk.number)) {
			auto defVar = def2var[one];
			if (find_var_def(out.at(blk.number), defVar)) {
				erase_defs(out.at(blk.number), defVar);
			}
		}
		// out = out U gen
		out.at(blk.number) = merge(out.at(blk.number), gen.at(blk.number));
	}
}

//根据gen、kill计算in，out
void UDchain1::count_in_and_out() {
	while (true) {
		auto in_old = in;
		auto out_old = out;

		iterate_in_out();

		if (in_old == in && out_old == out) {
			break;
		}
	}
}

//记录临时变量的d-u链
void UDchain1::add_A_DUChain(string var, const Node& use) {
	for (auto def : duchains) {
		if (def.first.first == var) {
			duchains[def.first] = use;
			break;
		}
	}
}

void UDchain1::add_A_UDChain(string var, const Node& use) {
	int i = use.bIdx;
	int j = use.lIdx;
	pair<string, Node> tmp(var, use);
	for (auto def : gen.at(i)) {	//如果本基本块前面有def，那么直接用这个def即可
		if (def2var[def] == var && def.lIdx < j) {
			if (chains.find(tmp) != chains.end()) {	//找到这个变量对应的 ud-链
				if (chains[tmp].size() != 1) {	//如果定义点不是一个就出了问题
					string msg = FORMAT("why can find more than one def {}", var);
					WARN_MSG(msg.c_str());
				}
				else {
					auto it = chains[tmp].begin();
					if (it->lIdx < def.lIdx) {
						chains[tmp].erase(it);
						chains[tmp].insert(def);
					}
				}
			}
			else {
				chains[tmp] = set<Node>();
				chains[tmp].insert(def);
			}
			return;
		}
	}
	//否则，def来自于in的所有同var的def
	for (auto def : in.at(i)) {
		if (def2var[def] == var) {	//定义点找到
			if (chains.find(tmp) != chains.end()) {
				chains[tmp].insert(def);
			}
			else {
				chains[tmp] = set<Node>();
				chains[tmp].insert(def);
			}
		}
	}
}


//理论上除了phi函数定义的变量都只有一个定义点！！
//计算整个流程图的使用-定义链
void UDchain1::count_UDchain() {
	// i 代表第几个基本块
	for (int i = 0; i < CFG.size(); i++) {
		auto blkIr = CFG.at(i).Ir;
		//遍历指令找Use
		for (int j = 0; j < blkIr.size(); j++) {
			auto instr = blkIr.at(j);
			auto op = instr.getCodetype();
			auto res = instr.getResult();
			auto ope1 = instr.getOperand1();
			auto ope2 = instr.getOperand2();
			switch (op)
			{
			case ADD: case SUB: case DIV: case MUL: case REM:
			case AND: case OR:
			case EQL: case NEQ: case SGT: case SGE: case SLT: case SLE: {
				if (isTmp(ope1)) {
					add_A_UDChain(ope1, Node(i, j, ope1));
					add_A_DUChain(ope1, Node(i, j, ope1));
				}
				if (isTmp(ope2)) {
					add_A_UDChain(ope2, Node(i, j, ope2));
					add_A_DUChain(ope2, Node(i, j, ope2));
				}
				break;
			}
			case NOT: {
				add_A_UDChain(ope1, Node(i, j, ope1));
				add_A_DUChain(ope1, Node(i, j, ope1));
				break;
			}
			case STORE: {
				if (isTmp(res)) {
					add_A_UDChain(res, Node(i, j, res));
					add_A_DUChain(res, Node(i, j, res));
				}
				break;
			}
			case STOREARR: {
				if (isTmp(res)) {
					add_A_UDChain(res, Node(i, j, res));
					add_A_DUChain(res, Node(i, j, res));
				}
				if (isTmp(ope2)) {
					add_A_UDChain(ope2, Node(i, j, ope2));
					add_A_DUChain(ope2, Node(i, j, ope2));
				}
				break;
			}
			case LOAD: {	//全局变量如何处理
				if (ope2 != "para" && ope2 != "array") {
					add_A_UDChain(ope1, Node(i, j, ope1));
				}
				break;
			}
			case LOADARR: {
				if (isTmp(ope2)) {
					add_A_UDChain(ope2, Node(i, j, ope2));
					add_A_DUChain(ope2, Node(i, j, ope2));
				}
				break;
			}
			case RET: case PUSH: case BR: {
				if (isTmp(ope1)) {
					add_A_UDChain(ope1, Node(i, j, ope1));
					add_A_DUChain(ope1, Node(i, j, ope1));
				}
				break;
			}
			default:
				break;
			}
		}
	}
}


void UDchain1::printUDchain(ofstream& ud) {
	if (TIJIAO) {
		ud << "func's UDChains" << endl;
		for (auto chain : chains) {
			string defInfo = "";
			for (auto one : chain.second) {
				defInfo += one.display() + " ";
			}
			ud << '\t' << chain.first.first << " = USE: <" << chain.first.second.bIdx << ", " << chain.first.second.lIdx << ">,    DEF: " << defInfo << endl;
		}
		/*ud << "func's DUChains" << endl;

		for (auto chain : duchains) {
			ud << '\t' << chain.first.first << " = DEF: <" << chain.first.second.bIdx << ", " << chain.first.second.lIdx << ">,    USE: " << chain.second.display() << endl;
		}*/

		ud << "var 2 defs:" << endl;
		for (auto one : var2defs) {
			ud << one.first << " : ";
			for (auto def : one.second) {
				ud << def.display() << " ";
			}
			ud << endl;
		}

		ud << endl;
		ud << "in:" << endl;
		for (int i = 0; i < in.size(); i++) {
			ud << "B" << i << ":  ";
			for (auto one : in.at(i)) {
				ud << one.display() << " ";
			}
			ud << endl;
		}
		ud << "out:" << endl;
		for (int i = 0; i < out.size(); i++) {
			ud << "B" << i << ":  ";
			for (auto one : out.at(i)) {
				ud << one.display() << " ";
			}
			ud << endl;
		}
		ud << "gen:" << endl;
		for (int i = 0; i < gen.size(); i++) {
			ud << "B" << i << ":  ";
			for (auto one : gen.at(i)) {
				ud << one.display() << " ";
			}
			ud << endl;
		}
		ud << "def -> var" << endl;
		for (auto def : def2var) {
			ud << def.first.display() << " ";
		}
		ud << endl;
	}
}