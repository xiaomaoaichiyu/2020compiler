

# 编译器设计

### IR设计

全局变量前面是@

局部变量前面是%

临时变量前面是%[0-9]，单个函数内部临时变量编号递增，不同函数之间编号可重新清零。

**注**：

- 为了方便 `def` 和 `use` 的计算，这里把所有的指令都设计为 `result` 存放的是被定义的变量（局部变量，临时变量）
- `operand1` 和 `operand2` 都设计为被使用的变量（局部变量、临时变量）
- 变量定义、参数定义、函数定义不遵循这一规则
- 注意 **`br` 指令**的修改
- $\color{red}红色$的指令表示只会在 `LIR` 出现



| 语义                       | op                  | result                  | operand1            | operand2                  |
| -------------------------- | ------------------- | ----------------------- | ------------------- | ------------------------- |
| 加法                       | add                 | res                     | ope1                | ope2                      |
| 减法                       | sub                 | res                     | ope1                | ope2                      |
| 除法                       | div                 | res                     | ope1                | ope2                      |
| 乘法                       | mul                 | res                     | ope1                | ope2                      |
| 取余                       | rem                 | res                     | ope1                | ope2                      |
| 逻辑与                     | and                 | res                     | ope1                | ope2                      |
| 不成立跳转label            | $\color{blue}and$   | label                   | ope1                | ope2                      |
| 逻辑或                     | or                  | res                     | ope1                | ope2                      |
| 不成立跳转label            | $\color{blue}or$    | label                   | ope1                | ope2                      |
| 逻辑非                     | not                 | res                     | ope1                | ope2                      |
| 不成立跳转label            | $\color{blue}not$   | label                   | ope1                |                           |
| 关系等于                   | eql                 | res                     | ope1                | ope2                      |
| 成立跳转(**eql**->**neq**) | $\color{blue}neq$   | label                   | ope1                | ope2                      |
| 关系不等                   | neq                 | res                     | ope1                | ope2                      |
| 成立跳转(**neq**->**eql**) | $\color{blue}eql $  | label                   | ope1                | ope2                      |
| 关系大于                   | sgt                 | res                     | ope1                | ope2                      |
| 成立跳转(**sgt**->**sle**) | $\color{blue}sle$   | label                   | ope1                | ope2                      |
| 关系大于等于               | sge                 | res                     | ope1                | ope2                      |
| 成立跳转(**sge**->**slt**) | $\color{blue}slt$   | label                   | ope1                | ope2                      |
| 关系小于                   | slt                 | res                     | ope1                | ope2                      |
| 成立跳转(**slt**->**sge**) | $\color{blue}sge$   | label                   | ope1                | ope2                      |
| 关系小于等于               | sle                 | res                     | ope1                | ope2                      |
| 成立跳转(**sle**->**sgt**) | $\color{blue}sgt$   | label                   | ope1                | ope2                      |
| **赋值单值**               | store               | value ➡                 | name                |                           |
| **存VR**                   | $\color{blue}store$ | tmpReg ➡                | VR[0-9]             | offset                    |
| **赋值数组**               | storeArr            | value ➡                 | address(暂时用name) | offset                    |
| **取内存**                 | load                | tmpReg ⬅                | name                |                           |
| **取VR**                   | $\color{blue}load$  | tmpReg ⬅                | VR[0-9]             | offset                    |
| **取数组参数的地址**       | load                | tmpReg ⬅                | 数组（参数）        | "para" \| "array"         |
| **取内存**                 | loadArr             | tmpReg ⬅                | address(暂时用name) | offset                    |
| 加载地址                   | $\color{red} lea$   |                         | tmpReg              | name(全局变量或全局数组)  |
| 移动                       | $\color{red}mov$    |                         | dst                 | src                       |
| 有返回值函数调用(变了)     | call                | retReg                  | funcName            | paraNum                   |
| 无返回值函数调用(变了)     | call                | void                    | funcName            | paraNum                   |
| 函数返回(变了)             | ret                 |                         | retValue            | int\|void                 |
| 函数传参(变了)             | push                | type(int\|int*\|string) | tmpReg              | num(第几个)               |
| 压栈                       | push                |                         | reg物理寄存器       | vReg(reg存储的虚拟寄存器) |
| 退栈                       | pop                 |                         |                     | offset                    |
| 退栈                       | pop ​                |                         | tmpReg              |                           |
| 标签                       | label               | name                    |                     |                           |
| 直接跳转(变了)             | br                  |                         | label               |                           |
| 条件跳转(变了)             | br                  | lable2(错误)            | tmpReg              | label1(正确)              |
| 取返回值                   | $\color{red}getRet$ |                         | tmpReg              |                           |
| 取返回值                   | $\color{red}getReg$ |                         |                     |                           |
|                            |                     |                         |                     |                           |
| 局部数组初始化             | arrayinit           | 0                       | name                | size                      |
| 函数定义                   | define              | name                    | funcType            |                           |
| 函数形参                   | para                | name                    | paraType(int\|int*) | dimension(维度)           |
| 局部常量\|变量             | alloc               | variableName            | value               | size                      |
| 全局变量\|常量             | global              | variableName            | value               | size                      |
| 注释                       | note                |                         | 注释内容            |                           |
| 函数参数压栈               | note                | funcName                | "func"              | begin \| end              |
| 数组索引计算               | note                | arrayName               | "array"             | begin \| end              |
| 注释                       | note                | 注释内容                | "note"              |                           |
|                            |                     |                         |                     |                           |
|                            |                     |                         |                     |                           |
|                            |                     |                         |                     |                           |
|                            |                     |                         |                     |                           |
|                            |                     |                         |                     |                           |
|                            |                     |                         |                     |                           |
|                            |                     |                         |                     |                           |
|                            |                     |                         |                     |                           |
|                            |                     |                         |                     |                           |

*****



### MIR2LIR

将IR转换围殴LIR（lower IR），接近底层的IR，在进行寄存器分配，之后转换为ARM汇编



### 由LIR到ARM汇编的选择

> **下面的规则是由LIR生成ARM的规则，上面是由前端生成IR的规则**

1. add、sub、div、mul、and、or、not、eql、neq、sgt、sge、slt、sle：**2-3个寄存器**

   - 第二个操作数为立即数

   - 第二个操作数为寄存器

     > 第一个操作数不会是立即数

2. lea：**R0**（以R0为例）

   - 把变量的地址加载到寄存器

3. load：**1-2个寄存器**

   - 变量是虚拟寄存器

     ```assembly
     load R0 VR1 offset
     ;加载栈中的临时变量，offset是从临时变量位置开始的偏移
     ```

   - 变量是单值变量，转换为两条指令

     ```assembly
     ;1. 全局变量，ope1字段一定是一个搞寄存器，但是具体是哪一个不确定，可能和存数据的是同一个
     ; load R0, R1
     LDR R0, [R1]
     或 LDR R0, [R0]
     ;2. 栈变量，ope1字段是变量名，生成的时候通过栈来转换为偏移
     ; load R0, 变量名
     LDR R0, [SP, #偏移]
     ```

4. loadarr：**1-2个寄存器**

   - 偏移是立即数

     ```assembly
     ;1. 全局数组 LOAD R0, R0, 8
     LDR R0, [R0 #8] ;这里前面的指令R0已经已经加在了数组的地址
     ;2. 栈数组 LOAD R0, 变量名, 8
     LDR R0, [SP #8 + (数组相对于sp的偏移)] ;立即数可以在编译期间计算得出
     ```

     > 这里秉承这样一个思想，全局数组的loadarr指令，会在**前面加一条lea指令专门加载地址**，所以直接按照上述的转换即可！

   - 偏移是一个寄存器

     ```assembly
     ;1. 全局数组 LOAD R0, R1, R2，这里极限情况是R0和R1是同一个寄存器或者R0和R2是同一个寄存器
     LDR R0, [R1, R2] ;这里前面的指令已经在R1中加载了数组的地址
     
     ;2. 栈数组 LOAD R0, 变量名, R1
     add R1, sp, R1
     add R1, R1, #数组相对于SP的偏移
     ;这里是否有指令可以一部计算出来：add R1, R1, [sp, #数组偏移]这种？？
     LDR R0, [R1]
     ```

5. store：**R0**

   - 变量是单值变量

     ```assembly
     ;1. 全局变量，ope1字段是寄存器，R0和R1一定是不同的寄存器
     ; store R0, R1
     SDR R0, [R1]
     ;2. 栈变量，ope1字段是变量名，生成的时候通过栈来转换为偏移
     ; store R0, 变量名
     SDR R0, [SP, #变脸偏移]
     ```

6. storearr：**2-3个寄存器**

   - 偏移是立即数

     ```assembly
     ;1. 全局数组， STORE R0, R1, 8
     SDR R0, [R1 #8]
     ;2. 栈数组，STORE R0, 变量名， 8
     SDR R0, [sp, #8+(数组相对于sp的偏移)]
     ```

   - 偏移是一个寄存器

     ```assembly
     ;1. 全局数组, STORE R0, R1, R2
     SDR R0, [R1, R2]
     ;2. 栈数组，STORE R0, 变量名，R1
     add R1, sp, R1
     add R1, R1, #数组相对于SP的偏移
     SDR R0, [R1]
     ```

7. call ：**R0**

   - call指令回来后返回值在R0，需要把R0的值移动到一个新的寄存器中，这个寄存器在**res**字段指定，即call指令不需要处理res字段，在call指令的下一条是一个**MOV**指令，指明将**R0**的值转换到那个寄存器

8. ret ：**R0**

   - 中间代码会加一条**MOV**，把返回值移动到R0寄存器，**故直接返回即可！无需处理R0的问题**

9. push 和 pop：

   - 保证中间代码的ope1字段都是寄存器

10. br：

    - 保证中间代码的ope1字段都是寄存器

11. 其余的正常翻译即可





> LIR

### 地址操作相关

#### 参数数组的使用

- 分配了寄存器，数组地址的加载为 `MOV R0, R4`
- 在栈里，数组地址的加载为 `load R0, name, "para"`

#### 局部数组的使用

- 采用 `LEA    R0, name` 来加载数组地址





****

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

