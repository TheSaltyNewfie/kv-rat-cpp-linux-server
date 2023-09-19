#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <vector>
#include <arpa/inet.h>
#include "external/SDL2/SDL.h"
#include "external/SDL2/SDL_image.h"
#include "external/imgui/imgui.h"
#include "external/imgui/imgui_impl_sdl2.h"
#include "external/imgui/imgui_impl_opengl3.h"
#include "external/imgui/imgui_stdlib.h"
#include <stdio.h>
#include "external/SDL2/SDL_opengl.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "external/nlohmann/json.hpp"

void load_texture(const char* path);
void imgui_client_screen();
void imgui_command_window(bool* done, std::queue<std::string> &commandQueue, std::mutex &queueMutex, std::condition_variable &dataCond);
void imgui_command_logs_window(std::string& sharedOutput);
int SDL_IMGUI_GUI(std::string& sharedOutput, int& mx, int& my, std::queue<std::string> &commandQueue, std::mutex &queueMutex, std::condition_variable &dataCond);
