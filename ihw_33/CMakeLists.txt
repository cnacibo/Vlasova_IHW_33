cmake_minimum_required(VERSION 3.28)
project(ihw_33 C)

set(CMAKE_C_STANDARD 17)

add_executable(ihw_33 main.c
        defs.h
        master.c
        worker.c)
