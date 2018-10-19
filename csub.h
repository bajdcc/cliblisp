//
// Project: cliblisp
// Created by bajdcc
//

#ifndef CLIBLISP_CSUB_H
#define CLIBLISP_CSUB_H

namespace clib {
    struct cval;

    cval *builtin_add(cval *val, cval *env);
    cval *builtin_sub(cval *val, cval *env);
    cval *builtin_mul(cval *val, cval *env);
    cval *builtin_div(cval *val, cval *env);
    cval *builtin_eval(cval *val, cval *env);
}

#endif //CLIBLISP_CSUB_H
