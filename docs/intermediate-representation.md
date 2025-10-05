# 中间表示

编译器生成的中间代码为三地址代码(three-address code)，以四元式形式表示，即：（运算符，左操作数，右操作数，结果）。

## 四元式

四元式操作，原则上均按照“x = y op z”形式的中缀表达式进行表达。

### 函数声明

源码形如：

`int foo( int a, int b, int c, int d)`

中间代码：

```
int foo()
para int a
para int b
para int c
para int d
```

### 函数调用

源码形如：

 `i = tar(x,y)`

中间代码：

```
push x
push y
call tar
i = RET
```

### 函数返回

源码形如：

`return (x)`

中间代码：

`ret x`

### 变量声明

源码形如：

`int i, j;`

中间代码（符号表信息输出，程序中可不生成真正的中间代码）：

```
var int i
var int j
```

### 常数声明

源码形如：

 `const int c = 10`

中间代码（符号表信息输出，程序中可不生成真正的中间代码）：

 `const int c = 10`

### 表达式

源码形如：

`x = a * (b + c)`

中间代码（可优化）：
```
t1 = b + c
t2 = a * t1
x = t2
```
### 条件判断

源码形如：

`x == y`

中间代码：

`x == y`

### 条件或无条件跳转

中间代码：

```
GOTO LABEL1 //无条件跳转到LABEL1
BNZ LABEL1 //满足条件跳转到LABEL1
BZ LABEL1 //不满足条件跳转到LABEL1
```

### 带标号语句

中间代码：
```
Label_1 :
x = a + b
```

### 数组赋值或取值

源码形如：

`a[i] = b * c[j]`

中间代码：

```
t1 = c[j]
t2 = b * t1
a[i] = t2
```
