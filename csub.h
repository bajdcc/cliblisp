//
// Project: cliblisp
// Created by bajdcc
//

#ifndef CLIBLISP_CSUB_H
#define CLIBLISP_CSUB_H

namespace clib {
    struct cval;
    class cvm;

    class builtins {
    public:
        static cval *add(cval *val, cval *env);
        static cval *sub(cval *val, cval *env);
        static cval *mul(cval *val, cval *env);
        static cval *div(cval *val, cval *env);
        static cval *eval(cval *val, cval *env);
        static cval *quote(cval *val, cval *env);
        static cval *list(cval *val, cval *env);
        static cval *car(cval *val, cval *env);
        static cval *cdr(cval *val, cval *env);
    };
}

#endif //CLIBLISP_CSUB_H
