编译器设计

****

### [LLVM](<https://zhuanlan.zhihu.com/p/100241322>)

1. > 除了被实现为一种语言，LLVM IR 实际上有三种等价形式：上面的文本格式、优化器自身检查和修改的内存数据结构形式，以及高效磁盘二进制“位码”形式。LLVM 项目还提供了将磁盘格式的文本转换为二进制的工具：`llvm-as`命令 将 `.ll`文本文件转成以`.bc`作为后缀的二进制流文件，`llvm-dis`命令将 `.bc` 文件转成`.ll`文件。

2. > 每个 LLVM 优化程序都独立出一个类，都继承自`Pass`父类。大多数优化程序都独立出`.cpp`文件，并且`Pass`的子类都被定义在匿名命名空间中（这使其完全对定义文件私有）。为了使用优化程序，文件之外的代码需要能引用到它，因此会在文件编写一个用于创建优化程序的类导出函数。

*****

### 前端->IR

全局常量：

- const ：保存在符号表，使用的时候通过符号表差值
- constArr ：需要开全局空间保存

全局变量

- var ：开空间 global
- varArr ：开空间 global [i1] [i2] [i3]......

#### 函数内部

1. 局部变量
   - var
   - varArr

2. 局部常量
   - const
   - constArr

3. Expr
   - add : +
   - sub : -
   - div : /
   - mul : +
   - rem : %

4. logic
   - and : &&
   - or : ||
   - not : !

5. 关系比较 (icmp) 

   - eq : ==

   - ne : !=
   - sgt : >
   - sge : >=
   - slt : <
   - sle : <=

6. if、if-else

   ```
   if-else
   //通过关系比较和br来进行转换
   entry:
   	res = icmp (关系比较) ope1, ope2
   	br res %if.then, %if.else
   if.then: //preds = %entry
   	...
   	br %if.end
   if.else: //preds = %entry
   	...
   	br %if.end
   if.end: //preds = %if.then, %if.else
   	...
   	
   if(no else)
   entry:
   	res = icmp (关系比较) ope1, ope2
   	br res %if.then, %if.end
   if.then: //preds = %entry
   	...
   	br %if.end
   if.end: //preds = %if.then, %if.else
   	...
   ```

7. while

   ```
   //通过关系比较和br来进行转换
   entry:
   	...
   	br %while.cond
   
   while.cond: //preds = %entry, %while.body
   	...
   	res = icmp (关系比较) ope1, ope2
   	br res %while.body, %while.end
   	
   while.body: //preds = %cond
   	...
   	br %while.cond
   	
   while.end:  //preds = %cond
   	...
   ```

8. ret

   - ret

9. 函数调用

   - call

10. break

    - br label

11. continue

    - br label

12. 赋值

    - 数组 storeArr
    - 单值 store

13. 加载内存值

    - 数组 loadArr
    - 单值 load

14. 注释

    - 函数调用

      ```cpp
      //call func "函数名字" begin
      push arg1
      push arg2
      call f
      //call func "函数名字" end
      ```

    - 数组索引计算

      ```cpp
      a[2][2];
      i = 1;
      求 a[i][i]
      //index count begin
      load       %0         %i        
      mul        %1         %0         8         
      add        %2         0          %1        
      load       %3         %i        
      mul        %4         %3         4         
      add        %5         %2         %4               
      //index count end
      loadArr    %6 		  @a         %5
      ```

      

***

### IR设计

全局变量前面是@

局部变量前面是%

临时变量前面是%[0-9]，单个函数内部临时变量编号递增，不同函数之间编号可重新清零。

**注**：

- 为了方便 `def` 和 `use` 的计算，这里把所有的指令都设计为 `result` 存放的是被定义的变量（局部变量，临时变量）
- `operand1` 和 `operand2` 都设计为被使用的变量（局部变量、临时变量）
- 变量定义、参数定义、函数定义不遵循这一规则
- 注意 **`br` 指令**的修改



| 语义                   | op       | result (define)                    | operand1 (use)      | operand2 (use) |
| ---------------------- | -------- | ---------------------------------- | ------------------- | -------------- |
| 加法                   | add      | res                                | ope1                | ope2           |
| 减法                   | sub      | res                                | ope1                | ope2           |
| 除法                   | div      | res                                | ope1                | ope2           |
| 乘法                   | mul      | res                                | ope1                | ope2           |
| 取余                   | rem      | res                                | ope1                | ope2           |
| 逻辑与                 | and      | res                                | ope1                | ope2           |
| 逻辑或                 | or       | res                                | ope1                | ope2           |
| 逻辑非                 | not      | res                                | ope1                | ope2           |
| 关系等于               | eql      | res                                | ope1                | ope2           |
| 关系不等               | neq      | res                                | ope1                | ope2           |
| 关系大于(signed)       | sgt      | res                                | ope1                | ope2           |
| 关系大于等于(signed)   | sge      | res                                | ope1                | ope2           |
| 关系小于(signed)       | slt      | res                                | ope1                | ope2           |
| 关系小于等于(signed)   | sle      | res                                | ope1                | ope2           |
| 赋值单值（变了）       | store    | value ➡                            | name                |                |
| 赋值数组（变了）       | storeArr | value ➡                            | address(暂时用name) | offset         |
| 取内存 （变了）        | load     | tmpReg ⬅                           | name                |                |
| 取内存 （变了）        | loadArr  | tmpReg ⬅                           | address(暂时用name) | offset         |
| 移动                   | mov      |                                    | src                 | dst            |
| 有返回值函数调用(变了) | call     | retReg(寄存器分配时指定为R0寄存器) | funcName            | paraNum        |
| 无返回值函数调用(变了) | call     | void                               | funcName            | paraNum        |
| 函数返回(变了)         | ret      |                                    | retValue            | int\|void      |
| 函数传参(变了)         | push     | type(int\|int*\|string)            | tmpReg              | num(第几个)    |
| 退栈                   | pop      | type(int\|int*)                    | tmpReg              |                |
| 标签                   | label    | name                               |                     |                |
| 直接跳转(变了)         | br       |                                    | label               |                |
| 条件跳转(变了)         | br       | lable2(错误)                       | tmpReg              | label1(正确)   |
|                        |          |                                    |                     |                |
| 函数定义               | define   | name                               | funcType            |                |
| 函数形参               | para     | name                               | paraType(int\|int*) |                |
| 局部常量\|变量         | alloc    | variableName                       | value               | size           |
| 全局变量\|常量         | global   | variableName                       | value               | size           |
| 注释                   | note     |                                    | 注释内容            |                |
|                        |          |                                    |                     |                |

*****

### 错误处理

- 词法语法报错
- 中间代码生成报错
- 优化过程报错
  - 栈空间如果不够报错
- 目标代码生成报错

*****

### 优化

- 循环分布 —— 循环比较大，次数比较少 p5
- 高级中间代码——数据缓存优化，循环的层次替换
- 尾递归
- 栈空间申请合并以及初始化的操作
- 非具名临时变量是按顺序编号的（使用一个递增计数器，从0开始）。注意整个基本块都被包含在这种编号方法中。例如，如果一个基本块的入口没有被给予一个标签名，那么它就会获得一个编号0。
