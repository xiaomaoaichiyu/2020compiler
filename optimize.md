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





### 改变顺序优化

- arr[i] = getint()：这里可以把数组的索引计算放在getint的后面，就会少使用几个寄存器，保存上下文也相对减轻了负担