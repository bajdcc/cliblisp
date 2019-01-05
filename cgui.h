//
// Project: cliblisp
// Created by bajdcc
//

#ifndef CLIBLISP_CGUI_H
#define CLIBLISP_CGUI_H

#include <array>
#include <deque>
#include "types.h"
#include "cparser.h"
#include "cvm.h"

#define GUI_FONT GLUT_BITMAP_9_BY_15
#define GUI_FONT_W 9
#define GUI_FONT_H 15
#define GUI_ROWS 30
#define GUI_COLS 80
#define GUI_SIZE (GUI_ROWS * GUI_COLS)
#define GUI_CYCLES 50
#define GUI_TICKS 5

namespace clib {

    class cgui {
    public:
        cgui();
        ~cgui() = default;

        cgui(const cgui &) = delete;
        cgui &operator=(const cgui &) = delete;

        void draw();

        void put_char(char c);
    private:
        void tick();
        void draw_text();

        void new_line();
        inline void draw_char(const char &c);

    public:
        static cgui &singleton();

    private:
        std::array<std::array<char, GUI_COLS>, GUI_ROWS> buffer;
        std::deque<string_t> codes;
        cvm vm;
        cparser p;
        bool running{false};
        int ptr_x{0};
        int ptr_y{0};
        int ptr_mx{0};
        int ptr_my{0};
    };
}

#endif //CLIBLISP_CGUI_H
