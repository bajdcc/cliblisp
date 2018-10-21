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

        static cval *def(cval *val, cval *env);
        static cval *lambda(cval *val, cval *env);
        static cval *call_lambda(cvm *vm, cval *param, cval *body, cval *val, cval *env);

        static cval *lt(cval *val, cval *env);
        static cval *le(cval *val, cval *env);
        static cval *gt(cval *val, cval *env);
        static cval *ge(cval *val, cval *env);
        static cval *eq(cval *val, cval *env);
        static cval *ne(cval *val, cval *env);

        static cval *_if(cval *val, cval *env);

        static cval *len(cval *val, cval *env);
    };
}

#endif //CLIBLISP_CSUB_H
