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
- [x] GC
- [x] 异常恢复
- [x] 简单的内建四则运算
- [ ] 常用内建函数/输入输出
- [x] 字符串处理
- [ ] 识别变量
- [ ] 识别函数
- [ ] 添加更多功能

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
lisp> + 9 8
17
```

### GC

```lisp
lisp> 1
[DEBUG] Allocate val node: int, val: 1
1
[DEBUG] GC free: 0x0054204C, node: int, val: 1
[DEBUG] Alive objects: 0
lisp> + 2 3
[DEBUG] Allocate val node: sexpr, count: 3
[DEBUG] Allocate val node: literal, id: +
[DEBUG] Allocate val node: int, val: 2
[DEBUG] Allocate val node: int, val: 3
5
[DEBUG] GC free: 0x0054204C, node: sexpr, count: 4
[DEBUG] GC free: 0x0054207C, node: literal, id: +
[DEBUG] GC free: 0x005420AC, node: int, val: 2
[DEBUG] GC free: 0x005420DC, node: int, val: 3
[DEBUG] GC free: 0x0054210C, node: int, val: 5
[DEBUG] Alive objects: 0
lisp> + 1 2 ( - 3 4 )
[DEBUG] Allocate val node: sexpr, count: 4
[DEBUG] Allocate val node: literal, id: +
[DEBUG] Allocate val node: int, val: 1
[DEBUG] Allocate val node: int, val: 2
[DEBUG] Allocate val node: sexpr, count: 3
[DEBUG] Allocate val node: literal, id: -
[DEBUG] Allocate val node: int, val: 3
[DEBUG] Allocate val node: int, val: 4
2
[DEBUG] GC free: 0x0054204C, node: sexpr, count: 8
[DEBUG] GC free: 0x0054207C, node: literal, id: +
[DEBUG] GC free: 0x005420AC, node: int, val: 1
[DEBUG] GC free: 0x005420DC, node: int, val: 2
[DEBUG] GC free: 0x0054210C, node: sexpr, count: 8
[DEBUG] GC free: 0x0054213C, node: literal, id: -
[DEBUG] GC free: 0x0054216C, node: int, val: 3
[DEBUG] GC free: 0x0054219C, node: int, val: 4
[DEBUG] GC free: 0x005421CC, node: int, val: -1
[DEBUG] GC free: 0x005421FC, node: int, val: 2
[DEBUG] Alive objects: 0
lisp> + 1 2 ( - 3 d)
[DEBUG] Allocate val node: sexpr, count: 4
[DEBUG] Allocate val node: literal, id: +
[DEBUG] Allocate val node: int, val: 1
[DEBUG] Allocate val node: int, val: 2
[DEBUG] Allocate val node: sexpr, count: 3
[DEBUG] Allocate val node: literal, id: -
[DEBUG] Allocate val node: int, val: 3
[DEBUG] Allocate val node: literal, id: d
COMPILER ERROR: invalid operator type
RUNTIME ERROR: std::exception
[DEBUG] GC free: 0x0054204C, node: sexpr, count: 12
[DEBUG] GC free: 0x0054207C, node: literal, id: +
[DEBUG] GC free: 0x005420AC, node: int, val: 1
[DEBUG] GC free: 0x005420DC, node: int, val: 2
[DEBUG] GC free: 0x0054210C, node: sexpr, count: 11
[DEBUG] GC free: 0x0054213C, node: literal, id: -
[DEBUG] GC free: 0x0054216C, node: int, val: 3
[DEBUG] GC free: 0x0054219C, node: literal, id: d
[DEBUG] GC free: 0x005421CC, node: int, val: 3
[DEBUG] Alive objects: 0
lisp> + "Hello" " " "world!"
[DEBUG] Allocate val node: sexpr, count: 4
[DEBUG] Allocate val node: literal, id: +
[DEBUG] Allocate val node: string, id: Hello
[DEBUG] Allocate val node: string, id:
[DEBUG] Allocate val node: string, id: world!
"Hello world!"
[DEBUG] GC free: 0x0063C04C, node: sexpr, count: 4
[DEBUG] GC free: 0x0063C07C, node: literal, id: +
[DEBUG] GC free: 0x0063C0AC, node: string, val: "Hello"
[DEBUG] GC free: 0x0063C0EC, node: string, val: " "
[DEBUG] GC free: 0x0063C11C, node: string, val: "world!"
[DEBUG] GC free: 0x0063C15C, node: string, val: "Hello world!"
[DEBUG] Alive objects: 0
```

## 改进

1. [x] ~~修改了Lexer识别数字的问题~~
2. [x] ~~优化了内存池合并块算法，当没有元素被使用时将重置~~
3. [x] ~~添加错误恢复功能，异常时恢复GC的存储栈大小~~

## 参考

1. [CMiniLang](https://github.com/bajdcc/CMiniLang)
2. [lysp](http://piumarta.com/software/lysp/lysp-1.1/lysp.c)
3. [MyScript](https://github.com/bajdcc/MyScript)