# cliblisp（C++ 简易LISP解释器 + 通用LR语法分析）

借鉴[CMiniLang](https://github.com/bajdcc/CMiniLang)的部分代码。

主要借鉴了CMiniLang的类型系统、词法分析、语法分析、AST、内存管理等代码（均为原创）。

事实证明CMiniLang的框架还是非常经典耐用的（再次强调）。

**语法分析采用LR分析。项目见：[clibparser](https://github.com/bajdcc/clibparser)。**

- 文法书写方式：以C++重载为基础的Parser Generator。
- 识别方式：**以下推自动机为基础，向看查看一个字符、带回溯的LR分析**。
- 内存管理：自制内存池。

本说明完善中，**末尾有测试用例**。

注：经[Qlib2d](https://github.com/bajdcc/Qlib2d)项目测试，本项目于**x64**环境下也可编译成功。

## 截图

![image](https://raw.githubusercontent.com/bajdcc/cliblisp/master/screenshots/1.png)

## 文章

- [【Lisp系列】开篇](http://zhuanlan.zhihu.com/p/45897626)
- [【Lisp系列】实现四则运算](http://zhuanlan.zhihu.com/p/46723048)
- [【Lisp系列】实现GC](http://zhuanlan.zhihu.com/p/46993463)
- [【Lisp系列】实现Lambda](http://zhuanlan.zhihu.com/p/47309037)
- [【Lisp系列】大功告成](http://zhuanlan.zhihu.com/p/47569910)
- [【Lisp系列】手动递归](http://zhuanlan.zhihu.com/p/47869195)

- [【Parser系列】实现LR分析——开篇](https://zhuanlan.zhihu.com/p/52478414)
- [【Parser系列】实现LR分析——生成AST](https://zhuanlan.zhihu.com/p/52528516)
- [【Parser系列】实现LR分析——支持C语言文法](https://zhuanlan.zhihu.com/p/52812144)
- [【Parser系列】实现LR分析——完成编译器前端！](https://zhuanlan.zhihu.com/p/53070412)

## 功能

当前完成了四则运算和常用函数，采用解释器求值。

**运行时所有对象采用标识回收GC，采用不可变值，传递拷贝。**

已实现：引用，变量，函数，四则，比较，递归，闭包，if，测试用例。

已实现**Y-combinator**，见测试用例#47-#49，由于递归运算会大量消耗内存，因此必要时需更改cvm.h中的**VM_MEM**宏的值为更大值。

**改进：将eval调用转化为手动调归，使得递归可以人工控制，后续可能将出错机制从throw方式转变为手动调归跳出方式。测试：除大数溢出外，其余均通过。**

- [x] 词法分析
- [x] 语法分析
- [x] 内存管理
- [x] 序列化
- [x] 识别数字
- [x] 识别S-表达式
- [x] 识别Q-表达式
- [x] GC
- [x] 运行时环境
- [x] 异常恢复
- [x] 简单的内建四则运算
- [x] Subroutine和Symbol
- [x] nil
- [x] 常用内建函数
- [ ] 输入
- [x] 输出
- [x] 字符串处理
- [x] 识别变量，设置变量
- [x] 识别函数Lambda
- [x] 支持递归
- [ ] 完善控制流：if
- [ ] 更多测试用例
- [ ] 添加更多功能

内建函数：

- 四则运算
- 比较运算
- lambda
- eval
- quote
- list
- cons
- car
- cdr
- def
- if
- len
- append

## 调试信息

- cval结点内存申请情况
- GC释放情况
- 内存池结点情况
- GC中的栈对象引用树

生成NGA图，去EPSILON化，生成PDA表，生成AST。

以下为下推自动机的识别过程（**太长，略**），如需查看，请修改cparser.cpp中的：

```cpp
#define TRACE_PARSING 0
#define DUMP_PDA 0
#define DEBUG_AST 0
#define CHECK_AST 0
```

将值改为1即可。

## 使用

```cpp
int main(int argc, char *argv[]) {
    clib::cvm vm;
    std::string input;
    while (true) {
        std::cout << "lisp> ";
        std::getline(std::cin, input);
        if (input == "exit")
            break;
        if (input.empty())
            continue;
        try {
            vm.save();
            clib::cparser p(input);
            auto root = p.parse();
            //clib::cast::print(root, 0, std::cout);
            auto val = vm.run(root);
            clib::cvm::print(val, std::cout);
            std::cout << std::endl;
            vm.gc();
        } catch (const std::exception &e) {
            printf("RUNTIME ERROR: %s\n", e.what());
            vm.restore();
            vm.gc();
        }
    }
    return 0;
}
```

## 例子

```lisp
lisp> + 1 2
3
lisp> * 1 2 3 4 5 6
720
lisp> - 8 4 2 9 8 
-15
lisp> + a 5
+ a 5
COMPILER ERROR: unsupported calc op
RUNTIME ERROR: std::exception
lisp> + "Hello" " " "world!"
"Hello world!"
lisp> eval 5
5
lisp> eval `(+ 1 2)
3
lisp> eval (+ 1 2)
3
lisp> `a
`a
lisp> `(a b c)
`(a b c)
```

### Subroutine

```lisp
lisp> + "Project: " __project__ ", author: " __author__
"Project: cliblisp, author: bajdcc"
lisp> +
<subroutine>
```

### List

```lisp
lisp> list
<subroutine "list">
lisp> car (list 1 2 3)
1
lisp> cdr (list 1 2 3)
`(2 3)
lisp> (eval (car (list + - * /))) 1 1
2
```

### Builtin

```lisp
lisp> def `(a b c d) 1 2 3 4
nil
lisp> + a b c d
10
```

### Lambda

```lisp
lisp> def `a (\ `(x y) `(+ x y))
nil
lisp> a
<lambda `(x y) `(+ x y)>
lisp> a 1 2 3
6
```

### comparison

```lisp
lisp> def `a (\ `(x y) `(+ x y))
nil
lisp> == (a 1 2) (a 2 1)
1
lisp> < (a 1 2) (a 1 1)
0
```

### recursion

```lisp
lisp> def `sum (\ `(n) `(if (> n 0) `(+ n (sum (- n 1))) `0))
nil
lisp> sum 100
5050
lisp> sum (- 0 5)
0
lisp> def `len (\ `l `(if (== l nil) `0 `(+ 1 (len (cdr l)))))
nil
lisp> len (list 1 2 3 )
3
```

## 测试用例

在目录下的test.cpp中。

```cpp
TEST #1> [PASSED] (+ 1 2)  =>  3
TEST #2> [PASSED] (* 1 2 3 4 5 6)  =>  720
TEST #3> [PASSED] (- 8 4 2 9 8)  =>  -15
TEST #4> [PASSED] (+ "Hello" " " "world!")  =>  "Hello world!"
TEST #5> [PASSED] (eval 5)  =>  5
TEST #6> [PASSED] (eval `(+ 1 2))  =>  3
TEST #7> [PASSED] (eval (+ 1 2))  =>  3
TEST #8> [PASSED] `a  =>  `a
TEST #9> [PASSED] `(a b c)  =>  `(a b c)
TEST #10> [PASSED] (+ "Project: " __project__ ", author: " __author__)  =>  "Project: cliblisp, author: bajdcc"
TEST #11> [PASSED] +  =>  <subroutine "+">
TEST #12> [PASSED] (quote (testing 1 2 -3.14e+159))  =>  `(testing 1 2 -3.14e+159)
TEST #13> [PASSED] (+ 2 2)  =>  4
TEST #14> [PASSED] (+ (* 2 100) (* 1 10))  =>  210
TEST #15> [PASSED] (if (> 6 5) `(+ 1 1) `(+ 2 2))  =>  2
TEST #16> [PASSED] (if (< 6 5) `(+ 1 1) `(+ 2 2))  =>  4
TEST #17> [PASSED] (def `x 3)  =>  3
TEST #18> [PASSED] x  =>  3
TEST #19> [PASSED] (+ x x)  =>  6
TEST #20> [PASSED] (begin (def `x 1) (def `x (+ x 1)) (+ x 1))  =>  3
TEST #21> [PASSED] ((\ `x `(+ x x)) 5)  =>  10
TEST #22> [PASSED] (def `twice (\ `x `(* 2 x)))  =>  <lambda `x `(* 2 x)>
TEST #23> [PASSED] (twice 5)  =>  10
TEST #24> [PASSED] (def `compose (\ `(f g) `(\ `x `(f (g x)))))  =>  <lambda `(f g) `(\ `x `(f (g x)))>
TEST #25> [PASSED] ((compose list twice) 5)  =>  `10
TEST #26> [PASSED] (def `repeat (\ `f `(compose f f)))  =>  <lambda `f `(compose f f)>
TEST #27> [PASSED] ((repeat twice) 5)  =>  20
TEST #28> [PASSED] ((repeat (repeat twice)) 5)  =>  80
TEST #29> [PASSED] (def `fact (\ `n `(if (<= n 1) `1 `(* n (fact (- n 1))))))  =>  <lambda `n `(if (<= n 1) `1 `(* n (fa
ct (- n 1))))>
TEST #30> [PASSED] (fact 3)  =>  6
TEST #31> [ERROR ] (fact 50)  =>  0   REQUIRE: 30414093201713378043612608166064768844377641568960512000000000000
TEST #32> [PASSED] (fact 12)  =>  479001600
TEST #33> [PASSED] (def `abs (\ `n `((if (> n 0) `+ `-) 0 n)))  =>  <lambda `n `((if (> n 0) `+ `-) 0 n)>
TEST #34> [PASSED] (abs -3)  =>  3
TEST #35> [PASSED] (list (abs -3) (abs 0) (abs 3))  =>  `(3 0 3)
TEST #36> [PASSED] (def `combine (\ `f `(\ `(x y) `(if (null? x) `nil `(f (list (car x) (car y)) ((combine f) (cdr x) (c
dr y)))))))  =>  <lambda `f `(\ `(x y) `(if (null? x) `nil `(f (list (car x) (car y)) ((combine f) (cdr x) (cdr y)))))>
TEST #37> [PASSED] (def `zip (combine cons))  =>  <lambda `(x y) `(if (null? x) `nil `(f (list (car x) (car y)) ((combin
e f) (cdr x) (cdr y))))>
TEST #38> [PASSED] (zip (list 1 2 3 4) (list 5 6 7 8))  =>  `(`(1 5) `(2 6) `(3 7) `(4 8))
TEST #39> [PASSED] (def `riff-shuffle (\ `deck `(begin (def `take (\ `(n seq) `(if (<= n 0) `nil `(cons (car seq) (take
(- n 1) (cdr seq)))))) (def `drop (\ `(n seq) `(if (<= n 0) `seq `(drop (- n 1) (cdr seq))))) (def `mid (\ `seq `(/ (len
 seq) 2))) ((combine append) (take (mid deck) deck) (drop (mid deck) deck)))))  =>  <lambda `deck `(begin (def `take (\
`(n seq) `(if (<= n 0) `nil `(cons (car seq) (take (- n 1) (cdr seq)))))) (def `drop (\ `(n seq) `(if (<= n 0) `seq `(dr
op (- n 1) (cdr seq))))) (def `mid (\ `seq `(/ (len seq) 2))) ((combine append) (take (mid deck) deck) (drop (mid deck)
deck)))>
TEST #40> [PASSED] (riff-shuffle (list 1 2 3 4 5 6 7 8))  =>  `(1 5 2 6 3 7 4 8)
TEST #41> [PASSED] ((repeat riff-shuffle) (list 1 2 3 4 5 6 7 8))  =>  `(1 3 5 7 2 4 6 8)
TEST #42> [PASSED] (riff-shuffle (riff-shuffle (riff-shuffle (list 1 2 3 4 5 6 7 8))))  =>  `(1 2 3 4 5 6 7 8)
TEST #43> [PASSED] (def `apply (\ `(item L) `(eval (cons item L))))  =>  <lambda `(item L) `(eval (cons item L))>
TEST #44> [PASSED] (apply + `(1 2 3))  =>  6
TEST #45> [PASSED] (def `sum (\ `n `(if (< n 2) `1 `(+ n (sum (- n 1))))))  =>  <lambda `n `(if (< n 2) `1 `(+ n (sum (-
 n 1))))>
TEST #46> [PASSED] (sum 10)  =>  55
TEST #47> [PASSED] (def `Y (\ `f `((\ `self `(f (\ `x `((self self) x)))) (\ `self `(f (\ `x `((self self) x)))))))  =>
 <lambda `f `((\ `self `(f (\ `x `((self self) x)))) (\ `self `(f (\ `x `((self self) x)))))>
TEST #48> [PASSED] (def `Y_fib (\ `f `(\ `n `(if (<= n 2) `1 `(+ (f (- n 1)) (f (- n 2)))))))  =>  <lambda `f `(\ `n `(i
f (<= n 2) `1 `(+ (f (- n 1)) (f (- n 2)))))>
TEST #49> [PASSED] ((Y Y_fib) 5)  =>  5
TEST #50> [PASSED] (def `range (\ `(a b) `(if (== a b) `nil `(cons a (range (+ a 1) b)))))  =>  <lambda `(a b) `(if (==
a b) `nil `(cons a (range (+ a 1) b)))>
TEST #51> [PASSED] (range 1 10)  =>  `(1 2 3 4 5 6 7 8 9)
TEST #52> [PASSED] (apply + (range 1 10))  =>  45
==== ALL TEST PASSED [51/52] ====
```

## 目标

1. [ ] 对内存使用进行优化，减少不必要的拷贝操作。
2. [ ] 添加更多测试用例，确保GC工作的可靠性，避免循环引用与僵尸引用。
3. [ ] 用LISP语言实现高阶函数。

## 目标

当前进展：

- [x] 生成文法表达式
    - [x] 序列
    - [x] 分支
    - [x] 可选
    - [x] 跳过单词
- [x] 生成LR项目
- [x] 生成非确定性文法自动机
    - [x] 闭包求解
    - [x] 去Epsilon
    - [x] 打印NGA结构
- [x] 生成下推自动机
    - [x] 求First集合，并输出
    - [x] 检查文法有效性（如不产生Epsilon）
    - [x] 检查纯左递归
    - [x] 生成PDA
    - [x] 打印PDA结构（独立于内存池）
- [x] 生成抽象语法树
    - [x] 自动生成AST结构
    - [x] 美化AST结构
    - [ ] 语义动作
- [x] 设计语言
    - [x] 使用[C语言文法](https://github.com/antlr/grammars-v4/blob/master/c/C.g4)
    - [x] 实现回溯，解决移进/归约冲突问题，解决回溯的诸多BUG
    - [x] 实现LISP的循环
- [ ] LISP虚拟机
    - [x] 创建窗口
    - [ ] 更多内置指令
    - [ ] 控制台交互
    - [x] 图形交互
    - [ ] 模拟操作系统

1. 将文法树转换表（完成）
2. 根据PDA表生成AST（完成）

## 改进

1. [x] ~~修改了Lexer识别数字的问题~~
2. [x] ~~优化了内存池合并块算法，当没有元素被使用时将重置~~
3. [x] ~~添加错误恢复功能，异常时恢复GC的存储栈大小~~
4. [x] ~~更改了字符串管理方式，设为不可变~~
5. [x] ~~GC申请内存后自动清零~~
6. [x] ~~内存池算法可能存在问题，导致不定时崩溃~~
7. [x] ~~解决了数字溢出的问题~~
8. [x] ~~解决函数闭包问题~~
9. [x] ~~改进cons的实现~~
10. [x] ~~当使用函数不存在时发生崩溃，原因为内存池容量不够~~
11. [x] ~~调用递归时，环境变量env被free的问题，已修正~~
12. [x] ~~修正了类似double后缀的数字识别问题~~
13. [x] ~~修正了def函数无法修改外界环境的问题~~
14. [x] ~~修正了内存池申请的bug：删去为头时的情况；改正申请时块大小为size+1~~
15. [x] ~~修正了GC中unlink的bug~~

- [ ] 生成LR项目集时将@符号提到集合的外面，减少状态
- [x] PDA表的生成时使用了内存池来保存结点，当生成PDA表后，内存池可以全部回收
- [x] 生成AST时减少嵌套结点
- [ ] 优化回溯时产生的数据结构，减少拷贝
- [ ] 解析成功时释放结点内存
- [x] 将集合结点的标记修改成枚举
- [ ] 设置的终结符可以不添加到语法树中

## 参考

1. [CMiniLang](https://github.com/bajdcc/CMiniLang)
2. [lysp](http://piumarta.com/software/lysp/lysp-1.1/lysp.c)
3. [MyScript](https://github.com/bajdcc/MyScript)
4. [Build Your Own Lisp](http://buildyourownlisp.com)
5. [Lisp interpreter in 90 lines of C++](http://howtowriteaprogram.blogspot.com/2010/11/lisp-interpreter-in-90-lines-of-c.html)
6. [CParser](https://github.com/bajdcc/CParser)
7. [vczh GLR Parser](https://github.com/vczh-libraries/Vlpp/tree/master/Source/Parsing)