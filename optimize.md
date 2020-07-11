# 优化	

## 1. 寄存器分配

ARM寄存器使用规范

| 寄存器 | 代名词 | 专用 |               解决                |
| :----: | :----: | :--: | :-------------------------------: |
| r0-r3  | a0-a4  |  -   | Argument/result/scratch registers |
| r4-r11 | v1-v8  |      |         Variable register         |
|  r12   |   -    |  ip  | Intra-proc-call scratch register  |
|  r13   |   -    |  sp  |           Stack Pointer           |
|  r14   |   -    |  lr  |           Link Register           |
|  r15   |   -    |  pc  |          Program Counter          |

r0-r3：临时寄存器，参数寄存器，返回值寄存器

r4-r11：全局寄存器

r12：可以用作临时寄存器

r13：sp寄存器

r14：函数调用返回地址

r15：PC

参考

> [寄存器使用](<http://blog.chinaunix.net/uid-69947851-id-5825875.html>)

***

### 1. 寄存器指派（基础版寄存器分配）

##### MIR2LIRpass

1. 将所有的操作数都放在**虚拟寄存器**：VR[0-9]*，编号从0开始
2. 主要是将**%[数字]替换为虚拟寄存器**





****

#### 问题

- 变量a一开始直接用寄存器，还是把值传到内存的位置？
- 这个stack slot的插入位置怎么搞呢？

#### 思路

- 将 `LOAD` 转换为 `MOV`
- **寄存器分配后消除一下 `MOV`，对于MOV的第一个操作数是只会被使用一次，所以可以找到并且替换掉**
- load分两种情况考虑：
  - 加载的变量已经有寄存器，$\color{red}直接把load的要加载的寄存器赋值为已有变量的寄存器$。
  - 否则正常的加载，并且分配寄存器

***

> 全局变量和数组元素遵循写回策略和每次使用load策略，不分配寄存器
>
> $\color{red}临时变量分配给r0 - r3，局部变量分配给r4 - r11​$？？这一点待定，暂时设计为无脑分配r0-r11



> Second chance binpacking 和 线性扫描



##### 线性化

- 连续的跳转的删除
- 分支块尽量在一块，比如 if 和 else
- 循环的块连续

##### block order

- 循环检测 —— loop_index 和 loop_depth

- 基于循环检测的loop_depth和一个栈的**final block orders**

- 指令标号 每次+2

##### 生存周期 lifetime intervals

一个虚拟寄存器可能对应多个生存周期

**usePosition**：在没有寄存器的时候用来判断分割并且溢出哪一个 interval，以及一个被溢出的 interval在什么时候被重新加载到寄存器







****

### 改变顺序优化

- arr[i] = getint()：这里可以把数组的索引计算放在getint的后面，就会少使用几个寄存器，保存上下文也相对减轻了负担

![1593348158143](C:\Users\legend\AppData\Roaming\Typora\typora-user-images\1593348158143.png)



### 不同模式下寄存器的使用情况

![1593351432585](C:\Users\legend\AppData\Roaming\Typora\typora-user-images\1593351432585.png)



#### SSA的$\phi$函数

> [Rice University](https://zh.wikipedia.org/w/index.php?title=Rice_University&action=edit&redlink=1)的Keith D. Cooper、Timothy J. Harvey及 Ken Kennedy在他们的文章*A Simple, Fast Dominance Algorithm*.[[2\]](https://zh.wikipedia.org/wiki/%E9%9D%99%E6%80%81%E5%8D%95%E8%B5%8B%E5%80%BC%E5%BD%A2%E5%BC%8F#cite_note-Cooper_2001-2)中所描述，这个算法使用了精心设计的数据结构来改进效能。

![1593619071719](C:\Users\legend\AppData\Roaming\Typora\typora-user-images\1593619071719.png)

#### 线性扫描

![1593766578443](C:\Users\legend\AppData\Roaming\Typora\typora-user-images\1593766578443.png)





### 到达定义分析 & 活跃变量分析

#### 到达定义：

> 这个用来做什么

#### 活跃变量分析：

> 寄存器分配	


