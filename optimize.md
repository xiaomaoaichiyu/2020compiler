# 优化





### 寄存器分配

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

[寄存器使用](<http://blog.chinaunix.net/uid-69947851-id-5825875.html>)

#### on-the-fly





### 改变顺序优化

- arr[i] = getint()：这里可以把数组的索引计算放在getint的后面，就会少使用几个寄存器，保存上下文也相对减轻了负担



![1593348158143](C:\Users\legend\AppData\Roaming\Typora\typora-user-images\1593348158143.png)



### 不同模式下寄存器的使用情况

![1593351432585](C:\Users\legend\AppData\Roaming\Typora\typora-user-images\1593351432585.png)



#### SSA的$\phi$函数

> [Rice University](https://zh.wikipedia.org/w/index.php?title=Rice_University&action=edit&redlink=1)的Keith D. Cooper、Timothy J. Harvey及 Ken Kennedy在他们的文章*A Simple, Fast Dominance Algorithm*.[[2\]](https://zh.wikipedia.org/wiki/%E9%9D%99%E6%80%81%E5%8D%95%E8%B5%8B%E5%80%BC%E5%BD%A2%E5%BC%8F#cite_note-Cooper_2001-2)中所描述，这个算法使用了精心设计的数据结构来改进效能。

![1593619071719](C:\Users\legend\AppData\Roaming\Typora\typora-user-images\1593619071719.png)

#### 线性扫描

![1593765755972](C:\Users\legend\AppData\Roaming\Typora\typora-user-images\1593765755972.png)

![1593766578443](C:\Users\legend\AppData\Roaming\Typora\typora-user-images\1593766578443.png)





### 到达定义分析 & 活跃变量分析

#### 到达定义：

> 这个用来做什么

#### 活跃变量分析：

> 寄存器分配