//
// Project: cliblisp
// Created by bajdcc
//

#include <iostream>
#include "cparser.h"

int main(int argc, char *argv[]) {
    try {
        auto lisp = R"(
(print "Hello\n" "World!"
    (+
        (author "bajdcc")
        (project "cliblisp")))
)";
        clib::cparser p(lisp);
        auto root = p.parse();
        clib::cast::print(root, 0, std::cout);
    } catch (const std::exception& e) {
        printf("ERROR: %s\n", e.what());
    }
    return 0;
}