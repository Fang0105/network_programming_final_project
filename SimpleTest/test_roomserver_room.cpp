#include "RoomServer.hpp"
#include "structures.h"
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>

#define SERV_PORT 1000

int main(){
    sockaddr_in room_addr;
    bzero(&room_addr, sizeof(room_addr));

    room_addr.sin_family = AF_INET;
    room_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    room_addr.sin_port = htons(SERV_PORT);

    RoomServer room(room_addr, SERV_PORT + 1);

    room.run();
}