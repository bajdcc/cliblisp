cmake_minimum_required(VERSION 3.12)
project(cliblisp)

set(CMAKE_CXX_STANDARD 14)

link_libraries(freeglut opengl32 glu32)

add_executable(cliblisp
        main.cpp
        memory.h
        memory_gc.h
        types.h
        types.cpp
        clexer.h
        clexer.cpp
        cparser.h
        cparser.cpp
        cunit.h
        cunit.cpp
        cexception.h
        cexception.cpp
        cast.h
        cast.cpp
        cvm.cpp
        cvm.h
        csub.cpp
        csub.h cgui.cpp cgui.h)

add_executable(cliblisp-test
        test.cpp
        memory.h
        memory_gc.h
        types.h
        types.cpp
        clexer.h
        clexer.cpp
        cparser.h
        cparser.cpp
        cunit.h
        cunit.cpp
        cexception.h
        cexception.cpp
        cast.h
        cast.cpp
        cvm.cpp
        cvm.h
        csub.cpp
        csub.h)