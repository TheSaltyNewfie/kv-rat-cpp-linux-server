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

const int DEFAULT_PORT = 4560;
const int BUFFER_SIZE = 4096;

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

void server() 
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
	    	std::string inputData = readDataFromStdin();
	    	sendData(clientSocket, inputData);

            std::vector<char> imageData;

            if (inputData == "Screenshot")
	    	{
                std::cout << "Starting saving image.\n";
                imageData = receiveBinaryData(clientSocket);
                std::cout << "Binary data received! Size: " << imageData.size() << " bytes\n";

                std::ofstream outFile("screenshot.png", std::ios::binary);
                if (outFile.is_open()) 
                {
                    std::cout << "Writing to file.\n";
                    outFile.write(imageData.data(), imageData.size());
                    if (outFile.good()) {
                        std::cout << "Successfully wrote to the file.\n";
                    } else {
                        std::cerr << "Failed to write all data to the file.\n";
                    }
                    std::cout << "Closing file.\n";
                    outFile.close();
                } 
                else 
                {
                    std::cerr << "Failed to open the file for writing." << std::endl;
                }
                std::cout << "Saved image!\n";
                std::cout << "Showing image.\n";
                ShowScreenshotScreen();
	    	}
            else if(inputData == "LoadLatestScreenshot")
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
	    	else
	    	{
	    		std::string recv = receiveData(clientSocket);
	    		if (recv.empty())
	    		{
	    			std::cout << "Client disconnected.\n";
	    			close(clientSocket);
                    break;
	    		}

	    		std::cout << "client: " << recv << "\n";
	    	}
	    }
    }
}

int main() 
{
    std::cout << "SDL2 Is in this build, very experimental\n";
    server();
    return 0;
}

/*
        
*/
