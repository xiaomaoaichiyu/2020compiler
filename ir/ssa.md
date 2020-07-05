// 使用方法
在前端基础上，Syntax.cpp头文件include "ssa.h"，并在第226行main函数的合适位置插入两行代码
    SSA ssa(codetotal);
    ssa.generate();
尚未完全实现，持续更新中

**注意：basicBlock结构中的start和end编号从1开始，即保存中间代码的vector结构的第0条中间代码在这里的编号是1；**

**注意：每个函数对基本块建立了流图分析，基本块0表示entry块，最后一个基本块表示exit块，中间的基本块才是将中间代码划分的基本块，因此第0条中间代码在基本块对应的编号是1**

```c++
class basicBlock {
public:
	int number;							// 基本块的编号
	int start;								// 基本块的起始中间代码
	int end;								// 基本块的结束中间代码
	std::set<int> pred;				// 前驱
	std::set<int> succeeds;			// 后继节点
	std::set<int> domin;				// 必经节点
	std::set<int> idom;				// 直接必经节点
	std::set<int> tmpIdom;			// 计算直接必经节点算法中需要的数据结构
	basicBlock(int number, int start, int end, std::set<int> pred, std::set<int> succeeds) {
		this->number = number;
		this->start = start;
		this->end = end;
		this->pred = pred;
		this->succeeds = succeeds;
	}
};
```

// 插入 $\phi$ 函数
1. 划分基本块
    * 入口点
    * 分支的目标（分支节点指有多个后继的节点），不应包含函数调用指令
    * 分支指令或返回指令之后的指令
2. 建立边关系，构造流图
3. 必经节点树
4. 生成后序遍历序列
5. 循环计算流图必经边界: $$DF_local$$ ∪ $$DF_up$$，求闭并，插入 $\phi$ 函数

// 变量重命名
1. 普通中间代码 -> SSA
2. SSA -> 普通中间代码

\primary + '@' + \int