//
// Project: cliblisp
// Created by bajdcc
//

#include <cstring>
#include <sstream>
#include "cvm.h"
#include "csub.h"
#include "cparser.h"
#include "cgui.h"

#define VM_OP(val) (val->val._v.child->next)

#define VM_CALL(name) vm->calc_sub(name, frame->val, frame->env)
#define VM_NIL vm->val_obj(ast_qexpr)

#define VM_RET(val) {*frame->ret = (val); return s_ret; }

namespace clib {

    bool strequ(const char * a, const char * b)
    {
        return std::strcmp(a, b) == 0;
    }

    static void add_builtin(cenv &env, const char *name, cval *val) {
        env.insert(std::make_pair(name, val));
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s, for builtin\n", val, cast::ast_str(val->type).c_str());
#endif
    }

    void cvm::builtin_load() {
        // Load init code
        auto codes = std::vector<std::string>{
            R"(def `nil `())",
            R"(def `cadr (\ `x `(car (cdr x))))",
            R"(def `caar (\ `x `(car (car x))))",
            R"(def `cdar (\ `x `(cdr (car x))))",
            R"(def `cddr (\ `x `(cdr (cdr x))))",
            R"(def `cddr (\ `x `(cdr (cdr x))))",
            R"((def `range (\ `(a b) `(if (== a b) `nil `(cons a (range (+ a 1) b))))))",
            R"(def `map-i (\ `(f L i) `(if (>= i (len L)) `nil `(begin (f (index L i)) (map-i f L (+ i))))))",
            R"(def `map (\ `(f L) `(if (null? L) `nil `(map-i f L 0))))",
        };
        cparser p;
        try {
            for (auto &code : codes) {
                auto cycles = 0;
                save();
                auto root = p.parse(code);
                prepare(root);
                auto val = run(INT32_MAX, cycles);
#if SHOW_ALLOCATE_NODE
                std::cout << "builtin> ";
                cast::print(root, 0, std::cout);
                std::cout << std::endl;
                cvm::print(val, std::cout);
                std::cout << std::endl;
#endif
                gc();
            }
        } catch (const std::exception &e) {
            printf("RUNTIME ERROR: %s\n", e.what());
            restore();
            gc();
            exit(-1);
        }
    }

    const char *logo() {
        return R"(
  .g8"""bgd `7MMF'      `7MMF'`7MM"""Yp, `7MMF'      `7MMF' .M"""bgd `7MM"""Mq.
.dP'     `M   MM          MM    MM    Yb   MM          MM  ,MI    "Y   MM   `MM.
dM'       `   MM          MM    MM    dP   MM          MM  `MMb.       MM   ,M9
MM            MM          MM    MM"""bg.   MM          MM    `YMMNq.   MMmmdM9
MM.           MM      ,   MM    MM    `Y   MM      ,   MM  .     `MM   MM
`Mb.     ,'   MM     ,M   MM    MM    ,9   MM     ,M   MM  Mb     dM   MM
  `"bmmmd'  .JMMmmmmMMM .JMML..JMMmmmd9  .JMMmmmmMMM .JMML.P"Ybmmd"  .JMML.

)";
    }

    void cvm::builtin_init() {
        auto &_env = *global_env->val._env.env;
        add_builtin(_env, "__author__", val_str(ast_string, "bajdcc"));
        add_builtin(_env, "__project__", val_str(ast_string, "cliblisp"));
        add_builtin(_env, "__logo__", val_str(ast_string, logo()));
        add_builtin(_env, "+", val_sub("+", builtins::add));
        add_builtin(_env, "-", val_sub("-", builtins::sub));
        add_builtin(_env, "*", val_sub("*", builtins::mul));
        add_builtin(_env, "/", val_sub("/", builtins::div));
        add_builtin(_env, "\\", val_sub("\\", builtins::lambda));
        add_builtin(_env, "<", val_sub("<", builtins::lt));
        add_builtin(_env, "<=", val_sub("<=", builtins::le));
        add_builtin(_env, ">", val_sub(">", builtins::gt));
        add_builtin(_env, ">=", val_sub(">=", builtins::ge));
        add_builtin(_env, "==", val_sub("==", builtins::eq));
        add_builtin(_env, "!=", val_sub("!=", builtins::ne));
        add_builtin(_env, "eval", val_sub("eval", builtins::call_eval));
        add_builtin(_env, "if", val_sub("if", builtins::_if));
        add_builtin(_env, "null?", val_sub("null?", builtins::is_null));
#define ADD_BUILTIN(name) add_builtin(_env, #name, val_sub(#name, builtins::name))
        ADD_BUILTIN(quote);
        ADD_BUILTIN(list);
        ADD_BUILTIN(car);
        ADD_BUILTIN(cdr);
        ADD_BUILTIN(cons);
        ADD_BUILTIN(def);
        ADD_BUILTIN(begin);
        ADD_BUILTIN(append);
        ADD_BUILTIN(len);
        ADD_BUILTIN(index);
        ADD_BUILTIN(type);
        ADD_BUILTIN(str);
        ADD_BUILTIN(word);
        ADD_BUILTIN(print);
        ADD_BUILTIN(conf);
        ADD_BUILTIN(attr);
#undef ADD_BUILTIN
        add_builtin(_env, "ui-put", val_sub("ui-put", builtins::ui_put));
    }

    template<ast_t t>
    struct gen_op {
        void add(cval *r, cval *v) {}
        void sub(cval *r, cval *v) {}
        void mul(cval *r, cval *v) {}
        void div(cval *r, cval *v) {}
        bool eq(cval *r, cval *v) {}
        bool ne(cval *r, cval *v) {}
        bool le(cval *r, cval *v) {}
        bool ge(cval *r, cval *v) {}
        bool lt(cval *r, cval *v) {}
        bool gt(cval *r, cval *v) {}
    };

#define DEFINE_VAL_OP(t) \
    template<> \
    struct gen_op<ast_##t> { \
        static void add(cval *r, cval *v) { if (v == nullptr) r->val._##t++; else r->val._##t += v->val._##t; } \
        static void sub(cval *r, cval *v) { if (v == nullptr) r->val._##t--; else r->val._##t -= v->val._##t; } \
        static void mul(cval *r, cval *v) { if (v != nullptr) r->val._##t *= v->val._##t; } \
        static void div(cval *r, cval *v) { if (v != nullptr) r->val._##t /= v->val._##t; } \
        static bool eq(cval *r, cval *v) { return r->val._##t == v->val._##t; } \
        static bool ne(cval *r, cval *v) { return r->val._##t != v->val._##t; } \
        static bool le(cval *r, cval *v) { return r->val._##t <= v->val._##t; } \
        static bool ge(cval *r, cval *v) { return r->val._##t >= v->val._##t; } \
        static bool lt(cval *r, cval *v) { return r->val._##t < v->val._##t; } \
        static bool gt(cval *r, cval *v) { return r->val._##t > v->val._##t; } \
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

    int cvm::calc(int op, ast_t type, cval *r, cval *v, cval *env) {
        switch (type) {
#define DEFINE_CALC_TYPE(t) \
            case ast_##t: \
                switch (op) { \
                    case '+': gen_op<ast_##t>::add(r, v); return 0; \
                    case '-': gen_op<ast_##t>::sub(r, v); return 0; \
                    case '*': gen_op<ast_##t>::mul(r, v); return 0; \
                    case '/': gen_op<ast_##t>::div(r, v); return 0; \
                    case '=' | '=' << 8: return gen_op<ast_##t>::eq(r, v); \
                    case '!' | '=' << 8: return gen_op<ast_##t>::ne(r, v); \
                    case '<' | '=' << 8: return gen_op<ast_##t>::le(r, v); \
                    case '>' | '=' << 8: return gen_op<ast_##t>::ge(r, v); \
                    case '<': return gen_op<ast_##t>::lt(r, v); \
                    case '>': return gen_op<ast_##t>::gt(r, v); \
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
        return 0;
    }

    static bool is_comparison(int op) {
        switch (op) {
            case '=' | '=' << 8:
            case '!' | '=' << 8:
            case '<' | '=' << 8:
            case '>' | '=' << 8:
            case '<':
            case '>':
                return true;
            default:
                return false;
        }
    }

    cval *cvm::calc_op(int op, cval *val, cval *env) {
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
            }
            error("invalid operator type for string");
        }
        if (is_comparison(op)) {
            if (!v->next || v->next->next)
                error("comparison requires 2 arguments");
            auto v2 = v->next;
            if (v->type != v2->type)
                error("invalid operator type for comparison");
            if (v->type == ast_string) {
                switch (op) {
                    case '=' | '=' << 8:
                        return val_bool(strcmp(v->val._string, v2->val._string) == 0);
                    case '!' | '=' << 8:
                        return val_bool(strcmp(v->val._string, v2->val._string) != 0);
                    case '<' | '=' << 8:
                        return val_bool(strcmp(v->val._string, v2->val._string) <= 0);
                    case '>' | '=' << 8:
                        return val_bool(strcmp(v->val._string, v2->val._string) >= 0);
                    case '<':
                        return val_bool(strcmp(v->val._string, v2->val._string) < 0);
                    case '>':
                        return val_bool(strcmp(v->val._string, v2->val._string) > 0);
                    default:
                        break;
                }
            }
            if (v->type == ast_qexpr) {
                std::stringstream s1, s2;
                v->next = nullptr;
                v2->next = nullptr;
                print(v, s1);
                print(v2, s2);
                auto a = s1.str();
                auto b = s2.str();
                return val_bool(a == b);
            }
            return val_bool(calc(op, v->type, v, v2, env) != 0);
        }
        auto r = val_obj(v->type);
        std::memcpy((char *) &r->val, (char *) &v->val, sizeof(v->val));
        v = v->next;
        if (v) {
            while (v) {
                if (r->type != v->type)
                    error("invalid operator type");
                calc(op, r->type, r, v, env);
                v = v->next;
            }
        } else {
            calc(op, r->type, r, v, env);
        }
        return r;
    }

    cval *cvm::calc_sub(const char *sub, cval *val, cval *env) {
        auto op = val->val._v.child->next;
        if (!isalpha(sub[0])) {
            if (strlen(sub) <= 2)
                return calc_op(sub[0] | sub[1] << 8, op, env);
        }
        error("not support subroutine yet");
        return nullptr;
    }

    static char *sub_name(cval *val) {
        return (char *) val + sizeof(cval);
    }

    status_t cvm::eval_one(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        auto &env = frame->env;
        if (frame->arg == nullptr) {
            return vm->call(eval, val->val._v.child, env, &(cval *&) frame->arg);
        } else {
            VM_RET((cval *) frame->arg);
        }
    }

    status_t cvm::eval_child(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        auto &env = frame->env;
        auto op = val->val._v.child;
        switch (op->type) {
            case ast_sub: {
                auto sub = op->val._sub.sub;
                return sub(vm, frame);
            }
            case ast_lambda: {
                return builtins::call_lambda(vm, frame);
            }
            case ast_sexpr:
            case ast_literal: {
                struct tmp_bag {
                    int step;
                    bool quote;
                    cval *v;
                    cval *local;
                    cval *i;
                    cval *r;
                };
                if (frame->arg == nullptr) {
                    auto v = vm->val_obj(val->type);
                    vm->mem.push_root(v);
#if SHOW_ALLOCATE_NODE
                    if (op->type == ast_literal) {
                        printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s, count: %lu\n", v,
                               cast::ast_str(val->type).c_str(),
                               children_size(val));
                    } else {
                        printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s\n", v, cast::ast_str(op->type).c_str());
                    }
#endif
                    v->val._v.child = nullptr;
                    v->val._v.count = 0;
                    auto tmp = vm->eval_tmp.alloc<tmp_bag>();
                    memset(tmp, 0, sizeof(tmp_bag));
                    tmp->v = v;
                    tmp->i = op;
                    frame->arg = tmp;
                    return vm->call(eval, op, env, &tmp->local);
                } else {
                    auto tmp = (tmp_bag *) frame->arg;
                    assert(tmp->local);
                    if (tmp->step == 0) {
                        auto &v = tmp->v;
                        auto &local = tmp->local;
                        auto &i = tmp->i;
                        if (op->type == ast_literal && local->type == ast_sub) {
                            if (strstr(sub_name(local), "quote")) {
                                tmp->quote = true;
                            }
                        }
                        v->val._v.child = local;
                        v->val._v.count = 1;
                        i = i->next;
                        if (i) {
                            if (tmp->quote) {
                                while (i) {
                                    v->val._v.count++;
                                    local->next = i;
                                    local = local->next;
                                    i = i->next;
                                }
                                tmp->step = 2;
                                vm->mem.pop_root();
                                return vm->call(eval, v, env, &tmp->r);
                            } else {
                                tmp->step = 1;
                                v->val._v.count++;
                                return vm->call(eval, i, env, &tmp->r);
                            }
                        } else {
                            tmp->step = 2;
                            vm->mem.pop_root();
                            return vm->call(eval, v, env, &tmp->r);
                        }
                    } else if (tmp->step == 1) {
                        auto &v = tmp->v;
                        auto &local = tmp->local;
                        auto &i = tmp->i;
                        local->next = tmp->r;
                        local = local->next;
                        i = i->next;
                        if (i) {
                            v->val._v.count++;
                            return vm->call(eval, i, env, &tmp->r);
                        } else {
                            tmp->step = 2;
                            vm->mem.pop_root();
                            return vm->call(eval, v, env, &tmp->r);
                        }
                    } else if (tmp->step == 2) {
                        auto r = tmp->r;
                        vm->eval_tmp.free(tmp);
                        VM_RET(r);
                    } else {
                        vm->error("invalid step in eval");
                        VM_RET(VM_NIL);
                    }
                }
            }
            default:
                break;
        }
        vm->error("invalid operator type for S-exp");
        VM_RET(VM_NIL);
    }

    status_t cvm::eval(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        auto &env = frame->env;
        if (!val) {
            VM_RET(VM_NIL);
        }
        switch (val->type) {
            case ast_sexpr: {
                if (val->val._v.child) {
                    if (val->val._v.count == 1) {
                        return eval_one(vm, frame);
                    }
                    return eval_child(vm, frame);
                } else {
                    VM_RET(VM_NIL);
                }
            }
            case ast_literal: {
                VM_RET(vm->calc_symbol(val->val._string, env));
            }
            default:
                break;
        }
        VM_RET(val);
    }

    status_t builtins::add(cvm *vm, cframe *frame) {
        VM_RET(VM_CALL("+"));
    }

    status_t builtins::sub(cvm *vm, cframe *frame) {
        VM_RET(VM_CALL("-"));
    }

    status_t builtins::mul(cvm *vm, cframe *frame) {
        VM_RET(VM_CALL("*"));
    }

    status_t builtins::div(cvm *vm, cframe *frame) {
        VM_RET(VM_CALL("/"));
    }

    status_t builtins::quote(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        if (val->val._v.count > 2)
            vm->error("quote not support more than one args");
        auto op = VM_OP(val);
        auto v = vm->val_obj(ast_qexpr);
        vm->mem.push_root(v);
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s, for quote\n", v, cast::ast_str(v->type).c_str());
#endif
        v->val._v.count = 1;
        v->val._v.child = vm->copy(op);
        vm->mem.pop_root();
        VM_RET(v);
    }

    status_t builtins::list(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        auto op = VM_OP(val);
        if (val->val._v.count == 2 && op->val._v.count == 0) VM_RET(vm->copy(op));
        auto v = vm->val_obj(ast_qexpr);
        vm->mem.push_root(v);
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s, for list\n", v, cast::ast_str(v->type).c_str());
#endif
        auto i = op;
        auto local = vm->copy(i);
        v->val._v.child = local;
        v->val._v.count = 1;
        i = i->next;
        while (i) {
            v->val._v.count++;
            local->next = vm->copy(i);
            local = local->next;
            i = i->next;
        }
        vm->mem.pop_root();
        VM_RET(v);
    }

    status_t builtins::car(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        if (val->val._v.count > 2)
            vm->error("car not support more than one args");
        auto op = VM_OP(val);
        if (op->type != ast_qexpr)
            vm->error("car need Q-exp");
        if (!op->val._v.child) VM_RET(VM_NIL);
        if (op->val._v.child->type == ast_sexpr) {
            VM_RET(vm->copy(op->val._v.child->val._v.child));
        } else {
            VM_RET(vm->copy(op->val._v.child));
        }
    }

    status_t builtins::cdr(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        if (val->val._v.count > 2)
            vm->error("cdr not support more than one args");
        auto op = VM_OP(val);
        if (op->type != ast_qexpr)
            vm->error("cdr need Q-exp");
        if (op->val._v.count > 0) {
            if (op->val._v.child->next) {
                auto v = vm->val_obj(ast_qexpr);
                vm->mem.push_root(v);
#if SHOW_ALLOCATE_NODE
                printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s, for cdr\n", v, cast::ast_str(v->type).c_str());
#endif
                auto i = op->val._v.child->next;
                auto local = vm->copy(i);
                v->val._v.child = local;
                v->val._v.count = 1;
                i = i->next;
                while (i) {
                    v->val._v.count++;
                    local->next = vm->copy(i);
                    local = local->next;
                    i = i->next;
                }
                vm->mem.pop_root();
                VM_RET(v);
            } else {
                VM_RET(VM_NIL);
            }
        } else {
            VM_RET(VM_NIL);
        }
    }

    status_t builtins::cons(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        if (val->val._v.count != 3)
            vm->error("cons requires 2 args");
        auto op = VM_OP(val);
        auto op2 = op->next;
        // if (op->type != ast_qexpr)
        //     vm->error("cons need Q-exp for first argument");
        if (op2->type != ast_qexpr)
            vm->error("cons need Q-exp for second argument");
        // if (op->val._v.count != 1)
        //     vm->error("cons need Q-exp(only one child) for first argument");
        // if (op2->val._v.count < 2)
        //     vm->error("cons need Q-exp(more than one child) for first argument");
        if (op2->val._v.count == 0) {
            auto v = vm->val_obj(ast_qexpr);
            vm->mem.push_root(v);
            v->val._v.child = vm->copy(op);
            v->val._v.count = 1;
            vm->mem.pop_root();
            VM_RET(v);
        }

        if (op2->type != ast_qexpr)
            vm->error("cons need Q-exp for second argument");
        auto v = vm->val_obj(ast_qexpr);
        vm->mem.push_root(v);
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s, for cons\n", v, cast::ast_str(v->type).c_str());
#endif
        v->val._v.child = vm->copy(op);
        v->val._v.count = 1 + op2->val._v.count;
        auto i = op2->val._v.child;
        auto local = vm->copy(i);
        v->val._v.child->next = local;
        i = i->next;
        while (i) {
            local->next = vm->copy(i);
            local = local->next;
            i = i->next;
        }
        vm->mem.pop_root();
        VM_RET(v);
    }

    status_t builtins::def(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        auto &env = frame->env;
        if (val->val._v.count <= 2)
            vm->error("def not support less than 2 args");
        auto op = VM_OP(val);
        if (op->type != ast_qexpr)
            vm->error("def need Q-exp for first argument");
        if (op->val._v.count == val->val._v.count - 2) {
            auto param = op->val._v.child;
            auto argument = op->next;
            for (auto i = 0; i < op->val._v.count; ++i) {
                if (param->type != ast_literal) {
                    vm->error("def need literal for Q-exp");
                }
                param = param->next;
            }
            param = op->val._v.child;
            vm->mem.push_root(env);
            cval *first_def = nullptr;
            for (auto i = 0; i < op->val._v.count; ++i) {
                auto name = param->val._string;
                auto _def = vm->def(env, name, argument);
                if (first_def == nullptr)
                    first_def = _def;
                param = param->next;
                argument = argument->next;
            }
            vm->mem.pop_root();
            if (op->val._v.count == 1) {
                VM_RET(vm->copy(first_def));
            }
            VM_RET(VM_NIL);
        } else {
            vm->error("def need same size of Q-exp and argument");
            VM_RET(nullptr);
        }
    }

    status_t builtins::lambda(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        auto &env = frame->env;
        if (val->val._v.count != 3)
            vm->error("lambda requires 2 args");
        auto op = VM_OP(val);
        if (op->type != ast_qexpr)
            vm->error("lambda need Q-exp for first argument");
        if (op->next->type != ast_qexpr)
            vm->error("lambda need Q-exp for second argument");
        auto param = op->val._v.child;
        for (auto i = 0; i < op->val._v.count; ++i) {
            if (param->type != ast_literal) {
                vm->error("lambda need valid argument type");
            }
            param = param->next;
        }
        VM_RET(vm->val_lambda(op, op->next, env));
    }

    static cval **lambda_env(cval *val) {
        return (cval **) ((char *) val + sizeof(cval));
    }

    status_t builtins::call_lambda(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        auto &env = frame->env;
        auto op = val->val._v.child;
        auto param = op->val._lambda.param;
        auto body = op->val._lambda.body;
        if (val->val._v.count != param->val._v.count + 1)
            vm->error("lambda need valid argument size");
        if (frame->arg == nullptr) {
            auto &env2 = *lambda_env(op);
            if (env2 != env)
                env2->val._env.parent = env;
            auto _param = param->val._v.child;
            auto _argument = op->next;
            auto new_env = vm->new_env(env2);
            vm->mem.unlink(new_env);
            auto &_env = *new_env->val._env.env;
            vm->mem.push_root(new_env);
            while (_param) {
                auto name = _param->val._string;
                _env[name] = vm->copy(_argument);
                _param = _param->next;
                _argument = _argument->next;
            }
            vm->mem.pop_root();
            assert(body->type == ast_qexpr);
            body->type = ast_sexpr;
            return vm->call(cvm::eval, body, new_env, &(cval *&) frame->arg);
        } else {
            auto ret = (cval *) frame->arg;
            body->type = ast_qexpr;
            VM_RET(ret);
        }
    }

    status_t builtins::call_eval(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        auto &env = frame->env;
        if (val->val._v.count > 2)
            vm->error("eval not support more than one args");
        auto op = VM_OP(val);
        struct tmp_bag {
            bool qexp;
            cval *ret;
        };
        if (frame->arg == nullptr) {
            auto tmp = vm->eval_tmp.alloc<tmp_bag>();
            memset(tmp, 0, sizeof(tmp_bag));
            tmp->qexp = op->type == ast_qexpr;
            frame->arg = tmp;
            if (tmp->qexp)
                op->type = ast_sexpr;
            return vm->call(cvm::eval, op, env, &tmp->ret);
        } else {
            auto tmp = (tmp_bag *) frame->arg;
            if (tmp->qexp)
                op->type = ast_qexpr;
            auto ret = tmp->ret;
            vm->eval_tmp.free(tmp);
            VM_RET(ret);
        }
    }

    status_t builtins::lt(cvm *vm, cframe *frame) {
        VM_RET(VM_CALL("<"));
    }

    status_t builtins::le(cvm *vm, cframe *frame) {
        VM_RET(VM_CALL("<="));
    }

    status_t builtins::gt(cvm *vm, cframe *frame) {
        VM_RET(VM_CALL(">"));
    }

    status_t builtins::ge(cvm *vm, cframe *frame) {
        VM_RET(VM_CALL(">="));
    }

    status_t builtins::eq(cvm *vm, cframe *frame) {
        VM_RET(VM_CALL("=="));
    }

    status_t builtins::ne(cvm *vm, cframe *frame) {
        VM_RET(VM_CALL("!="));
    }

    status_t builtins::begin(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        auto op = VM_OP(val);
        while (op->next) {
            op = op->next;
        }
        VM_RET(op);
    }

    status_t builtins::_if(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        auto &env = frame->env;
        if (val->val._v.count != 4)
            vm->error("if requires 3 args");
        auto op = VM_OP(val);
        if (frame->arg == nullptr) {
            auto flag = true;
            if (op->type == ast_int && op->val._int == 0)
                flag = false;
            auto _t = op->next;
            auto _f = _t->next;
            if (_t->type != ast_qexpr)
                vm->error("lambda need Q-exp for true branch");
            if (_f->type != ast_qexpr)
                vm->error("lambda need Q-exp for false branch");
            if (flag) {
                _t->type = ast_sexpr;
                return vm->call(cvm::eval, _t, env, &(cval *&) frame->arg);
            } else {
                _f->type = ast_sexpr;
                return vm->call(cvm::eval, _f, env, &(cval *&) frame->arg);
            }
        } else {
            VM_RET((cval *) frame->arg);
        }
    }

    status_t builtins::len(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        auto op = VM_OP(val);
        if (op->type != ast_qexpr)
            vm->error("len requires Q-exp");
        auto v = vm->val_obj(ast_int);
        v->val._int = (int) op->val._v.count;
        VM_RET(v);
    }

    status_t builtins::index(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        if (val->val._v.count != 3)
            vm->error("index requires 2 args");
        auto op = VM_OP(val);
        if (op->type != ast_qexpr)
            vm->error("len requires Q-exp for first arg");
        if (op->next->type != ast_int)
            vm->error("len requires int for second arg");
        auto i = op->next->val._int;
        auto size = op->val._v.count;
        if (i >= 0 && i < size) {
            auto node = op->val._v.child;
            for (int j = 0; j < i; ++j) {
                node = node->next;
            }
            VM_RET(vm->copy(node));
        }
        VM_RET(VM_NIL);
    }

    status_t builtins::append(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        auto op = VM_OP(val);
        if (op->type != ast_qexpr)
            vm->error("append need Q-exp for first argument");
        if (val->val._v.count == 2) {
            VM_RET(vm->copy(op));
        }
        auto v = vm->copy(op);
        vm->mem.push_root(v);
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s, for append\n", v, cast::ast_str(v->type).c_str());
#endif
        auto i = op->next;
        auto local = v->val._v.child;
        while (local->next) {
            local = local->next;
        }
        while (i) {
            if (i->type == ast_qexpr) {
                if (i->val._v.count > 0) {
                    auto j = i->val._v.child;
                    while (j) {
                        local->next = vm->copy(j);
                        local = local->next;
                        j = j->next;
                        v->val._v.count++;
                    }
                    i = i->next;
                } else {
                    i = i->next;
                }
            } else {
                local->next = vm->copy(i);
                local = local->next;
                i = i->next;
                v->val._v.count++;
            }
        }
        vm->mem.pop_root();
        VM_RET(v);
    }

    status_t builtins::is_null(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        if (val->val._v.count != 2)
            vm->error("null? requires 1 args");
        auto op = VM_OP(val);
        VM_RET(vm->val_bool(op->type == ast_qexpr && op->val._v.count == 0));
    }

    status_t builtins::type(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        if (val->val._v.count != 2)
            vm->error("type requires 1 args");
        auto op = VM_OP(val);
        VM_RET(vm->val_str(ast_string, cast::ast_str(op->type).c_str()));
    }

    static void stringify(cval *val, std::ostream &os) {
        if (!val)
            return;
        switch (val->type) {
            case ast_string:
                os << val->val._string;
                break;
            case ast_char:
                os << val->val._char;
                break;
            default:
                cvm::print(val, os);
        }
    }

    status_t builtins::str(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        if (val->val._v.count != 2)
            vm->error("str requires 1 args");
        auto op = VM_OP(val);
        std::stringstream ss;
        stringify(op, ss);
        VM_RET(vm->val_str(ast_string, ss.str().c_str()));
    }

    status_t builtins::word(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        if (val->val._v.count != 2)
            vm->error("word requires 1 args");
        auto op = VM_OP(val);
        if (op->type != ast_string)
            vm->error("word requires string");
        auto s = string_t(op->val._string);
        auto v = vm->val_obj(ast_qexpr);
        v->val._v.count = 0;
        v->val._v.child = nullptr;
        if (s.empty()) {
            VM_RET(v);
        }
        vm->mem.push_root(v);
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s, for word\n", v, cast::ast_str(v->type).c_str());
#endif
        auto len = s.length();
        v->val._v.count = len;
        v->val._v.child = vm->val_char(s[0]);
        auto local = v->val._v.child;
        for (auto i = 1; i < len; i++) {
            local->next = vm->val_char(s[i]);
            local = local->next;
        }
        vm->mem.pop_root();
        VM_RET(v);
    }

    status_t builtins::print(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        if (val->val._v.count != 2)
            vm->error("str requires 1 args");
        auto op = VM_OP(val);
        decltype(op->val._string) s;
        if (op->type != ast_string) {
            std::stringstream ss;
            stringify(op, ss);
            s = ss.str().c_str();
        } else {
            s = op->val._string;
        }
        std::cout << s;
        VM_RET(VM_NIL);
    }

    status_t builtins::conf(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        auto i = VM_OP(val);
        auto not_ret = false;
        while (i) {
            if (i->type == ast_qexpr && i->val._v.count >= 1 && i->val._v.child->type == ast_literal) {
                auto op = i->val._v.child;
                auto count = i->val._v.count;
                auto str = op->val._string;
                if (strequ(str, "cycle") && count == 2 && op->next->type == ast_int) {
                    auto cycle = op->next->val._int;
                    cgui::singleton().set_cycle(cycle);
                } else if (strequ(str, "ticks") && count == 2 && op->next->type == ast_int) {
                    auto cycle = op->next->val._int;
                    cgui::singleton().set_ticks(cycle);
                } else if (strequ(str, "record") && count == 1) {
                    if (frame->arg != (void *) 1) {
                        cgui::singleton().record();
                        frame->arg = (void *) 1;
                    }
                } else if (strequ(str, "continue") && count == 1) {
                    if (frame->arg != (void *) 1) {
                        cgui::singleton().control(0);
                        frame->arg = (void *) 1;
                    }
                } else if (strequ(str, "break") && count == 1) {
                    if (frame->arg != (void *) 1) {
                        cgui::singleton().control(1);
                        frame->arg = (void *) 1;
                    }
                } else if (strequ(str, "wait") && count == 2 && op->next->type == ast_double) {
                    auto offset = op->next->val._double;
                    if (!cgui::singleton().reach(offset))
                        not_ret = true;
                }
            }
            i = i->next;
        }
        if (not_ret)
            return s_sleep;
        VM_RET(VM_NIL);
    }

    status_t builtins::attr(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        if (val->val._v.count < 2)
            vm->error("attr requires more than 1 args");
        auto op = VM_OP(val);
        if (op->type != ast_qexpr)
            vm->error("attr requires Q-exp at first");
        auto v = vm->copy(op);
        v->val._v.count = val->val._v.count - 1;
        vm->mem.push_root(v);
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s, for word\n", v, cast::ast_str(v->type).c_str());
#endif
        auto i = op->next;
        auto local = v->val._v.child;
        while (i) {
            local->next = vm->copy(i);
            local = local->next;
            i = i->next;
        }
        vm->mem.pop_root();
        VM_RET(v);
    }

    // GUI

    status_t builtins::ui_put(cvm *vm, cframe *frame) {
        auto &val = frame->val;
        if (val->val._v.count != 2)
            vm->error("ui-put requires 1 args");
        auto op = VM_OP(val);
        if (op->type != ast_char)
            vm->error("ui-put requires char type");
        auto c = op->val._char;
        cgui::singleton().put_char(c);
        VM_RET(VM_NIL);
    }
}