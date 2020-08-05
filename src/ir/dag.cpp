#include "ssa.h"

TreeNode::TreeNode() {
    this->op = ADD;
    this->leftchild = this->rightchild = this->father = -1;
    this->nodeid = -1;
}

// �Ƿ���������ָ��
bool SSA::ifCalIr(irCodeType ict) {
    return ict == ADD || ict == SUB || ict == DIV || ict == MUL || ict == REM ||
        ict == LOAD || ict == LOADARR || ict == STORE || ict == STOREARR;
}

void SSA::delete_common_sub_exp(int funNum, int blkNum, int IrStart, int IrEnd) {
    map<string, int> node2id;	    // �ڵ���
    vector<TreeNode> dagtree;	// dagͼ
    bool debug = true;             // debug
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
                    if (node2id.find(op1Name) == node2id.end()) {   // ����Ҷ�ӽڵ�
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
                    cout << "dagͼop1 error��������ָ��������������ʱ��������" << endl;
                }
            }
            {   // Operand2
                if (ifTempVariable(ci.getOperand2()) || ifDigit(ci.getOperand2())) {
                    string op2Name = ci.getOperand2();
                    if (node2id.find(op2Name) == node2id.end()) {   // ����Ҷ�ӽڵ�
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
                    cout << "dagͼop2 error��������ָ��������������ʱ��������" << endl;
                }
            }
            {   // �м�ڵ�
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
                    cout << "dag error: ������ָ����û�ж�Ӧ�ڵ�." << endl;
                }
                node2id[ci.getResult()] = opnodeid;
            }
            break;
        case STORE:
            if (debug) cout << ci.getContent() << endl;
            {   // Result
                if (ifTempVariable(ci.getResult()) || ifDigit(ci.getResult())) {
                    string resultName = ci.getResult();
                    if (node2id.find(resultName) == node2id.end()) {   // ����Ҷ�ӽڵ�
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
                    cout << "dagͼresult error��storeָ�ֵֵ������ʱ��������." << endl;
                }
            }
            {   // Operand1
                if (op1nodeid == -1) {
                    cout << "dag error: storeָ��op1û�ж�Ӧ�ڵ�." << endl;
                }
                node2id[ci.getOperand1()] = op1nodeid;
            }
            break;
        case STOREARR:  // result op2 ��Ϊ����������
            if (debug) cout << ci.getContent() << endl;
            {   // Result
                if (ifTempVariable(ci.getResult()) || ifDigit(ci.getResult())) {
                    string resultName = ci.getResult();
                    if (node2id.find(resultName) == node2id.end()) {   // ����Ҷ�ӽڵ�
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
                    cout << "dagͼresult error��storearrָ�ֵֵ������ʱ��������." << endl;
                }
            }
            {   // Operand2
                if (ifTempVariable(ci.getOperand2()) || ifDigit(ci.getOperand2())) {
                    string op2Name = ci.getOperand2();
                    if (node2id.find(op2Name) == node2id.end()) {   // ����Ҷ�ӽڵ�
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
                    cout << "dagͼop2 error��storearrָ��������������ʱ��������" << endl;
                }
            }
            {   // �м�ڵ�
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
                    cout << "dag error: storearrָ��op1û�ж�Ӧ�ڵ�." << endl;
                }
                node2id[ci.getOperand1()] = opnodeid;
            }
            break;
        case LOAD:
            if (debug) cout << ci.getContent() << endl;
            {   // Operand1
                if (ifLocalVariable(ci.getOperand1()) || ifDigit(ci.getOperand1())) {
                    string op1Name = ci.getOperand1();
                    if (node2id.find(op1Name) == node2id.end()) {   // ����Ҷ�ӽڵ�
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
                    cout << "dagͼop1 error��load��ָ�����������Ǿֲ���������" << endl;
                }
            }
            {   // Result
                if (op1nodeid == -1) {
                    cout << "dag error: loadָ��resultû�ж�Ӧ�ڵ�." << endl;
                }
                node2id[ci.getResult()] = op1nodeid;
            }
            break;
        case LOADARR:   // ��ADD������ָ����߼���ͬ
            if (debug) cout << ci.getContent() << endl;
            {   // Operand1
                if (ifLocalVariable(ci.getOperand1()) || ifGlobalVariable(ci.getOperand1())) {
                    string op1Name = ci.getOperand1();
                    if (node2id.find(op1Name) == node2id.end()) {   // ����Ҷ�ӽڵ�
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
                    cout << "dagͼop1 error��loadarr���������Ǿֲ�������ȫ�ֱ���." << endl;
                }
            }
            {   // Operand2
                if (ifTempVariable(ci.getOperand2()) || ifDigit(ci.getOperand2())) {
                    string op2Name = ci.getOperand2();
                    if (node2id.find(op2Name) == node2id.end()) {   // ����Ҷ�ӽڵ�
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
                    cout << "dagͼop2 error��loadarr������������ʱ��������" << endl;
                }
            }
            {   // �м�ڵ�
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
                    cout << "dag error: loadarr���û�ж�Ӧ�ڵ�." << endl;
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
            cout << "dag error��ɾ�������ӱ��ʽ��Ӧ���ֵ�ָ��." << endl;
            break;
        default:
            break;
        }
    }
    for (map<string, int>::iterator iter = node2id.begin(); iter != node2id.end(); iter++)
        cout << iter->first << " " << iter->second << endl;
    for (int i = 0; i < dagtree.size(); i++)
        cout << dagtree[i].nodeid << " " << dagtree[i].leftchild << " " << dagtree[i].rightchild << " " << dagtree[i].father << " " << dagtree[i].op << " " << endl;
    vector<CodeItem> irQueue;
    vector<int> opNodeQueue;

}

void SSA::build_dag() {
	int i, j, k1, k2;
	int size1 = blockCore.size();
    bool debug = false; // debug
	for (i = 1; i < size1; i++) {	// ��������
		int size2 = blockCore[i].size();
		for (j = 0; j < size2; j++) {	// ����������
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