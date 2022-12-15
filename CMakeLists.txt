cmake_minimum_required(VERSION 3.16)
project(glfw-window)

link_libraries(GL glfw)

add_library(glfw-window STATIC glfw-window.cpp)