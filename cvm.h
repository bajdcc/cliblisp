//
// Project: cliblisp
// Created by bajdcc
//

#ifndef CLIBLISP_CVM_H
#define CLIBLISP_CVM_H

#define VM_MEM (32 * 1024)
#define SHOW_ALLOCATE_NODE 0

#include <vector>
#include "cast.h"
#include "memory_gc.h"

namespace clib {

    struct cval {
        using cenv_t = std::unordered_map<std::string, cval *>;
        using csub_t = cval *(*)(cval *arg, cval *env);
        ast_t type;
        cval *next;
        union {
            struct {
                uint count;
                cval *child;
            } _v;
            struct {
                cval *parent;
                cenv_t *env;
            } _env;
            struct {
                void *vm;
                csub_t sub;
            } _sub;
            struct {
                cval *param;
                cval *body;
            } _lambda;
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

    using cenv = cval::cenv_t;
    using csub = cval::csub_t;

    class cvm {
    public:
        cvm();
        ~cvm() = default;

        cvm(const cvm &) = delete;
        cvm &operator=(const cvm &) = delete;

        friend class builtins;

        cval *run(ast_node *root);
        void gc();

        static void print(cval *val, std::ostream &os);

        void save();
        void restore();

        void error(const string_t &info);

        void dump();

    private:
        void builtin();
        void builtin_init();
        cval *run_rec(ast_node *node, cval *env, bool quote);

        void calc(char op, ast_t type, cval *r, cval *v, cval *env);
        cval *calc_op(char op, cval *val, cval *env);
        cval *calc_symbol(const char *sym, cval *env);
        cval *calc_sub(const char *sub, cval *val, cval *env);
        cval *calc_lambda(cval *param, cval *body, cval *val, cval *env);
        cval *eval(cval *val, cval *env);
        cval *val_obj(ast_t type);
        cval *val_str(ast_t type, const char *str);
        cval *val_sub(const char *name, csub sub);
        cval *val_sub(cval *val);

        cval *copy(cval *val);
        cval *new_env(cval *env);

        static uint children_size(cval *val);

        void set_free_callback();

    private:
        cval *global_env{nullptr};
        memory_pool_gc<VM_MEM> mem;
    };
}

#endif //CLIBLISP_CVM_H
