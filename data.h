#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include "external/nlohmann/json.hpp"
#include <vector>


namespace json 
{
    // Function to receive JSON data from a socket
    nlohmann::json recvData(int socket_fd);

    // Function to send JSON data to a socket
    void sendData(int socket_fd, const nlohmann::json& data_to_send);
}