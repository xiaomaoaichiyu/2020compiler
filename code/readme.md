# 一、代码组成说明

Syntax.cpp 相当于是main.cpp，是整个代码main函数的入口

​	功能：翻译文法并生成符号表和中间代码

word.cpp和word.h

​	功能：词法分析相关内容，主要用于拆分单词

symboltable.cpp和symboltable.h

​	功能：符号表相关

intermedicatecode.cpp和intermedicatecode.h

​	功能：中间代码相关

# 二、代码使用说明

目前检查中间代码准确性，只需要在同级目录下建立一个testexample.txt文件(见Syntax.cpp98行)，里面放上SYS文法对应代码，运行，最后输出框即为中间代码

# 三、符号表和中间代码组成说明

符号表和中间代码采用的都是vector嵌套模型，

vector<vector<symbolTable>> total;	//总符号表

vector<vector<CodeItem>> codetotal;   //总中间代码

其中total[0]和codetotal[0]对应的vector是全局作用域符号表和中间代码

剩下一个函数占一个下标，total[i]和codetotal[i]对应的vector是该函数作用域符号表和中间代码