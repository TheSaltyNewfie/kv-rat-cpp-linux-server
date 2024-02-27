#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include "external/nlohmann/json.hpp"
#include <vector>

namespace json 
{

    // Function to receive JSON data from a socket
    nlohmann::json recvData(int socket_fd) 
    {
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);

        if (bytes_received < 0)
        {
            std::cerr << "Error in recvData(). Exiting." << std::endl;
            exit(EXIT_FAILURE);
        }

        nlohmann::json received_json = nlohmann::json::parse(buffer);

        return received_json;
    }

    // Function to send JSON data to a socket
    void sendData(int socket_fd, const nlohmann::json& data_to_send) 
    {
        std::string serialized_data = data_to_send.dump();
        int bytes_sent = send(socket_fd, serialized_data.c_str(), serialized_data.length(), 0);

        if (bytes_sent < 0) 
        {
            std::cerr << "Error in send(). Exiting." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

namespace file
{
    
}