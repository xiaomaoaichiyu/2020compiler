#include "ssa.h"

TreeNode::TreeNode() {
    this->op = ADD;
    this->nameset = set<string>();
    this->leftchild = this->rightchild = this->father = -1;
    this->nodeid = -1;
}

void SSA::build_dag() {
	int i, j, k;
	int size1 = blockCore.size();
    bool debug = false; // debug
	for (i = 1; i < size1; i++) {	// 遍历函数
		int size2 = blockCore[i].size();
		for (j = 0; j < size2; j++) {	// 遍历基本块
			if (blockCore[i][j].Ir.empty()) continue;
			map<string, int> node2id;	// 节点编号
			vector<TreeNode> dagtree;	// dag图
			vector<int> calIrNum;	// 计算型中间代码的编号
            map<string, string> tmp2var;    // 保存局部变量的临时变量
            map<string, string> var2tmp;    // 保存临时变量的局部变量
			int size3 = blockCore[i][j].Ir.size();
			for (k = 0; k < size3; k++)
			{
				CodeItem ci = blockCore[i][j].Ir[k];
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
                        if (ifTempVariable(ci.getOperand1())) {
                            if (tmp2var.find(ci.getOperand1()) == tmp2var.end()) {
                                cout << "dag图op1 error：临时变量参加运算前未定义." << endl;
                            }
                            else {
                                string varName = tmp2var[ci.getOperand1()];
                                tmp2var.erase(ci.getOperand1());
                                var2tmp.erase(varName);
                                if (varName2St[i][varName].getDimension() == 0) {   // 暂不考虑数组
                                    if (node2id.find(varName) == node2id.end()) {   // 创建叶子节点
                                        op1node.nodeid = dagtree.size();
                                        op1node.nameset.insert(varName);
                                        dagtree.push_back(op1node);
                                        node2id[varName] = op1node.nodeid;
                                    }
                                    else {
                                        op1node = dagtree[node2id[varName]];
                                    }
                                }
                            }
                        }
                        else if (ifDigit(ci.getOperand1())) {
                            string constVal = ci.getOperand1();
                            if (node2id.find(constVal) == node2id.end()) {  // 创建叶子节点
                                op1node.nodeid = dagtree.size();
                                op1node.nameset.insert(constVal);
                                dagtree.push_back(op1node);
                                node2id[constVal] = op1node.nodeid;
                            }
                            else {
                                op1node = dagtree[node2id[constVal]];
                            }
                        }
                        else {
                            cout << "dag图op1 error：运算型指令运算数不是临时变量或常数" << endl;
                        }
                    }
                    {   // Operand2
                        if (ifTempVariable(ci.getOperand2())) {
                            if (tmp2var.find(ci.getOperand2()) == tmp2var.end()) {
                                cout << "dag图op2 error：临时变量参加运算前未定义." << endl;
                            }
                            else {
                                string varName = tmp2var[ci.getOperand2()];
                                tmp2var.erase(ci.getOperand2());
                                var2tmp.erase(varName);
                                if (varName2St[i][varName].getDimension() == 0) {   // 暂不考虑数组
                                    if (node2id.find(varName) == node2id.end()) {   // 创建叶子节点
                                        op2node.nodeid = dagtree.size();
                                        op2node.nameset.insert(varName);
                                        dagtree.push_back(op2node);
                                        node2id[varName] = op2node.nodeid;
                                    }
                                    else {
                                        op2node = dagtree[node2id[varName]];
                                    }
                                }
                            }
                        }
                        else if (ifDigit(ci.getOperand2())) {
                            string constVal = ci.getOperand2();
                            if (node2id.find(constVal) == node2id.end()) {  // 创建叶子节点
                                op2node.nodeid = dagtree.size();
                                op2node.nameset.insert(constVal);
                                dagtree.push_back(op2node);
                                node2id[constVal] = op2node.nodeid;
                            }
                            else {
                                op2node = dagtree[node2id[constVal]];
                            }
                        }
                        else {
                            cout << "dag图op2 error：运算型指令运算数不是临时变量或常数" << endl;
                        }
                    }
                    {   // 中间节点
                        op1nodeid = op1node.nodeid;
                        op2nodeid = op2node.nodeid;
                        if (op1nodeid == -1 || op2nodeid == -1) break;  // 有数组元素参与运算
                        for (int m = 0; m < dagtree.size(); m++) {
                            irCodeType iCT = ci.getCodetype();
                            if (dagtree[m].op == ci.getCodetype()) {
                                if ((dagtree[m].leftchild == op1nodeid && dagtree[m].rightchild == op2nodeid) ||
                                    ((iCT == ADD || iCT == MUL) && dagtree[m].leftchild == op2nodeid && dagtree[m].rightchild == op1nodeid)) {
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
                        if (!ifTempVariable(ci.getResult())) {
                            cout << "dag图 result error：运算指令结果不是临时变量." << endl;
                        }
                        node2id[ci.getResult()] = opnodeid;
                        dagtree[opnodeid].nameset.insert(ci.getResult());
                    }
                    calIrNum.push_back(k);
                    break;
                case AND:
                    break;
                case OR:
                    break;
                case NOT:
                    break;
                case EQL:
                    break;
                case NEQ:
                    break;
                case SGT:
                    break;
                case SGE:
                    break;
                case SLT:
                    break;
                case SLE:
                    break;
                case ALLOC:
                    break;
                case STORE:
                    break;
                case STOREARR:
                    break;
                case LOAD:

                    break;
                case LOADARR:
                    break;
                case INDEX:
                    break;
                case CALL:
                    break;
                case RET:
                    break;
                case PUSH:
                    break;
                case POP:
                    break;
                case LABEL:
                    break;
                case BR:
                    break;
                case DEFINE:
                    break;
                case PARA:
                    break;
                case GLOBAL:
                    break;
                case NOTE:
                    break;
                case MOV:
                    break;
                case LEA:
                    break;
                case GETREG:
                    break;
                case PHI:
                    break;
                case ARRAYINIT:
                    break;
                default:
                    break;
				}
			}
		}
	}
}