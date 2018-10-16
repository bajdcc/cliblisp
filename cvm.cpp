//
// Project: cliblisp
// Created by bajdcc
//

#include <iostream>
#include <iomanip>
#include <cstring>
#include "cvm.h"
#include "cast.h"

#define SHOW_ALLOCATE_NODE 1

namespace clib {

    cvm::cvm() {
        builtin();
    }

    void cvm::builtin() {

    }

    cval *cvm::val(ast_t type) {
        auto v = mem.alloc<cval>();
        v->type = type;
        v->next = nullptr;
        return v;
    }

    cval *cvm::run_rec(ast_node *node) {
        if (node == nullptr)
            return nullptr;
        auto type = (ast_t) node->flag;
        switch (type) {
            case ast_root: // 根结点，全局声明
                return run_rec(node->child);
            case ast_sexpr:
                if (!node->child)
                    error("lambda: missing value");
                if (node->child->flag == ast_literal) {
                    auto v = val(type);
#if SHOW_ALLOCATE_NODE
                    printf("[DEBUG] Allocate val node: %s, count: %d\n", cast::ast_str(type).c_str(), cast::children_size(node));
#endif
                    v->val._v.child = nullptr;
                    auto i = node->child;
                    if (i->next == i) {
                        v->val._v.count = 1;
                        v->val._v.child = run_rec(i);
                        return eval(v);
                    }
                    auto local = run_rec(i);
                    v->val._v.child = local;
                    v->val._v.count++;
                    i = i->next;
                    while (i != node->child) {
                        v->val._v.count++;
                        local->next = run_rec(i);
                        local = local->next;
                        i = i->next;
                    }
                    return eval(v);
                } else
                    error("lambda: missing literal");
                break;
            case ast_literal: {
                auto v = val(type);
                v->val._string = node->data._string;
#if SHOW_ALLOCATE_NODE
                printf("[DEBUG] Allocate val node: %s, id: %s\n", cast::ast_str(type).c_str(), node->data._string);
#endif
                return v;
            }
#if SHOW_ALLOCATE_NODE
#define DEFINE_VAL(t) \
            case ast_##t: { \
                auto v = val(type); \
                v->val._##t = node->data._##t; \
                printf("[DEBUG] Allocate val node: %s, val: ", cast::ast_str(type).c_str()); \
                cast::print(node, 0, std::cout); \
                std::cout << std::endl; \
                return v; }
#else
#define DEFINE_VAL(t) \
            case ast_##t: { \
                auto v = val(type); \
                v->val._##t = node->data._##t; \
                return v; }
#endif
            DEFINE_VAL(char)
            DEFINE_VAL(uchar)
            DEFINE_VAL(short)
            DEFINE_VAL(ushort)
            DEFINE_VAL(int)
            DEFINE_VAL(uint)
            DEFINE_VAL(long)
            DEFINE_VAL(ulong)
            DEFINE_VAL(float)
            DEFINE_VAL(double)
#undef DEFINE_VAL
            default:
                break;
        }
    }

    cval *cvm::run(ast_node *root) {
        return run_rec(root);
    }

    void cvm::error(const string_t &info) {
        printf("COMPILER ERROR: %s\n", info.c_str());
        throw std::exception();
    }

    void cvm::print(cval *val, std::ostream &os) {
        if (!val)
            return;
        switch (val->type) {
            case ast_root:
                break;
            case ast_sexpr:
                break;
            case ast_literal:
                os << val->val._string;
                break;
            case ast_string:
                os << '"' << cast::display_str(val->val._string) << '"';
                break;
            case ast_char:
                if (isprint(val->val._char))
                    os << '\'' << val->val._char << '\'';
                else if (val->val._char == '\n')
                    os << "'\\n'";
                else
                    os << "'\\x" << std::setiosflags(std::ios::uppercase) << std::hex
                       << std::setfill('0') << std::setw(2)
                       << (unsigned int) val->val._char << '\'';
                break;
            case ast_uchar:
                os << (unsigned int) val->val._uchar;
                break;
            case ast_short:
                os << val->val._short;
                break;
            case ast_ushort:
                os << val->val._ushort;
                break;
            case ast_int:
                os << val->val._int;
                break;
            case ast_uint:
                os << val->val._uint;
                break;
            case ast_long:
                os << val->val._long;
                break;
            case ast_ulong:
                os << val->val._ulong;
                break;
            case ast_float:
                os << val->val._float;
                break;
            case ast_double:
                os << val->val._double;
                break;
        }
    }

    template<ast_t t>
    struct gen_op {
        void add(cval *r, cval *v) {}
        void sub(cval *r, cval *v) {}
        void mul(cval *r, cval *v) {}
        void div(cval *r, cval *v) {}
    };

#define DEFINE_VAL_OP(t) \
    template<> \
    struct gen_op<ast_##t> { \
        static void add(cval *r, cval *v) { r->val._##t += v->val._##t; } \
        static void sub(cval *r, cval *v) { r->val._##t -= v->val._##t; } \
        static void mul(cval *r, cval *v) { r->val._##t *= v->val._##t; } \
        static void div(cval *r, cval *v) { r->val._##t /= v->val._##t; } \
    };
    DEFINE_VAL_OP(char)
    DEFINE_VAL_OP(uchar)
    DEFINE_VAL_OP(short)
    DEFINE_VAL_OP(ushort)
    DEFINE_VAL_OP(int)
    DEFINE_VAL_OP(uint)
    DEFINE_VAL_OP(long)
    DEFINE_VAL_OP(ulong)
    DEFINE_VAL_OP(float)
    DEFINE_VAL_OP(double)
#undef DEFINE_VAL_OP

    void cvm::calc(char op, ast_t type, cval *r, cval *v) {
        switch (type) {
#define DEFINE_CALC_TYPE(t) \
            case ast_##t: \
                switch (op) { \
                    case '+': return gen_op<ast_##t>::add(r, v); \
                    case '-': return gen_op<ast_##t>::sub(r, v); \
                    case '*': return gen_op<ast_##t>::mul(r, v); \
                    case '/': return gen_op<ast_##t>::div(r, v); \
                } \
                break;
            DEFINE_CALC_TYPE(char)
            DEFINE_CALC_TYPE(uchar)
            DEFINE_CALC_TYPE(short)
            DEFINE_CALC_TYPE(ushort)
            DEFINE_CALC_TYPE(int)
            DEFINE_CALC_TYPE(uint)
            DEFINE_CALC_TYPE(long)
            DEFINE_CALC_TYPE(ulong)
            DEFINE_CALC_TYPE(float)
            DEFINE_CALC_TYPE(double)
#undef DEFINE_CALC_TYPE
            default:
                break;
        }
        error("unsupported calc op");
    }

    cval *cvm::calc_op(char op, cval *val) {
        if (!val)
            error("missing operator");
        auto v = val;
        auto r = mem.alloc<cval>();
        r->type = v->type;
        std::memcpy(&r->val, &v->val, sizeof(v->val));
        v = v->next;
        while (v) {
            calc(op, r->type, r, v);
            v = v->next;
        }
        return r;
    }

    cval *cvm::eval(cval *val) {
        if (!val)
            return nullptr;
        switch (val->type) {
            case ast_sexpr: {
                if (val->val._v.child) {
                    auto op = val->val._v.child;
                    auto opstr = op->val._string;
                    return calc_op(opstr[0], op->next);
                }
            }
                break;
            case ast_literal:
                break;
            default:
                return val;
        }
        return nullptr;
    }
}
