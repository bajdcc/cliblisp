//
// Project: cliblisp
// Created by bajdcc
//

#include <GL/freeglut.h>
#include "cgui.h"

namespace clib {

    cgui::cgui() {
        auto cs = std::vector<string_t>{
            R"(def `put-str (\ `s `(map ui-put (word s))))",
            R"(def `ui-put-delay (\ `(c t) `(begin (ui-put c) (conf `(record)) (conf (attr `wait t)))))",
            R"(def `put-str-delay (\ `(s t) `(map (\ `c `(ui-put-delay c t)) (word s))))",
            R"(def `(i S) 0 (word __logo__))",
            R"(conf `(ticks 1000))",
            R"(if (< i (len S)) `(begin (ui-put (index S i)) (def `i (+ i)) (conf `continue)) `(conf `break))",
            R"(conf `(ticks 5))",
            R"(put-str-delay "Hello world!\n" 0.4d)",
            R"(put-str "Welcome to cliblisp by bajdcc!\n")",
        };
        std::copy(cs.begin(), cs.end(), std::back_inserter(codes));
    }

    cgui &cgui::singleton() {
        static clib::cgui gui;
        return gui;
    }

    void cgui::draw() {
        for (int i = 0; i < ticks; ++i) {
            tick();
        }
        draw_text();
    }

    void cgui::draw_text() {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        int w = glutGet(GLUT_WINDOW_WIDTH);
        int h = glutGet(GLUT_WINDOW_HEIGHT);
        int width = GUI_COLS * GUI_FONT_W;
        int height = GUI_ROWS * GUI_FONT_H;
        gluOrtho2D(0, w, h, 0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glColor3f(0.9f, 0.9f, 0.9f);
        int x = std::max((w - width) / 2, 0);
        int y = std::max((h - height) / 2, 0);

        for (auto i = 0; i < GUI_ROWS; ++i) {
            glRasterPos2i(x, y);
            for (auto j = 0; j < GUI_COLS; ++j) {
                glutBitmapCharacter(GUI_FONT, buffer[i][j]);
            }
            y += GUI_FONT_H;
        }

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    }

    void cgui::tick() {
        auto c = 0;
        auto error = false;
        if (running) {
            try {
                auto val = vm.run(cycle, c);
                if (val != nullptr) {
                    vm.gc();
                    vm.dump();
                    running = false;
                }
            } catch (const std::exception &e) {
                error = true;
                printf("RUNTIME ERROR: %s\n", e.what());
                vm.restore();
                vm.gc();
                running = false;
            }
        } else {
            if (!codes.empty()) {
                current_code = codes.front();
                codes.pop_front();
                try {
                    auto root = p.parse(current_code);
                    vm.prepare(root);
                    auto val = vm.run(GUI_CYCLES, c);
                    if (val != nullptr) {
                        vm.gc();
                        vm.dump();
                    } else {
                        running = true;
                    }
                } catch (const std::exception &e) {
                    error = true;
                    printf("RUNTIME ERROR: %s\n", e.what());
                    vm.restore();
                    vm.gc();
                }
            }
        }
        if (continues > 0) {
            if (error) {
                continues = 0;
                current_code = "";
            } else {
                codes.push_front(current_code);
            }
        }
    }

    void cgui::put_char(char c) {
        if (c == '\n') {
            if (ptr_y == GUI_ROWS - 1) {
                new_line();
            } else {
                ptr_x = 0;
                ptr_y++;
            }
        } else if (c == '\b') {
            if (ptr_mx + ptr_my * GUI_COLS < ptr_x + ptr_y * GUI_COLS) {
                if (ptr_y == 0) {
                    if (ptr_x != 0) {
                        draw_char('\u0000');
                        ptr_x--;
                    }
                } else {
                    if (ptr_x != 0) {
                        draw_char('\u0000');
                        ptr_x--;
                    } else {
                        draw_char('\u0000');
                        ptr_x = GUI_COLS - 1;
                        ptr_y--;
                    }
                }
            }
        } else if (c == '\u0002') {
            ptr_x--;
            while (ptr_x >= 0) {
                draw_char('\u0000');
                ptr_x--;
            }
            ptr_x = 0;
        } else if (c == '\r') {
            ptr_x = 0;
        } else if (ptr_x == GUI_COLS - 1) {
            if (ptr_y == GUI_ROWS - 1) {
                draw_char(c);
                new_line();
            } else {
                draw_char(c);
                ptr_x = 0;
                ptr_y++;
            }
        } else {
            draw_char(c);
            ptr_x++;
        }
    }

    void cgui::new_line() {
        ptr_x = 0;
        for (int i = 0; i < GUI_ROWS - 1; ++i) {
            std::copy(buffer[i + 1].begin(), buffer[i + 1].end(), buffer[i].begin());
        }
        std::fill(buffer[GUI_ROWS - 1].begin(), buffer[GUI_ROWS - 1].end(), 0);
    }

    void cgui::draw_char(const char &c) {
        buffer[ptr_y][ptr_x] = c;
    }

    void cgui::set_cycle(int cycle) {
        this->cycle = cycle;
    }

    void cgui::set_ticks(int ticks) {
        this->ticks = ticks;
    }

    void cgui::record() {
        record_now = std::chrono::high_resolution_clock::now();
    }

    bool cgui::reach(const decimal &d) {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::duration<decimal>>(now - record_now).count() > d;
    }

    void cgui::control(int type) {
        if (type == 0) { // continue
            continues++;
        } else if (type == 1) { // break
            continues = 0;
        }
    }
}
