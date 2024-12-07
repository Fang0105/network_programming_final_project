#ifndef __FINAL_PROJECT_ROOM_SERVER__
#define __FINAL_PROJECT_ROOM_SERVER__

#include "unp.h"
#include <string>
#include <vector>

class RoomServer {
    private:
        sockaddr_in addr;
        std::vector<int> users_fd;
        int presenter_fd;
        int port;

        int accept_user_socket;

    public:
        RoomServer(struct sockaddr_in addr, int port) {
            accept_user_socket = socket(AF_INET, SOCK_STREAM, 0);

            addr.sin_port = htons(port);

            this->port = port;
            this->addr = addr;

            Bind(accept_user_socket, (SA *) &this->addr, sizeof(this->addr));
        }

        void AcceptUser() {

        }

        void Boradcast() {
            
        }
};

#endif