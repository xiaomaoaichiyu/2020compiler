### SSA

SSA涉及到的两个文件 **ssa.h** 和 **ssa.cpp**.

SSA测试文件 **ssa_test.cpp**.

SSA优化文件 **ssa_optimize.cpp**

SSA主要完成的工作是 **插入\phi函数以及对其的处理** 与 **变量重命名 工作**.

SSA的全称是**static single assignment**，即静态单一赋值。SSA 形式的 IR 主要特征是每个变量只赋值一次。相比而言，非 SSA 形式的 IR 里一个变量可以赋值多次。采用SSA的目的是SSA形式的IR具有更好的可优化性。

因此，SSA这部分正确的处理顺序应该是：
$$
睿轩前端生成中间代码（第一版）
$$

$$
\downarrow ①
$$

$$
由SSA部分处理得到SSA形式的中间代码（第二版）
$$

$$
\downarrow ②
$$

$$
在SSA形式的IR上做各种优化
$$

$$
\downarrow ③
$$

$$
将SSA形式的IR转换为正常IR（第三版）
$$

$$
\downarrow ④
$$

$$
后端在第三版IR上生成汇编
$$

该部分完成的功能包括：划分基本块、建立前后序关系、计算def-use及活跃变量分析、确立必经关系及必经边界、插入\phi函数、变量重命名与对\phi函数的处理等。

**写在前面**：ssa.h内定义的所有类/结构，内部的成员变量都是public，如果需要可以直接修改其值。

##### 1. 划分基本块 divide_basic_block()

该函数的前提函数是 **find_primary_statement()**，即首先找到每个基本块的起始语句，再根据起始语句划分基本块。起始语句包括：第1条中间代码、BR跳转指令和RET跳转指令以及跳转到的label标签位置。

基本块结构见 **ssa.h** 中 **basicBlock**类。

基本块中包含了该基本块对应的中间代码（**Ir成员变量**），因此SSA形式的中间代码（第二版）的访问目前可以通过顺序依次遍历基本块中的中间代码获得。

基本块内有成员变量 **number**，该字段的含义是基本块的序号。我为每个函数手动添加了**entry块和exit块**，这两个块对应的序号number分别是0和size-1（即最后一个基本块），这两个基本块中不包含任何中间代码（即Ir为空），只是正常建立流图分析需要。实际上一个函数内部划分的基本块序号是从 1～size-2，记得根据需要控制遍历条件。

##### 2. 建立基本块前后序关系 build_pred_and_succeeds()

前后序分别对应 **basicBlock** 类中的 **pred** 和 **succeeds** 成员变量。

划分基本块后建立前后序关系。

某基本块的前序关系指的是执行完毕后到达该基本块的那些基本块；

某基本块的后序关系指的是该基本块执行完毕后到达的那些基本块；

##### 3. 建立必经关系 build_dom_tree()

必经节点对应 **basicBlock**类中的 **domin** 成员变量。

必经节点：从entry节点到i的每一条可能的执行路径都包含d，则说节点d是节点i的必经节点。

实现算法参考《高级编译器设计与实现》P132

##### 4. 建立直接必经关系 build_idom_tree()

直接必经节点对应 **basicBlock** 类中的 **idom** 成员变量。

直接必经节点：对于 a≠b，a idom b 当且仅当 a dom b 且不存在一个c≠a且c≠b 的节点c，使得 a dom c且c dom b。

实现算法参考《高级编译器设计与实现》P134

##### 5. 建立直接必经关系的反关系 build_reverse_idom_tree()

对应 **basicBlock** 类中的 reverse_idom 成员变量。

其含义只是由于遍历必经节点树的需要，比如 idom 关系包括 <b, a>, <c, a>，则reverse_idom 为 <a, b>, <a, c>。

##### 6. 前序遍历 build_pre_order() 和 后序遍历必经节点树 build_post_order()

每个函数的必经节点树前序遍历结果和后序遍历结果分别保存在 **SSA** 中 **preOrder** 和 **postOrder** 成员变量中。

##### 7. 计算def-use链 build_def_use_chain()

基本块内的def和use变量分别对应保存在 **basicBlock** 中的 **def** 和 **use** 成员变量。

def变量的插入规则：只分析局部变量和参数，**不包括中间临时变量和全局变量。**

use变量的插入规则：除上述要求外，**同时保证该变量在使用前未被定义。**

##### 8. 活跃变量分析 active_var_analyse()

分别对应 **basicBlock** 中的 **in** 和 **out** 集合。

实现算法与上学期编译课程狼书中介绍的相同，不再详述。

##### 9. 计算流图必经边界 build_dom_frontier()

对于流图的一个节点x，x的必经边界（dominance frontier）记作DF(x)，它是流图中所有满足后面这个条件的节点y的集合：x是y的直接前驱节点的必经节点，但不是y的严格必经节点，即
$$
DF(x) = \{y | (\exist z ∈ Pred(y)使得x \quad dom \quad z)并且x \quad !sdom \quad y\}
$$
实现算法参考《高级编译器设计与实现》P185.

##### 10. 计算变量的迭代必经边界 build_var_chain()

每个函数有一个变量链，对应 **SSA** 中的 **varChain** 结构，该变量链中保存了该函数内部的所有局部变量和参数，**不包括中间临时变量、全局变量和数组**。

每个变量对应 **varStruct** 结构，该结构记录了变量名和该变量的迭代必经边界信息。

迭代必经边界是旨在确定\phi函数的插入位置，即若变量%a的迭代必经边界计算出来为3，说明在基本块编号为3的基本块需要插入变量%a的\phi函数。

因此要对函数内的每个变量进行迭代必经边界分析，实现算法参考《高级编译器设计与实现》P186.

##### 11. 添加\phi函数 add_phi_function()

上面我们得出了函数内部每个变量需要插入\phi函数的位置，下面我们需要将\phi函数插入到基本块内。

\phi函数对应的数据结构为 **ssa.h** 中的 **phiFun** 类。对于形如 

```c++
class phiFun {
	// phi函数的例子：y^3 = \phi(y^1, y^2)
public:
	std::string primaryName;	// 初始命名为y，之后不做修改
	std::string name;			// 初始命名为y，后面更改为y^3
	std::vector<int> blockNums;		// y^1, y^2所在的基本块序号
	std::vector<std::string> subIndexs;	// y^1, y^2
}
```

在该步骤，或者说该函数内，我们在每个基本块添加了需要的\phi函数，我们能确定的是`primaryName`、`name`和`blockNums`成员变量，但`subIndexs`为空，这个需要等到变量重命名结束后才能确定。

添加\phi函数的位置保存在 **basicBlock** 类中的 **phi** 成员变量中，通过vector形式将所有的\phi函数组织起来。

注意：**最终要用到的中间代码不用理会这里的\phi函数，**因为我们后续会对\phi函数进行处理，即从当前的中间代码中去除\phi函数，处理方式后面详述。在访问SSA形式的IR时只需要正常遍历基本块内的Ir即可。

##### 12. 变量重命名 renameVar()

变量重命名的形式：将形如 `%a` 的变量变为 `%a^[数字]` 的形式。

变量重命名即实现SSA所谓的单一赋值，实现算法三本参考书中都没有阐述，参考网上资料，最终我的实现算法如下：

1. 由于\phi函数位于每个基本块的起始位置（严格规定：一个基本块的\phi函数前不允许有非\phi函数语句），因此我们首先处理\phi函数。将\phi函数的name域进行重命名，即将def域重命名。
2. 接下来遍历基本块内的中间代码，对def和use分别重命名，重命名算法为：
   1. def变量按照 `%a^[数字]` 的形式按照 [数字]/下标 递增生成。
   2. use变量按照之前定义过的最后一个该变量进行**替换**。寻找之前定义的变量方法为首先看该基本块内有没有定义过（包括\phi函数定义），如果没有说明该变量到达该基本块一定由确定的数据流到达（否则如果不唯一的话一定会在该基本块插入\phi函数），因此遍历它的前驱节点找到定义即可。
3. 各个基本块都完成重命名之后，最后更新\phi函数的 `subIndex` 域，即确定 \phi 函数的use域。

##### 13. 处理phi函数 deal_phi_function()

处理phi函数可能会遇到 lost of copy 和 swap problem 两个问题，详见

[知乎 Phi node 是如何实现它的功能的]: https://www.zhihu.com/question/24992774	"知乎 Phi node 是如何实现它的功能的"

但在我思考下觉得按照目前的处理和组织形式不会出现以下两个问题，因此我对phi函数的处理方法正如上面链接 R大 的回答。

举例来说，如果在一个基本块B2的开头有：

```c
x2 = phi(x0, x1)
```

那么最简单的resolution做法就是：假如x0在基本块B0定义，x1在基本块B1定义，并且假如x0分配到了R1，x1分配到了R2，x2分配到了R3，那么在B0的末尾会生成：

```c
move R3 <- R1
```

而在B1末尾则会生成：

```c
move R3 <- R2
```

这样后面R3就得到正确的x2的值了。

不可避免地，转换为SSA形式的IR会带来额外的赋值开销，这点是不可避免的，在官方文档和所有相关资料上对这点都承认不讳，仍然采用SSA的原因是它可以带来更好地优化，权衡之下SSA具有更大的优势。

##### 14. 将SSA恢复正常中间代码 rename_back()

该函数用于在优化完成后将SSA形式的Ir恢复为正常可交由后端处理的Ir。

##### 15. 测试文件 ssa_test.cpp

测试文件包含对各个实现函数的测试功能，将输出测试结果输出到 debug_ssa.txt 文件中，函数入口在 **ssa.cpp** 的 **generate()** 函数中的 **Test_SSA()** 函数进行调用。可以自由地开关各个测试，每个函数都有清晰的注释。

```c++
void SSA::Test_SSA() {
	// 打开文件，将测试输出信息输出到该文件
	debug_ssa.open("debug_ssa.txt");
	/* Test_* 函数用于对每个函数进行测试，输出相关信息 */
	Test_Divide_Basic_Block();// 测试函数：输出所有的基本块信息
	Test_Build_Dom_Tree();// 测试函数：输出构建的必经节点
	Test_Build_Idom_Tree();// 测试函数：输出直接必经节点
	Test_Build_Reverse_Idom_Tree();// 测试函数：输出反向必经节点
	// Test_Build_Post_Order();// 测试函数：输出后序遍历序列
	// Test_Build_Pre_Order();// 测试函数：输出前序遍历序列
	Test_Build_Def_Use_Chain();// 测试函数：输出基本块的def-use链
	// Test_Active_Var_Analyse();// 测试函数：输出基本块的in-out信息
	Test_Build_Dom_Frontier();// 测试函数：输出必经边界
	Test_Build_Var_Chain();// 测试函数：输出函数变量的迭代必经边界
	Test_Add_Phi_Fun();// 测试函数，输出添加\phi函数的信息
	// 关闭文件
	debug_ssa.close();
}
```

参考资料：

《高级编译器设计与实现》

https://www.zhihu.com/question/24992774

http://llvm.org/docs/LangRef.html#urem-instruction

https://llvm.zcopy.site/docs/langref/

https://www.zhihu.com/question/33518780

http://blog.sina.com.cn/s/blog_7e4ac8b501018vkb.html

https://blog.csdn.net/dashuniuniu/article/details/103389157

https://blog.csdn.net/u014713475/article/details/78224433

------

​																													刘泽华 2020.7.14

------
### 优化部分

#### 第一版优化 pre_optimize()

针对睿轩生成的第一版中间代码做的部分优化

##### 优化1 简化条件为常值的跳转指令 simplify_br()

key：将有条件跳转转化为无条件跳转

e.g. `br label2 0 label1` --> `br label2`

e.g. `br label2 1 label1` --> `br label1`

##### 优化2 简化load和store指令相邻的指令 load_and_store()

e.g. `a = a;`

```
 load       %0         %a
 store      %0         %a
```

##### 优化3 简化加减0、乘除模1这样的指令 simplify_add_minus_multi_div_mod()

key：如果前一个是load指令，则直接将load进该指令即可。

e.g. `b = a + 0;`

```
load       %1         %a                   
add        %2         %1         0         
store      %2         %b                   
```

```
load       %2         %a       
store      %2         %b               
```

##### 优化4 简化紧邻的跳转 simplify_br_label()

key：如果一个br无条件跳转指令后面是要跳转到的标签，则可以删除该跳转指令

e.g. 

```
br                    %if.end_0            
label      %if.end_0 
```

#### 第二版优化 ssa_optimize()

**计算def-use及之后的活跃变量分析时加入中间变量**

##### 优化1 死代码删除 delete_dead_codes()

预备知识：out集合表示在该基本块结束后仍然活跃的变量

实现算法：

* 初始化tmpout集合为out集合
* 从后向前遍历基本块内的中间代码，对每条语句进行判断，若该语句中的变量在tmpout集合中没有，说明该语句之后并没有用到该变量，则可以将该语句删除；否则，应将该语句中的定义和使用的各个变量加入到tmpout集合内。

