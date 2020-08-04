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
	for (i = 1; i < size1; i++) {	// ��������
		int size2 = blockCore[i].size();
		for (j = 0; j < size2; j++) {	// ����������
			if (blockCore[i][j].Ir.empty()) continue;
			map<string, int> node2id;	// �ڵ���
			vector<TreeNode> dagtree;	// dagͼ
			vector<int> calIrNum;	// �������м����ı��
            map<string, string> tmp2var;    // ����ֲ���������ʱ����
            map<string, string> var2tmp;    // ������ʱ�����ľֲ�����
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
                                cout << "dagͼop1 error����ʱ�����μ�����ǰδ����." << endl;
                            }
                            else {
                                string varName = tmp2var[ci.getOperand1()];
                                tmp2var.erase(ci.getOperand1());
                                var2tmp.erase(varName);
                                if (varName2St[i][varName].getDimension() == 0) {   // �ݲ���������
                                    if (node2id.find(varName) == node2id.end()) {   // ����Ҷ�ӽڵ�
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
                            if (node2id.find(constVal) == node2id.end()) {  // ����Ҷ�ӽڵ�
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
                            cout << "dagͼop1 error��������ָ��������������ʱ��������" << endl;
                        }
                    }
                    {   // Operand2
                        if (ifTempVariable(ci.getOperand2())) {
                            if (tmp2var.find(ci.getOperand2()) == tmp2var.end()) {
                                cout << "dagͼop2 error����ʱ�����μ�����ǰδ����." << endl;
                            }
                            else {
                                string varName = tmp2var[ci.getOperand2()];
                                tmp2var.erase(ci.getOperand2());
                                var2tmp.erase(varName);
                                if (varName2St[i][varName].getDimension() == 0) {   // �ݲ���������
                                    if (node2id.find(varName) == node2id.end()) {   // ����Ҷ�ӽڵ�
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
                            if (node2id.find(constVal) == node2id.end()) {  // ����Ҷ�ӽڵ�
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
                            cout << "dagͼop2 error��������ָ��������������ʱ��������" << endl;
                        }
                    }
                    {   // �м�ڵ�
                        op1nodeid = op1node.nodeid;
                        op2nodeid = op2node.nodeid;
                        if (op1nodeid == -1 || op2nodeid == -1) break;  // ������Ԫ�ز�������
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
                            cout << "dagͼ result error������ָ����������ʱ����." << endl;
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