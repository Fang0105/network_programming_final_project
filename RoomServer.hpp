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

    public:
    RoomServer(const std::string& ip, int port) {

    }

    void AcceptUser() {

    }

    void Boradcast() {
        
    }
};

#endif