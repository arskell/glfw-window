#ifndef GLFW_WINDOW_H
#define GLFW_WINDOW_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <string>
#include <atomic>


void glfw_windows_init();

/*returns newly created window id*/
GLFWwindow* create_window(int width, int height, const std::string& title);
void apply_pixelmap(GLFWwindow* wnd, void* data, unsigned int size);
void destroy_window(GLFWwindow* wnd);
void glfw_windows_do_event_loop(std::atomic<bool>& exit_signal);


#endif