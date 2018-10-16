//
// Project: cliblisp
// Created by bajdcc
//

#include <iostream>
#include "cparser.h"
#include "cvm.h"

int main(int argc, char *argv[]) {
    clib::cvm vm;
    std::string input;
    while (true) {
        std::cout << "lisp> ";
        std::getline(std::cin, input);
        if (input == "exit")
            break;
        if (input.empty())
            continue;
        try {
            clib::cparser p(input);
            auto root = p.parse();
            //clib::cast::print(root, 0, std::cout);
            auto val = vm.run(root);
            vm.print(val, std::cout);
        } catch (const std::exception &e) {
            printf("RUNTIME ERROR: %s", e.what());
        }
        std::cout << std::endl;
    }
    return 0;
}