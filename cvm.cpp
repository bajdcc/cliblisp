//
// Project: cliblisp
// Created by bajdcc
//

#include <iostream>
#include <iomanip>
#include <cstring>
#include "cvm.h"
#include "cast.h"
#include "csub.h"

#define SHOW_ALLOCATE_NODE 1

namespace clib {

    cvm::cvm() {
        builtin();
        set_free_callback();
    }

    void cvm::builtin() {
        global_env = val_obj(ast_env);
        global_env->val._env.env = new cval::cenv_t();
        global_env->val._env.parent = nullptr;
        mem.push_root(global_env);
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] Allocate val node: %s, address: 0x%p\n", cast::ast_str(global_env->type).c_str(), global_env);
#endif
        builtin_init();
        mem.pop_root();
        mem.protect(global_env);
    }

    static void add_builtin(cenv &env, const char *name, cval *val) {
        env.insert(std::make_pair(name, val));
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] Allocate val node: %s, name: %s, address: 0x%p\n", cast::ast_str(val->type).c_str(), name, val);
#endif
    }

    void cvm::builtin_init() {
        auto &_env = *global_env->val._env.env;
        add_builtin(_env, "__author__", val_str(ast_string, "bajdcc"));
        add_builtin(_env, "__project__", val_str(ast_string, "cliblisp"));
        add_builtin(_env, "+", val_sub(builtin_add));
        add_builtin(_env, "-", val_sub(builtin_sub));
        add_builtin(_env, "*", val_sub(builtin_mul));
        add_builtin(_env, "/", val_sub(builtin_div));
        add_builtin(_env, "eval", val_sub(builtin_eval));
    }

    cval *cvm::val_obj(ast_t type) {
        auto v = mem.alloc<cval>();
        v->type = type;
        v->next = nullptr;
        return v;
    }

    cval *cvm::val_str(ast_t type, const char *str) {
        auto len = strlen(str);
        auto v = (cval *) mem.alloc(sizeof(cval) + len + 1);
        v->type = type;
        v->next = nullptr;
        v->val._string = ((char *) v) + sizeof(cval);
        strncpy((char *) v->val._string, str, len);
        return v;
    }

    cval *cvm::val_sub(csub sub) {
        auto v = val_obj(ast_sub);
        v->val._sub.vm = this;
        v->val._sub.sub = sub;
        return v;
    }

    uint cvm::children_size(cval *val) {
        if (!val || (val->type != ast_sexpr && val->type != ast_qexpr))
            return 0;
        return val->val._v.count;
    }

    cval *cvm::run_rec(ast_node *node, cval *env) {
        if (node == nullptr)
            return nullptr;
        auto type = (ast_t) node->flag;
        switch (type) {
            case ast_root: // 根结点，全局声明
                return run_rec(node->child, env);
            case ast_sexpr:
                if (!node->child)
                    error("S-exp: missing value");
                if (node->child->flag == ast_literal) {
                    auto v = val_obj(type);
                    mem.push_root(v);
#if SHOW_ALLOCATE_NODE
                    printf("[DEBUG] Allocate val node: %s, count: %d, address: 0x%p\n", cast::ast_str(type).c_str(),
                           cast::children_size(node), v);
#endif
                    v->val._v.child = nullptr;
                    auto i = node->child;
                    if (i->next == i) {
                        v->val._v.count = 1;
                        v->val._v.child = run_rec(i, env);
                        mem.pop_root();
                        return eval(v, env);
                    }
                    auto local = run_rec(i, env);
                    v->val._v.child = local;
                    v->val._v.count++;
                    i = i->next;
                    while (i != node->child) {
                        v->val._v.count++;
                        local->next = run_rec(i, env);
                        local = local->next;
                        i = i->next;
                    }
                    mem.pop_root();
                    return eval(v, env);
                } else
                    error("S-exp: missing literal");
                break;
            case ast_qexpr:
                if (!node->child)
                    error("Q-exp: missing value");
                {
                    auto v = val_obj(type);
                    mem.push_root(v);
#if SHOW_ALLOCATE_NODE
                    printf("[DEBUG] Allocate val node: %s, count: %d, address: 0x%p\n", cast::ast_str(type).c_str(),
                           cast::children_size(node), node);
#endif
                    v->val._v.child = nullptr;
                    auto i = node->child;
                    if (i->next == i) {
                        v->val._v.count = 1;
                        v->val._v.child = run_rec(i, env);
                        mem.pop_root();
                        return eval(v, env);
                    }
                    auto local = run_rec(i, env);
                    v->val._v.child = local;
                    v->val._v.count++;
                    i = i->next;
                    while (i != node->child) {
                        v->val._v.count++;
                        local->next = run_rec(i, env);
                        local = local->next;
                        i = i->next;
                    }
                    mem.pop_root();
                    return v;
                }
            case ast_string: {
                auto v = val_str(type, node->data._string);
#if SHOW_ALLOCATE_NODE
                printf("[DEBUG] Allocate val node: %s, val: %s, address: 0x%p\n", cast::ast_str(type).c_str(),
                    v->val._string, v);
#endif
                return v;
            }
            case ast_literal: {
                auto v = val_str(type, node->data._string);
#if SHOW_ALLOCATE_NODE
                printf("[DEBUG] Allocate val node: %s, id: %s, address: 0x%p\n", cast::ast_str(type).c_str(),
                    v->val._string, v);
#endif
                return eval(v, env);
            }
#if SHOW_ALLOCATE_NODE
#define DEFINE_VAL(t) \
            case ast_##t: { \
                auto v = val_obj(type); \
                v->val._##t = node->data._##t; \
                printf("[DEBUG] Allocate val node: %s, val: ", cast::ast_str(type).c_str()); \
                print(v, std::cout); \
                printf(", address: 0x%p\n", v); \
                return v; }
#else
#define DEFINE_VAL(t) \
            case ast_##t: { \
                auto v = val_obj(type); \
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
        mem.save_stack();
        auto val = run_rec(root, global_env);
        return eval(val, global_env);
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
            case ast_env:
            case ast_sub:
                os << "<subroutine>";
                break;
            case ast_sexpr:
                break;
            case ast_qexpr: {
                os << '`';
                auto head = val->val._v.child;
                if (val->val._v.count == 1) {
                    print(head, os);
                } else {
                    os << '(';
                    while (head) {
                        print(head, os);
                        head = head->next;
                    }
                    os << ')';
                }
            }
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
        if (val->next) {
            os << ' ';
        }
    }

    void cvm::gc() {
        mem.gc();
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] Alive objects: %lu\n", mem.count());
#endif
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

    void cvm::calc(char op, ast_t type, cval *r, cval *v, cval *env) {
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

    cval *cvm::calc_op(char op, cval *val, cval *env) {
        if (!val)
            error("missing operator");
        auto v = val;
        if (v->type == ast_sub) {
            error("invalid operator type for sub");
        }
        if (v->type == ast_string) {
            if (op == '+') {
                std::stringstream ss;
                while (v) {
                    if (v->type != ast_string)
                        error("invalid operator type for string");
                    ss << v->val._string;
                    v = v->next;
                }
                return val_str(ast_string, ss.str().c_str());
            } else {
                error("invalid operator type for string");
            }
        }
        auto r = val_obj(v->type);
        std::memcpy((char *) &r->val, (char *) &v->val, sizeof(v->val));
        v = v->next;
        while (v) {
            if (r->type != v->type)
                error("invalid operator type");
            calc(op, r->type, r, v, env);
            v = v->next;
        }
        return r;
    }

    cval *cvm::calc_sub(const char *sub, cval *val, cval *env) {
        auto op = val->val._v.child->next;
        if (!isalpha(sub[0])) {
            return calc_op(sub[0], op, env);
        }
        error("not support subroutine yet");
        return nullptr;
    }

    cval *cvm::copy(cval *val) {
        cval *new_val{nullptr};
        switch (val->type) {
            case ast_root:
            case ast_env:
                error("not supported");
                break;
            case ast_sub:
                new_val = val_sub(val->val._sub.sub);
                new_val->val._sub.vm = val->val._sub.vm;
                break;
            case ast_sexpr:
            case ast_qexpr:
                new_val = val_obj(val->type);
                new_val->val._v.count = val->val._v.count;
                {
                    mem.push_root(new_val);
                    auto head = val->val._v.child;
                    new_val->val._v.child = copy(head);
                    if (val->val._v.count > 1) {
                        auto _head = new_val->val._v.child;
                        head = head->next;
                        while (head) {
                            _head->next = copy(head);
                            head = head->next;
                            _head = _head->next;
                        }
                    }
                    mem.pop_root();
                }
                break;
            case ast_literal:
            case ast_string:
                new_val = val_str(val->type, val->val._string);
                break;
            case ast_char:
            case ast_uchar:
            case ast_short:
            case ast_ushort:
            case ast_int:
            case ast_uint:
            case ast_long:
            case ast_ulong:
            case ast_float:
            case ast_double:
                new_val = val_obj(val->type);
                std::memcpy((char *) &new_val->val, (char *) &val->val, sizeof(val->val));
                break;
        }
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] Copy node: %s, address: 0x%p\n", cast::ast_str(val->type).c_str(), new_val);
#endif
        return new_val;
    }

    cval *cvm::calc_symbol(const char *sym, cval *env) {
        while (env) {
            auto &_env = *env->val._env.env;
            auto f = _env.find(sym);
            if (f != _env.end()) {
                return copy(f->second);
            }
            env = env->val._env.parent;
        }
        error("cannot find symbol");
        return nullptr;
    }

    cval *cvm::eval(cval *val, cval *env) {
        if (!val)
            return nullptr;
        switch (val->type) {
            case ast_sexpr: {
                if (val->val._v.child) {
                    auto op = val->val._v.child;
                    if (val->val._v.count == 1)
                        return eval(op, env);
                    if (op->type != ast_sub)
                        error("invalid operator type for S-exp");
                    auto sub = op->val._sub.sub;
                    return sub(val, env);
                }
            }
                break;
            case ast_literal:
                return calc_symbol(val->val._string, env);
            default:
                return val;
        }
    }

    void cvm::set_free_callback() {
#if SHOW_ALLOCATE_NODE
        mem.set_callback([](void *ptr) {
            cval *val = (cval *) ptr;
            printf("[DEBUG] GC free: 0x%p, node: %s, ", ptr, cast::ast_str(val->type).c_str());
            if (val->type == ast_sexpr || val->type == ast_qexpr) {
                printf("count: %lu\n", children_size(val));
            } else if (val->type == ast_literal) {
                printf("id: %s\n", val->val._string);
            } else if (val->type == ast_env) {
                printf("env: %d\n", val->val._env.env->size());
                delete val->val._env.env;
            } else if (val->type == ast_sub) {
                printf("\n");
            } else {
                printf("val: ");
                print(val, std::cout);
                std::cout << std::endl;
            }
        });
#else
        mem.set_callback([](void *ptr) {
            cval *val = (cval *) ptr;
            if (val->type == ast_env) {
                delete val->val._env.env;
            }
        });
#endif
    }

    void cvm::save() {
        mem.save_stack();
    }

    void cvm::restore() {
        mem.restore_stack();
    }

    void cvm::dump() {
        mem.dump(std::cout);
    }
}
