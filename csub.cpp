//
// Project: cliblisp
// Created by bajdcc
//

#include "cvm.h"
#include "csub.h"

namespace clib {
    cval *builtin_add(cval *val, cval *env) {
        auto _this = (cvm *)val->val._v.child->val._sub.vm;
        return _this->calc_sub("+", val, env);
    }

    cval *builtin_sub(cval *val, cval *env) {
        auto _this = (cvm *)val->val._v.child->val._sub.vm;
        return _this->calc_sub("-", val, env);
    }

    cval *builtin_mul(cval *val, cval *env) {
        auto _this = (cvm *)val->val._v.child->val._sub.vm;
        return _this->calc_sub("*", val, env);
    }

    cval *builtin_div(cval *val, cval *env) {
        auto _this = (cvm *)val->val._v.child->val._sub.vm;
        return _this->calc_sub("/", val, env);
    }

    cval *builtin_eval(cval *val, cval *env) {
        auto _this = (cvm *)val->val._v.child->val._sub.vm;
        if (val->val._v.count > 2)
            _this->error("eval not support more than one args");
        auto op = val->val._v.child->next;
        if (op->type == ast_qexpr)
            op->type = ast_sexpr;
        return _this->eval(op, env);
    }
}