//
// Project: cliblisp
// Created by bajdcc
//

#include <GL/freeglut.h>
#include "cgui.h"

namespace clib {

    cgui::cgui() {
        codes.push("ui-put 'A'");
    }

    void cgui::draw() {
        for (int i = 0; i < GUI_TICKS; ++i) {
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
        if (running) {
            auto val = vm.run(GUI_CYCLES, c);
            if (val != nullptr) {
                vm.gc();
                vm.dump();
                running = false;
            }
        } else {
            if (!codes.empty()) {
                auto code = codes.front();
                codes.pop();
                try {
                    clib::cparser p;
                    auto root = p.parse(code);
                    vm.prepare(root);
                    auto val = vm.run(GUI_CYCLES, c);
                    if (val != nullptr) {
                        vm.gc();
                        vm.dump();
                    } else {
                        running = true;
                    }
                } catch (const std::exception &e) {
                    printf("RUNTIME ERROR: %s\n", e.what());
                    vm.restore();
                    vm.gc();
                }
            }
        }
    }
}
