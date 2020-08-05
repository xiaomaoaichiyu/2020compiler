#include "ssa.h"
#include "../front/syntax.h"

TreeNode::TreeNode() {
    this->op = ADD;
    this->leftchild = this->rightchild = this->father = -1;
    this->nodeid = -1;
    this->ifIn = false;
    this->nameset = set<string>();
}

map<irCodeType, string> irCodeType2Output = { {ADD, "ADD"}, {SUB, "SUB"}, {MUL, "MUL"}, {DIV, "DIV"}, {REM, "REM"},
    {LOAD, "LOAD"}, {LOADARR, "LOADARR"}, {STORE, "STORE"}, {STOREARR, "STOREARR"} };

string SSA::getNewTempVariable() {
    string temp = "%" + to_string(func2tmpIndex[codetotal.size() - 1]);
    func2tmpIndex[codetotal.size() - 1]++;
    return temp;
}

// 是否是运算型指令
bool SSA::ifCalIr(irCodeType ict) {
    return ict == ADD || ict == SUB || ict == DIV || ict == MUL || ict == REM || ict == LOAD || ict == STORE;
        // || ict == LOADARR || ict == STOREARR;
}

void SSA::delete_common_sub_exp(int funNum, int blkNum, int IrStart, int IrEnd) {
    map<string, int> node2id;	    // 节点编号
    vector<TreeNode> dagtree;	// dag图
    map<string, int> var2Count; // 变量第几次出现
    bool debug = true;              // debug
    for (int i = IrStart; i < IrEnd; i++) {
        CodeItem ci = blockCore[funNum][blkNum].Ir[i];
        TreeNode op1node, op2node, opnode;
        int op1nodeid = -1, op2nodeid = -1, opnodeid = -1;
        switch (ci.getCodetype())
        {
        case ADD:
        case SUB:
        case DIV:
        case MUL:
        case REM:
            if (debug) cout << ci.getContent() << endl;
            {   // Operand1
                if (ifTempVariable(ci.getOperand1()) || ifDigit(ci.getOperand1())) {
                    string op1Name = ci.getOperand1();
                    if (node2id.find(op1Name) == node2id.end()) {   // 创建叶子节点
                        op1nodeid = dagtree.size();
                        op1node.nodeid = op1nodeid;
                        dagtree.push_back(op1node);
                        node2id[op1Name] = op1nodeid;
                    }
                    else {
                        op1node = dagtree[node2id[op1Name]];
                        op1nodeid = op1node.nodeid;
                    }
                }
                else {
                    cout << "dag图op1 error：运算型指令运算数不是临时变量或常数" << endl;
                }
            }
            {   // Operand2
                if (ifTempVariable(ci.getOperand2()) || ifDigit(ci.getOperand2())) {
                    string op2Name = ci.getOperand2();
                    if (node2id.find(op2Name) == node2id.end()) {   // 创建叶子节点
                        op2nodeid = dagtree.size();
                        op2node.nodeid = op2nodeid;
                        dagtree.push_back(op2node);
                        node2id[op2Name] = op2nodeid;
                    }
                    else {
                        op2node = dagtree[node2id[op2Name]];
                        op2nodeid = op2node.nodeid;
                    }
                }
                else {
                    cout << "dag图op2 error：运算型指令运算数不是临时变量或常数" << endl;
                }
            }
            {   // 中间节点
                for (int m = 0; m < dagtree.size(); m++) {
                    if (dagtree[m].op == ci.getCodetype()) {
                        if ((dagtree[m].leftchild == op1nodeid && dagtree[m].rightchild == op2nodeid) ||
                            ((ci.getCodetype() == ADD || ci.getCodetype() == MUL) && dagtree[m].leftchild == op2nodeid && dagtree[m].rightchild == op1nodeid)) {
                            opnode = dagtree[m];
                            opnodeid = opnode.nodeid;
                            break;
                        }
                    }
                }
                if (opnodeid == -1) {
                    opnodeid = dagtree.size();
                    opnode.nodeid = opnodeid;
                    opnode.op = ci.getCodetype();
                    opnode.leftchild = op1nodeid;
                    opnode.rightchild = op2nodeid;
                    dagtree.push_back(opnode);
                    dagtree[op1nodeid].father = opnodeid;
                    dagtree[op2nodeid].father = opnodeid;
                }
            }
            {   // Result     
                if (opnodeid == -1) {
                    cout << "dag error: 运算型指令结果没有对应节点." << endl;
                }
                node2id[ci.getResult()] = opnodeid;
            }
            break;
        case STORE:
            if (debug) cout << ci.getContent() << endl;
            {   // Result
                if (ifTempVariable(ci.getResult()) || ifDigit(ci.getResult())) {
                    string resultName = ci.getResult();
                    if (node2id.find(resultName) == node2id.end()) {   // 创建叶子节点
                        op1nodeid = dagtree.size();
                        op1node.nodeid = op1nodeid;
                        dagtree.push_back(op1node);
                        node2id[resultName] = op1nodeid;
                    }
                    else {
                        op1node = dagtree[node2id[resultName]];
                        op1nodeid = op1node.nodeid;
                    }
                }
                else {
                    cout << "dag图result error：store指令赋值值不是临时变量或常数." << endl;
                }
            }
            {   // Operand1
                if (op1nodeid == -1) {
                    cout << "dag error: store指令op1没有对应节点." << endl;
                }
                node2id[ci.getOperand1()] = op1nodeid;
            }
            break;
        case STOREARR:  // result op2 作为两个操作数
            if (debug) cout << ci.getContent() << endl;
            {   // Result
                if (ifTempVariable(ci.getResult()) || ifDigit(ci.getResult())) {
                    string resultName = ci.getResult();
                    if (node2id.find(resultName) == node2id.end()) {   // 创建叶子节点
                        op1nodeid = dagtree.size();
                        op1node.nodeid = op1nodeid;
                        dagtree.push_back(op1node);
                        node2id[resultName] = op1nodeid;
                    }
                    else {
                        op1node = dagtree[node2id[resultName]];
                        op1nodeid = op1node.nodeid;
                    }
                }
                else {
                    cout << "dag图result error：storearr指令赋值值不是临时变量或常数." << endl;
                }
            }
            {   // Operand2
                if (ifTempVariable(ci.getOperand2()) || ifDigit(ci.getOperand2())) {
                    string op2Name = ci.getOperand2();
                    if (node2id.find(op2Name) == node2id.end()) {   // 创建叶子节点
                        op2nodeid = dagtree.size();
                        op2node.nodeid = op2nodeid;
                        dagtree.push_back(op2node);
                        node2id[op2Name] = op2nodeid;
                    }
                    else {
                        op2node = dagtree[node2id[op2Name]];
                        op2nodeid = op2node.nodeid;
                    }
                }
                else {
                    cout << "dag图op2 error：storearr指令运算数不是临时变量或常数" << endl;
                }
            }
            {   // 中间节点
                for (int m = 0; m < dagtree.size(); m++) {
                    if (dagtree[m].op == ci.getCodetype()) {
                        if (dagtree[m].leftchild == op1nodeid && dagtree[m].rightchild == op2nodeid) {
                            opnode = dagtree[m];
                            opnodeid = opnode.nodeid;
                            break;
                        }
                    }
                }
                if (opnodeid == -1) {
                    opnodeid = dagtree.size();
                    opnode.nodeid = opnodeid;
                    opnode.op = ci.getCodetype();
                    opnode.leftchild = op1nodeid;
                    opnode.rightchild = op2nodeid;
                    dagtree.push_back(opnode);
                    dagtree[op1nodeid].father = opnodeid;
                    dagtree[op2nodeid].father = opnodeid;
                }
            }
            {   // Operand1     
                if (opnodeid == -1) {
                    cout << "dag error: storearr指令op1没有对应节点." << endl;
                }
                node2id[ci.getOperand1()] = opnodeid;
            }
            break;
        case LOAD:
            if (debug) cout << ci.getContent() << endl;
            {   // Operand1
                if (ifLocalVariable(ci.getOperand1()) || ifDigit(ci.getOperand1())) {
                    string op1Name = ci.getOperand1();
                    if (node2id.find(op1Name) == node2id.end()) {   // 创建叶子节点
                        op1nodeid = dagtree.size();
                        op1node.nodeid = op1nodeid;
                        dagtree.push_back(op1node);
                        node2id[op1Name] = op1nodeid;
                    }
                    else {
                        op1node = dagtree[node2id[op1Name]];
                        op1nodeid = op1node.nodeid;
                    }
                }
                else {
                    cout << "dag图op1 error：load型指令运算数不是局部变量或常数" << endl;
                }
            }
            {   // Result
                if (op1nodeid == -1) {
                    cout << "dag error: load指令result没有对应节点." << endl;
                }
                node2id[ci.getResult()] = op1nodeid;
            }
            break;
        case LOADARR:   // 与ADD型运算指令处理逻辑相同
            if (debug) cout << ci.getContent() << endl;
            {   // Operand1
                if (ifLocalVariable(ci.getOperand1()) || ifGlobalVariable(ci.getOperand1())) {
                    string op1Name = ci.getOperand1();
                    if (node2id.find(op1Name) == node2id.end()) {   // 创建叶子节点
                        op1nodeid = dagtree.size();
                        op1node.nodeid = op1nodeid;
                        dagtree.push_back(op1node);
                        node2id[op1Name] = op1nodeid;
                    }
                    else {
                        op1node = dagtree[node2id[op1Name]];
                        op1nodeid = op1node.nodeid;
                    }
                }
                else {
                    cout << "dag图op1 error：loadarr运算数不是局部变量或全局变量." << endl;
                }
            }
            {   // Operand2
                if (ifTempVariable(ci.getOperand2()) || ifDigit(ci.getOperand2())) {
                    string op2Name = ci.getOperand2();
                    if (node2id.find(op2Name) == node2id.end()) {   // 创建叶子节点
                        op2nodeid = dagtree.size();
                        op2node.nodeid = op2nodeid;
                        dagtree.push_back(op2node);
                        node2id[op2Name] = op2nodeid;
                    }
                    else {
                        op2node = dagtree[node2id[op2Name]];
                        op2nodeid = op2node.nodeid;
                    }
                }
                else {
                    cout << "dag图op2 error：loadarr运算数不是临时变量或常数" << endl;
                }
            }
            {   // 中间节点
                for (int m = 0; m < dagtree.size(); m++) {
                    if (dagtree[m].op == ci.getCodetype()) {
                        if ((dagtree[m].leftchild == op1nodeid && dagtree[m].rightchild == op2nodeid) ||
                            ((ci.getCodetype() == ADD || ci.getCodetype() == MUL) && dagtree[m].leftchild == op2nodeid && dagtree[m].rightchild == op1nodeid)) {
                            opnode = dagtree[m];
                            opnodeid = opnode.nodeid;
                            break;
                        }
                    }
                }
                if (opnodeid == -1) {
                    opnodeid = dagtree.size();
                    opnode.nodeid = opnodeid;
                    opnode.op = ci.getCodetype();
                    opnode.leftchild = op1nodeid;
                    opnode.rightchild = op2nodeid;
                    dagtree.push_back(opnode);
                    dagtree[op1nodeid].father = opnodeid;
                    dagtree[op2nodeid].father = opnodeid;
                }
            }
            {   // Result     
                if (opnodeid == -1) {
                    cout << "dag error: loadarr结果没有对应节点." << endl;
                }
                node2id[ci.getResult()] = opnodeid;
            }
            break;
        case AND:
        case OR:
        case NOT:
        case EQL:
        case NEQ:
        case SGT:
        case SGE:
        case SLT:
        case SLE:
        case ALLOC:
        case INDEX:
        case CALL:
        case RET:
        case PUSH:
        case POP:
        case LABEL:
        case BR:
        case DEFINE:
        case PARA:
        case GLOBAL:
        case NOTE:
        case MOV:
        case LEA:
        case GETREG:
        case PHI:
        case ARRAYINIT:
            cout << "dag error：删除公共子表达式不应出现的指令." << endl;
            break;
        default:
            break;
        }
    }
    for (map<string, int>::iterator iter = node2id.begin(); iter != node2id.end(); iter++)
        cout << iter->first << " " << iter->second << endl;
    for (int i = 0; i < dagtree.size(); i++)
        cout << dagtree[i].nodeid << " " << dagtree[i].leftchild << " " << dagtree[i].rightchild << " " << dagtree[i].father << " " << irCodeType2Output[dagtree[i].op] << " " << endl;
    vector<CodeItem> irQueue;
    vector<int> opNodeQueue;
    stack<int> opNodeStack;
    for (auto i : dagtree) {
        if (i.leftchild != -1 && i.rightchild != -1) opNodeQueue.push_back(i.nodeid);
    }
    bool update = true;
    while (update) {
        update = false;
        for (int i = 0; i < opNodeQueue.size(); i++) {
            int nodeid = opNodeQueue[i];
            if (!dagtree[nodeid].ifIn) {
                update = true;  // 仍然有未加入到stack的中间节点
                if (dagtree[nodeid].father == -1 || dagtree[dagtree[nodeid].father].ifIn) {
                    opNodeStack.push(nodeid);
                    dagtree[nodeid].ifIn = true;
                    while (dagtree[nodeid].leftchild != -1) {
                        nodeid = dagtree[nodeid].leftchild;
                        if (dagtree[nodeid].leftchild == -1 || dagtree[nodeid].rightchild == -1) break; // 不是中间节点
                        if (dagtree[nodeid].father == -1 || dagtree[dagtree[nodeid].father].ifIn) {
                            opNodeStack.push(nodeid);
                            dagtree[nodeid].ifIn = true;
                        }
                        else break;
                    }
                    break;
                }
            }
        }
    }
    if (debug) {
        while (!opNodeStack.empty()) { cout << "stack"; cout << opNodeStack.top() << " "; opNodeStack.pop(); }
        cout << endl;
    }
    while (!opNodeStack.empty()) {
        int nodeid = opNodeStack.top();
        opNodeStack.pop();
        TreeNode tn = dagtree[nodeid];
        
    }
}

void SSA::build_dag() {
	int i, j, k1, k2;
	int size1 = blockCore.size();
    bool debug = false; // debug
	for (i = 1; i < size1; i++) {	// 遍历函数
		int size2 = blockCore[i].size();
		for (j = 0; j < size2; j++) {	// 遍历基本块
			if (blockCore[i][j].Ir.empty()) continue;
            k1 = k2 = -1;
            while (true) {
                for (k1 = k2 + 1; k1 < blockCore[i][j].Ir.size(); k1++)
                    if (ifCalIr(blockCore[i][j].Ir[k1].getCodetype()))
                        break;
                if (k1 == blockCore[i][j].Ir.size()) break;
                for (k2 = k1 + 1; k2 < blockCore[i][j].Ir.size(); k2++)
                    if (!ifCalIr(blockCore[i][j].Ir[k2].getCodetype()))
                        break;
                delete_common_sub_exp(i, j, k1, k2);
            }
		}
	}
}