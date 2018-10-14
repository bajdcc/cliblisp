//
// Project: cliblisp
// Created by bajdcc
//

#ifndef CLIBLISP_CVM_H
#define CLIBLISP_CVM_H

#define VM_MEM (32 * 1024)

#include <vector>
#include "cast.h"

namespace clib {

    struct cval {
        ast_t type;
        cval *next;
        union {
            struct {
                uint count;
                cval *child;
            } _v;
            const char *_string;
#define DEFINE_CVAL(t) LEX_T(t) _##t;
            DEFINE_CVAL(char)
            DEFINE_CVAL(uchar)
            DEFINE_CVAL(short)
            DEFINE_CVAL(ushort)
            DEFINE_CVAL(int)
            DEFINE_CVAL(uint)
            DEFINE_CVAL(long)
            DEFINE_CVAL(ulong)
            DEFINE_CVAL(float)
            DEFINE_CVAL(double)
#undef DEFINE_CVAL
        } val;
    };

    class cvm {
    public:
        cvm();
        ~cvm() = default;

        cvm(const cvm &) = delete;
        cvm &operator=(const cvm &) = delete;

        cval *run(ast_node *root);
        void print(cval *val, std::ostream &os);

    private:
        void builtin();
        cval *run_rec(ast_node *node);

        void calc(char op, ast_t type, cval *r, cval *v);
        cval *calc_op(char op, cval *val);
        cval *eval(cval *val);

        void error(const string_t &info);

    private:
        // TODO: 后面做一个简单的GC，当异常时收回当前申请的内存
        memory_pool<VM_MEM> mem;
    };
}

#endif //CLIBLISP_CVM_H
