# cliblisp（C++ 简易LISP解释器）

借鉴[CMiniLang](https://github.com/bajdcc/CMiniLang)的部分代码。

主要借鉴了CMiniLang的类型系统、词法分析、语法分析、AST、内存管理等代码。

事实证明CMiniLang的框架还是非常经典耐用的（再次强调）。

本说明完善中。

## 功能

当前仅完成了四则运算，采用解释器求值。

- [x] 词法分析
- [x] 语法分析
- [x] 内存管理
- [x] 序列化
- [x] 识别数字
- [x] 识别S-表达式
- [ ] 识别Q-表达式
- [ ] GC
- [ ] 异常恢复
- [x] 简单的内建四则运算
- [ ] 常用内建函数/输入输出
- [ ] 字符串处理
- [ ] 识别变量
- [ ] 识别函数
- [ ] 添加更多功能

## 使用

```cpp
    clib::cvm vm;
    std::string input;
    while (input != "exit") {
        std::cout << "lisp> ";
        std::getline(std::cin, input);
        try {
            clib::cparser p(input);
            auto root = p.parse();
            //clib::cast::print(root, 0, std::cout);
            auto val = vm.run(root);
            vm.print(val, std::cout);
        } catch (const std::exception &e) {
            printf("RUNTIME ERROR: %s", e.what());
        }
        std::cout << std::endl;
    }
```

## 例子

```lisp
lisp>+ 1 2
 + 1 2
3
lisp>* 1 2 3 4 5 6
 * 1 2 3 4 5 6
720
lisp>- 8 4 2 9 8 
 - 8 4 2 9 8
-15
lisp>+ a 5
 + a 5
COMPILER ERROR: unsupported calc op
RUNTIME ERROR: std::exception
lisp>+ 9 8
 + 9 8
17
```

## 参考

1. [CMiniLang](https://github.com/bajdcc/CMiniLang)
2. [lysp](http://piumarta.com/software/lysp/lysp-1.1/lysp.c)
3. [MyScript](https://github.com/bajdcc/MyScript)