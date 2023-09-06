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

const int DEFAULT_PORT = 4560;
const int BUFFER_SIZE = 4096;

std::queue<std::string> commandQueue;  // Shared command queue
std::mutex queueMutex;                 // Mutex for command queue
std::condition_variable dataCond; 

std::string receiveData(int clientSocket) 
{
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead <= 0)
        return "";
    return std::string(buffer, bytesRead);
}
std::vector<char> receiveBinaryData(int clientSocket) 
{
    char buffer[BUFFER_SIZE];  // Temporary buffer
    std::vector<char> fullData;  // Vector to hold the complete binary data

    ssize_t bytesRead;
    do {
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead > 0) {
            // Check for EOF marker
            if (bytesRead >= 3 && std::string(buffer + bytesRead - 3, 3) == "EOF") {
                fullData.insert(fullData.end(), buffer, buffer + bytesRead - 3);  // Exclude EOF
                break;
            }
            // Otherwise, append received bytes to the vector
            fullData.insert(fullData.end(), buffer, buffer + bytesRead);
        }
    } while (bytesRead > 0);  // Continue until socket is closed or an error occurs

    if (bytesRead < 0) {
        // Handle error
        std::cerr << "An error occurred while receiving data." << std::endl;
    }

    return fullData;  // Return the vector containing the received binary data
}
bool sendData(int clientSocket, const std::string& data) 
{
    ssize_t bytesSent = send(clientSocket, data.c_str(), data.length(), 0);
    return bytesSent == static_cast<ssize_t>(data.length());
}

std::string readDataFromStdin() 
{
    std::string input;
    std::cout << "MSG: ";
    std::getline(std::cin, input);
    return input;
}

void ShowScreenshotScreen()
{
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL couldn't initialize: " << SDL_GetError() << std::endl;
    }
    // Load image using SDL_Image
    SDL_Surface* imageSurface = IMG_Load("screenshot.png");
    if (!imageSurface)
    {
        std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    int imgWidth = imageSurface->w;
    int imgHeight = imageSurface->h;
    window = SDL_CreateWindow("Latest Screenshot", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, imgWidth, imgHeight, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        std::cerr << "Window couldn't be created: " << SDL_GetError() << std::endl;
        SDL_Quit();
    }
    // Get window surface
    screenSurface = SDL_GetWindowSurface(window);
    // Fill the surface white
    SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
    // Blit the image surface onto the window surface
    SDL_BlitSurface(imageSurface, NULL, screenSurface, NULL);
    // Update the surface
    SDL_UpdateWindowSurface(window);
    // Wait for a while to see the image
    bool quit = false;
    SDL_Event e;

    // While application is running
    while (!quit)
    {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0)
        {
            // User requests quit or presses a key
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN)
            {
                quit = true;
            }
        }
    }
    // Destroy image surface and window
    SDL_FreeSurface(imageSurface);
    SDL_DestroyWindow(window);
    // Quit SDL subsystems
    SDL_Quit();
}

void server(std::string& sharedOutput) 
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) 
    {
        std::cerr << "Failed to create server socket.\n";
        return;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(DEFAULT_PORT);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) 
    {
        std::cerr << "Failed to bind to the server socket.\n";
        close(serverSocket);
        return;
    }

    if (listen(serverSocket, SOMAXCONN) == -1) 
    {
        std::cerr << "Listen failed.\n";
        close(serverSocket);
        return;
    }

    std::cout << "Server is running.\n";
    std::string ipAddress = inet_ntoa(serverAddress.sin_addr);
    std::cout << "Chosen IP address: " << ipAddress << std::endl;

    // This is just a loop to keep the server running even if the client crashes or disconnects, allowing the client to reconnect when it comes back online
    while(true)
    {
        sockaddr_in clientAddress{};
        socklen_t clientAddressSize = sizeof(clientAddress);

        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);
        if (clientSocket == -1) 
        {
            std::cerr << "Accept failed.\n";
            continue;
        }

        std::cout << "Client connected.\n";

        while (true)
	    {
	    	std::unique_lock<std::mutex> lock(queueMutex);
            dataCond.wait(lock, []{ return !commandQueue.empty(); });  // Wait for data

            std::string command = commandQueue.front();  // Get command from shared queue
            commandQueue.pop();

            lock.unlock();

            // Now process 'command' as you would normally
            sendData(clientSocket, command);

            std::vector<char> imageData;

            if (command == "Screenshot")
	    	{
                std::cout << "Starting saving image.\n";
                sharedOutput += "Starting saving image.\n";
                imageData = receiveBinaryData(clientSocket);
                std::cout << "Binary data received! Size: " << imageData.size() << " bytes\n";
                sharedOutput += "Binary data received!\n";

                std::ofstream outFile("screenshot.png", std::ios::binary);
                if (outFile.is_open()) 
                {
                    std::cout << "Writing to file.\n";
                    sharedOutput += "Writing to file\n";
                    outFile.write(imageData.data(), imageData.size());
                    if (outFile.good()) {
                        std::cout << "Successfully wrote to the file.\n";
                        sharedOutput += "Successfully wrote to the file.\n";
                    } else {
                        std::cerr << "Failed to write all data to the file.\n";
                        sharedOutput += "Failed to write all data to the file.\n";
                    }
                    std::cout << "Closing file.\n";
                    sharedOutput += "Closing file.\n";
                    outFile.close();
                } 
                else 
                {
                    std::cerr << "Failed to open the file for writing." << std::endl;
                    sharedOutput += "Failed to open the file for writing.\n";
                }
                std::cout << "Saved image!\n";
                sharedOutput += "Saved image!\n";
                std::cout << "Showing image.\n";
                sharedOutput += "Showing image.\n";
                //ShowScreenshotScreen();
	    	}
	    	else
	    	{
	    		std::string recv = receiveData(clientSocket);
	    		if (recv.empty())
	    		{
	    			std::cout << "Client disconnected.\n";
                    sharedOutput += "Client disconnected.\n";
	    			close(clientSocket);
                    break;
	    		}

	    		std::cout << "client: " << recv << "\n";
                sharedOutput += "client: " + recv + "\n";
	    	}
	    }
    }
}

int SDL_IMGUI_GUI(std::string& sharedOutput)
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER| SDL_INIT_GAMECONTROLLER) != 0)
    {
        std::cerr << "Error" << SDL_GetError();
        return -1;
    }

    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool done = false;

    while(!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(); 
        ImGui::NewFrame();

        std::string input;
        char buffer [256];
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("KV-RAT-GUI", NULL);
            ImGui::SetWindowSize(ImVec2(100, 1500));

            std::string inputCommand;
            if (ImGui::InputText("Command", &inputCommand, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                commandQueue.push(inputCommand);  // Add command to shared queue
                dataCond.notify_one();  // Notify server thread
            }
            ImGui::Text("%s", sharedOutput.c_str());

            ImGui::End();
        }

        // 3. Show another simple window.
        /*
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }
        */

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void sdl_thread(std::string& sharedOutput)
{
    SDL_IMGUI_GUI(sharedOutput);
}

void server_thread(std::string& sharedOutput)
{
    server(sharedOutput);
}

int main(int argc, char**argv) 
{
    if(argc < 2)
    {
        printf("Usage: ./linux-server-kv-rat [GUI 0/1]\n");
    }

    std::cout << "SDL2 Is in this build, very experimental\n";

    std::string sharedOutput;
    
    std::thread sdlThread(sdl_thread, std::ref(sharedOutput));
    std::thread serverThread(server_thread, std::ref(sharedOutput));

    sdlThread.join();
    serverThread.join();

    return 0;
}