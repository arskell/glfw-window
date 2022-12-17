#include "glfw-window.h"
#include <GL/gl.h>

#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <map>
#include <vector>
#include <memory.h>
#include <mutex>

struct GLFW_Window{
    std::vector<char> current_pixelmap;    
    int width;
    int height;
};

class GLFW_Window_map{
    std::map<GLFWwindow*, GLFW_Window> _data;
    std::mutex _ownership;

    struct MailBox{
        bool is_data_present;
        std::vector<char> data;
    };

    std::map<GLFWwindow*, MailBox> _mail_box;
    std::mutex _mailbox_ownership;

    void _update_content(GLFWwindow* wnd, void *data, unsigned int size){
        auto it = _data.find(wnd);
        assert(it != _data.end());
        assert(it->second.current_pixelmap.size() == size);
        memcpy( it->second.current_pixelmap.data(), data, it->second.current_pixelmap.size());
    }

public:
    void add_window(GLFWwindow* wnd, int width, int height){
        std::lock_guard<std::mutex> lg(_ownership);
        std::lock_guard<std::mutex> lg2(_mailbox_ownership);
        GLFW_Window content;
        const auto size = width * height * 4;
        content.current_pixelmap.resize(size);
        auto mail_box = content.current_pixelmap;

        content.height = height;
        content.width = width;
        _data.insert({wnd, std::move(content)});
        _mail_box.insert({wnd,{false, std::move(mail_box)}});
    }

    void update_content(GLFWwindow* wnd, void *data, unsigned int size){
        if(_ownership.try_lock()){
            _update_content(wnd, data, size);
            _ownership.unlock();
        }else{
            std::lock_guard<std::mutex> lg(_mailbox_ownership);
            auto it = _mail_box.find(wnd);
            assert(it->second.data.size() == size);
            memcpy(it->second.data.data(), data, size);
            it->second.is_data_present = true;
        }
    }

    void remove_window(GLFWwindow* wnd){
        std::lock_guard<std::mutex> lg(_ownership);
        std::lock_guard<std::mutex> lg2(_mailbox_ownership);
        glfwDestroyWindow(wnd);
        _data.erase(wnd);
        _mail_box.erase(wnd);
    }

    bool render_all_windows(){
        std::lock_guard<std::mutex> lg(_ownership);
        
        //try to look into mailbox
        if(_mailbox_ownership.try_lock()){
            
            for(auto& elem: _mail_box){
                if(elem.second.is_data_present){
                    auto it = _data.find(elem.first);
                    if(it != _data.end()){
                        assert(it->second.current_pixelmap.size() == elem.second.data.size());
                        memcpy(it->second.current_pixelmap.data(), elem.second.data.data(), elem.second.data.size() * sizeof( elem.second.data[0]));
                    }
                    elem.second.is_data_present = false;
                }

            }

            _mailbox_ownership.unlock();
        }

        if(_data.empty()){
            return false;
        }

        std::vector<GLFWwindow*> delete_queue;

        for(auto& elem: _data){
            if(glfwWindowShouldClose(elem.first)){
                delete_queue.push_back(elem.first);
            }else{
                glfwMakeContextCurrent(elem.first);
                glClear(GL_COLOR_BUFFER_BIT);
                glRasterPos2d(-1, 1);
                glPixelZoom(1, -1);
                glDrawPixels(elem.second.width, elem.second.height, GL_RGBA,  GL_UNSIGNED_BYTE, elem.second.current_pixelmap.data());
                glFlush();
                glfwSwapBuffers(elem.first);
                glfwPollEvents();
            }
        }

        for(auto& elem: delete_queue){
            glfwDestroyWindow(elem);
            _data.erase(elem);
        }

        return true;
    }

} _window_map;


void _error_callback(int error, const char* description)
{
    std::cerr<<"GLFW Error: "<< description<<std::endl;
}

void glfw_windows_init(){
    assert(glfwInit());
    glfwSetErrorCallback(_error_callback);
}

GLFWwindow* create_window(int width, int height, const std::string& title){
    GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    assert(window);
    _window_map.add_window(window, width, height);
    return window;
}

void apply_pixelmap(GLFWwindow* wnd, void* data, unsigned int size){
    _window_map.update_content(wnd, data, size);
}

void destroy_window(GLFWwindow* wnd){
    _window_map.remove_window(wnd);
}

void glfw_windows_do_event_loop(std::atomic<bool>& exit_signal){
    while(_window_map.render_all_windows()){
        if(exit_signal.load()){
            break;
        }
    }
}


