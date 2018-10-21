//
// Project: cliblisp
// Created by bajdcc
//

#include <cstring>
#include <sstream>
#include "cvm.h"
#include "csub.h"

#define VM_THIS(val) ((cvm *)val->val._v.child->val._sub.vm)
#define VM_OP(val) (val->val._v.child->next)

#define VM_CALL(name) VM_THIS(val)->calc_sub(name, val, env)
#define VM_NIL(this) this->val_obj(ast_qexpr)

namespace clib {

    static void add_builtin(cenv &env, const char *name, cval *val) {
        env.insert(std::make_pair(name, val));
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s, for builtin\n", val, cast::ast_str(val->type).c_str());
#endif
    }

    void cvm::builtin_init() {
        auto &_env = *global_env->val._env.env;
        add_builtin(_env, "__author__", val_str(ast_string, "bajdcc"));
        add_builtin(_env, "__project__", val_str(ast_string, "cliblisp"));
        add_builtin(_env, "nil", val_obj(ast_qexpr));
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
#define ADD_BUILTIN(name) add_builtin(_env, #name, val_sub(#name, builtins::name))
        ADD_BUILTIN(eval);
        ADD_BUILTIN(quote);
        ADD_BUILTIN(list);
        ADD_BUILTIN(car);
        ADD_BUILTIN(cdr);
        ADD_BUILTIN(def);
#undef ADD_BUILTIN
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
        static void add(cval *r, cval *v) { r->val._##t += v->val._##t; } \
        static void sub(cval *r, cval *v) { r->val._##t -= v->val._##t; } \
        static void mul(cval *r, cval *v) { r->val._##t *= v->val._##t; } \
        static void div(cval *r, cval *v) { r->val._##t /= v->val._##t; } \
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
            return val_bool(calc(op, v->type, v, v2, env) != 0);
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
            if (strlen(sub) <= 2)
                return calc_op(sub[0] | sub[1] << 8, op, env);
        }
        error("not support subroutine yet");
        return nullptr;
    }

    cval *builtins::add(cval *val, cval *env) {
        return VM_CALL("+");
    }

    cval *builtins::sub(cval *val, cval *env) {
        return VM_CALL("-");
    }

    cval *builtins::mul(cval *val, cval *env) {
        return VM_CALL("*");
    }

    cval *builtins::div(cval *val, cval *env) {
        return VM_CALL("/");
    }

    cval *builtins::eval(cval *val, cval *env) {
        auto _this = VM_THIS(val);
        if (val->val._v.count > 2)
            _this->error("eval not support more than one args");
        auto op = VM_OP(val);
        if (op->type == ast_qexpr)
            op->type = ast_sexpr;
        return _this->eval(op, env);
    }

    cval *builtins::quote(cval *val, cval *env) {
        auto _this = VM_THIS(val);
        if (val->val._v.count > 2)
            _this->error("quote not support more than one args");
        auto op = VM_OP(val);
        auto v = _this->val_obj(ast_qexpr);
        _this->mem.push_root(v);
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s, for quote\n", v, cast::ast_str(v->type).c_str());
#endif
        v->val._v.count = 1;
        v->val._v.child = _this->copy(op);
        _this->mem.pop_root();
        return v;
    }

    cval *builtins::list(cval *val, cval *env) {
        auto _this = VM_THIS(val);
        auto op = VM_OP(val);
        auto v = _this->val_obj(ast_qexpr);
        _this->mem.push_root(v);
#if SHOW_ALLOCATE_NODE
        printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s, for list\n", v, cast::ast_str(v->type).c_str());
#endif
        auto i = op;
        auto local = _this->copy(i);
        v->val._v.child = local;
        v->val._v.count = 1;
        i = i->next;
        while (i) {
            v->val._v.count++;
            local->next = _this->copy(i);
            local = local->next;
            i = i->next;
        }
        _this->mem.pop_root();
        return v;
    }

    cval *builtins::car(cval *val, cval *env) {
        auto _this = VM_THIS(val);
        if (val->val._v.count > 2)
            _this->error("car not support more than one args");
        auto op = VM_OP(val);
        if (op->type != ast_qexpr)
            _this->error("car need Q-exp");
        if (!op->val._v.child)
            return VM_NIL(_this);
        if (op->val._v.child->type == ast_sexpr) {
            return _this->copy(op->val._v.child->val._v.child);
        } else {
            return _this->copy(op->val._v.child);
        }
    }

    cval *builtins::cdr(cval *val, cval *env) {
        auto _this = VM_THIS(val);
        if (val->val._v.count > 2)
            _this->error("cdr not support more than one args");
        auto op = VM_OP(val);
        if (op->type != ast_qexpr)
            _this->error("cdr need Q-exp");
        if (op->val._v.count > 0) {
            if (op->val._v.child->next) {
                auto v = _this->val_obj(ast_qexpr);
                _this->mem.push_root(v);
#if SHOW_ALLOCATE_NODE
                printf("[DEBUG] ALLOC | addr: 0x%p, node: %-10s, for cdr\n", v, cast::ast_str(v->type).c_str());
#endif
                auto i = op->val._v.child->next;
                auto local = _this->copy(i);
                v->val._v.child = local;
                v->val._v.count = 1;
                i = i->next;
                while (i) {
                    v->val._v.count++;
                    local->next = _this->copy(i);
                    local = local->next;
                    i = i->next;
                }
                _this->mem.pop_root();
                return v;
            } else {
                return VM_NIL(_this);
            }
        } else {
            return VM_NIL(_this);
        }
    }

    cval *builtins::def(cval *val, cval *env) {
        auto _this = VM_THIS(val);
        if (val->val._v.count <= 2)
            _this->error("def not support less than 2 args");
        auto op = VM_OP(val);
        if (op->type != ast_qexpr)
            _this->error("def need Q-exp for first argument");
        if (op->val._v.count == val->val._v.count - 2) {
            auto param = op->val._v.child;
            auto argument = op->next;
            for (auto i = 0; i < op->val._v.count; ++i) {
                if (param->type != ast_literal) {
                    _this->error("def need literal for Q-exp");
                }
                param = param->next;
            }
            param = op->val._v.child;
            _this->mem.push_root(env);
            auto &_env = *env->val._env.env;
            for (auto i = 0; i < op->val._v.count; ++i) {
                auto name = param->val._string;
                auto old = _env.find(name);
                if (old != _env.end()) {
                    _this->mem.unlink(env, old->second);
                }
                _env[param->val._string] = _this->copy(argument);
                param = param->next;
                argument = argument->next;
            }
            _this->mem.pop_root();
            return VM_NIL(_this);
        } else {
            _this->error("def need same size of Q-exp and argument");
            return nullptr;
        }
    }

    cval *builtins::lambda(cval *val, cval *env) {
        auto _this = VM_THIS(val);
        if (val->val._v.count != 3)
            _this->error("lambda requires 2 args");
        auto op = VM_OP(val);
        if (op->type != ast_qexpr)
            _this->error("lambda need Q-exp for first argument");
        if (op->next->type != ast_qexpr)
            _this->error("lambda need Q-exp for second argument");
        auto param = op->val._v.child;
        for (auto i = 0; i < op->val._v.count; ++i) {
            if (param->type != ast_literal) {
                _this->error("lambda need valid argument type");
            }
            param = param->next;
        }
        auto v = _this->val_obj(ast_lambda);
        _this->mem.push_root(v);
        v->val._lambda.param = _this->copy(op);
        v->val._lambda.body = _this->copy(op->next);
        v->val._lambda.body->type = ast_sexpr;
        _this->mem.pop_root();
        return v;
    }

    cval *builtins::call_lambda(cvm *vm, cval *param, cval *body, cval *val, cval *env) {
        auto _this = vm;
        if (val->val._v.count != param->val._v.count + 1)
            _this->error("lambda need valid argument size");
        auto op = VM_OP(val);
        auto _param = param->val._v.child;
        auto _argument = op;
        auto new_env = _this->new_env(env);
        auto &_env = *new_env->val._env.env;
        _this->mem.push_root(new_env);
        while (_param) {
            auto name = _param->val._string;
            _env[name] = _this->copy(_argument);
            _param = _param->next;
            _argument = _argument->next;
        }
        auto ret = _this->eval(body, new_env);
        _this->mem.pop_root();
        return ret;
    }

    cval *builtins::lt(cval *val, cval *env) {
        return VM_CALL("<");
    }

    cval *builtins::le(cval *val, cval *env) {
        return VM_CALL("<=");
    }

    cval *builtins::gt(cval *val, cval *env) {
        return VM_CALL(">");
    }

    cval *builtins::ge(cval *val, cval *env) {
        return VM_CALL(">=");
    }

    cval *builtins::eq(cval *val, cval *env) {
        return VM_CALL("==");
    }

    cval *builtins::ne(cval *val, cval *env) {
        return VM_CALL("!=");
    }
}