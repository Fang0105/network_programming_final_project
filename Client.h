#ifndef __FINAL_PROJECT_CLIENT__
#define __FINAL_PROJECT_CLIENT__

#include "structures.h"
#include <vector>

class Client {
    private:
    in_addr server_ip_addr;
    UserData data;
    int connection_fd;
    std::vector<RoomData> rooms;

    //General
    void Close_connetion();

    //Central Server Functions
    void Connect_central_server();
    void Central_loop();    

    void Print_commands();

    void Request_room_list();
    void Print_room_list();

    int Build_room();

    //Room Server Functions
    bool Join_room(int target_room);
    void Room_loop();

    public:
    Client(const std::string server_ip_addr);
    ~Client();

    int Run();
};

#endif