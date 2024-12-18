#ifndef __FINAL_PROJECT_CLIENT__
#define __FINAL_PROJECT_CLIENT__

#include "structures.h"
#include <vector>

class Client {
    private:
    in_addr server_ip_addr;
    std::string username;
    int connection_fd;
    std::vector<RoomData> rooms;
    const int BUFFER_SIZE = 4096;

    //General
    void Close_connetion();

    //Central Server Functions
    void Connect_central_server();
    void Central_loop();    

    void Print_commands();
    
    void Create_and_join_room(const std::string& room_name);

    void Request_room_list();
    void Print_room_list();

    bool Join_room_by_port(int port, Identiy user_type);
    bool Join_room(int target_room);
    void Room_loop();

    //Room Server Functions
    void Handle_message();
    void Send_audio();
    void Send_video();
    void Receive_audio();
    void Receive_video();

    public:
    Client(const std::string server_ip_addr);
    ~Client();

    int Run();
};

#endif