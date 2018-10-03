# cliblisp（C++ 简易LISP解释器）

借鉴[CMiniLang](https://github.com/bajdcc/CMiniLang)的部分代码。

主要借鉴了CMiniLang的类型系统、词法分析、语法分析、AST、内存管理等代码。

事实证明CMiniLang的框架还是非常经典耐用的（再次强调）。

本说明完善中。

## 功能

- [x] 词法分析
- [x] 语法分析
- [x] 内存管理
- [x] 序列化
- [ ] 添加更多功能

## 使用

```cpp
    try {
        auto json = R"(
(print "Hello\n" "World!"
    (+
        (author "bajdcc")
        (project "cliblisp")))
)";
        clib::cparser p(json);
        auto root = p.parse();
        clib::cast::print(root, 0, std::cout);
    } catch (const std::exception& e) {
        printf("ERROR: %s\n", e.what());
    }
```

## 参考

1. [CMiniLang](https://github.com/bajdcc/CMiniLang)
2. [lysp](http://piumarta.com/software/lysp/lysp-1.1/lysp.c)
3. [MyScript](https://github.com/bajdcc/MyScript)