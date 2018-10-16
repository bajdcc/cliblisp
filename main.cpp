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
            vm.save();
            auto val = vm.run(root);
            clib::cvm::print(val, std::cout);
            std::cout << std::endl;
            vm.gc();
        } catch (const std::exception &e) {
            printf("RUNTIME ERROR: %s\n", e.what());
            vm.restore();
            vm.gc();
        }
    }
    return 0;
}