#ifndef __FINAL_PROJECT_CLIENT__
#define __FINAL_PROJECT_CLIENT__

#include "structures.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <unordered_map>
#include <atomic>
#include <opencv2/opencv.hpp>

class Client {
    private:
    in_addr server_ip_addr;
    UserData user_data;
    std::string username;
    Identiy identity;
    int connection_fd;
    int connection_port;
    std::vector<RoomData> rooms;
    std::atomic<bool> keepLoop;

    //For video & audio
    std::queue<cv::Mat> frameQueue;
    std::mutex queueMutex;
    std::condition_variable frameCondVar;

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
    void Display_frames();
    
    public:
    Client(const std::string server_ip_addr);
    ~Client();

    int Run();
};

#endif