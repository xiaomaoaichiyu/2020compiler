1. 加减乘除、取余、赋值、取值   
    // 使用变量或者常数都需要先加载到一个寄存器中，这里我们认为虚拟寄存器（用于中间代码），所以一定要先load到一个寄存器；~~我印象里精简指令集都是要在寄存器中进行运算的，无法直接和内存运算。~~

    ```assembly
    a = b * (c + d) % e;
    
    ;四元式的中间代码store和load的操作数operand2是相对于变量的偏移，数组变量有用，非数组变量置为0
    load %0 %c 0
    load %1 %d 0
    add %2 %0 %1
    load %3 %b 0
    mul %4 %3 %2
    load %5 %e 0
    rem %6 %4 %5
    store %6 %a 0
    ```

2. 函数声明
    // 参数声明时保持参数原名就可以，LLVM都换成了临时变量，这点不用改了
    // 关于这里的四元式是新增para的定义还是采用alloc待定，暂时分开，后续考虑合并
    // 不会处理多维数组的传参问题，这个暂时传递一个名字，和类型

    ```assembly
    int foo(int a, int b[], int c[][3]);
    
    define @foo int ; 函数名与全局同处理，即函数名前面加@
    ; alloc写法
    alloc %a 4
    alloc %b ?      ; 如果是alloc的写法第3个size字段应该是什么呢？
    alloc %c ?
    ; para写法
    para %a int
    para %b int*    
    para %c int*    ;关于多维数组传参有问题，没有好的解决思路
    ;这里定义的时候应该不需要大小，因为无法确定，传参时这么考虑，我也不太明白
    ```

3. 函数调用

    ```assembly
    i = foo(x, y, z)
    
    load %0, %x
    push %0
    load %1, %y
    push %1
    load %2, %z
    push %2
    call @foo %3 3
    ;函数在返回时（ret）会进行堆栈维护，这里不需要退栈
    store %3 %i 0
    ```

4. 局部变量声明 | 常量申明

    ```assembly
    int x, y;
    int arr[3];
    const int a = 1;
    const int b[2] = {1,2};
    ;局部常量可以直接在编译过程中处理掉，语法分析阶段直接对其进行替换，没有必要存储，或者统一？或者就是和全局的一样
    
    alloc %x _ 1	;这里的size规定为分配空间大小
    alloc %y _ 1  	;初始值未知所以置为_，因为局部变量的初始化可以认为是赋值，所以可以统一为这种形式
    alloc %arr _ 3
    
    alloc %a 1 1
    alloc %b _ 2
    ```

5. 全局变量声明
    // 全局变量的数组信息维护在符号表？

    // 初始值的信息肯定是维护在符号表的

    ```assembly
    int x, y;
    int arr[3];
    
    const int a = 1;
    const int b[2] = {};
    
    global @x 0 4
    global @y 0 4
    global @arr _ array
    
    global @a 1 1
    global @b _ 2
    ```

6. 跳转指令
    // label标签的编号与处理局部变量重名加编号原则相同，在标签前加%表示局部，标签后加后缀区分同一函数内部的if、else标签

    ```assembly
    if (a && (b == 3)) {
        ...
    }
    else {
    	...  
    }
    
    eql %0 %b 3
    and %1 %a %0
    br  %1 %if.then_1 %if.else_1  ;有条件跳转
    labe %if.then_1
    ...
    br %if.end_1   ; 无条件跳转
    
    label %if.else_1
    ...
    br %if.end_1	; 无条件跳转
    
    label %if.end_1
    ```

7. 循环指令

    ```assembly
    while((a + b) > 3) {
        ...
    }
    
    label %while.cond_1
       add %0 %a %b
       sgt %1 %0 3
       br %1 %while.body_1 %while.end_1   
    
    label %while.body_1
       ...
       br while.cond_1
    
    label while.end_1
       // 若考虑上学期编译课程的优化，可将while变成do while形式减少跳转指令
    ```





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

      