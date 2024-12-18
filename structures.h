#ifndef __FINAL_PROJECT_STRUCTURES__
#define __FINAL_PROJECT_STRUCTURES__

#include <string>
#include <cstring>
#include <netinet/in.h>

#define LISTENQ 1024
#define FRAMES_PER_BUFFER 512
#define SERV_PORT 10000

enum Identiy {IDENT_NONE, IDENT_AUDIENCE, IDENT_PROVIDER};

struct UserData {
    int id = -1;                     // server give
    char name[16];           // user provided
    Identiy identity = IDENT_NONE;   // 0: not set, 1: audience, 2: provider
};

std::string to_string(const UserData& d) {
    return  "name: " + std::string(d.name) + "\n" + 
            "identity: " + std::to_string(int(d.identity)) + "\n";
}

struct RoomData {
    int room_id;
    char room_name[16];
    UserData host_user;
    int running_port;
};

std::string to_string(const RoomData& d) {
    return  "room_id: " + std::to_string(d.room_id) + "\n" + 
            "room_name: " + std::string(d.room_name) + "\n" +
            "running_port:" + std::to_string(d.running_port) + "\n" +
            "host_user: \n" + to_string(d.host_user) + "\n";
}

struct ClientData {
    int id;
    char name[1024];
    bool is_online;
    int connfd;
    Identiy identity;
    sockaddr_in address;
};

enum CommandType {CREATE_ROOM, LIST_ROOM, JOIN_ROOM};

struct Command{
    CommandType type;
    UserData user; // store user data for host information
    int room_id; // for audience to JOIN_ROOM
    char room_name[16]; // for host to CREATE_ROOM
};

void serialize_UserData(const UserData &obj, char *buffer) {
    memcpy(buffer, &obj, sizeof(UserData));
}

void deserialize_UserData(const char *buffer, UserData &obj) {
    memcpy(&obj, buffer, sizeof(UserData));
}

void serialize_Command(const Command &obj, char *buffer) {
    memcpy(buffer, &obj, sizeof(Command));
}

void deserialize_Command(const char *buffer, Command &obj) {
    memcpy(&obj, buffer, sizeof(Command));
}

void serialize_RoomData(const RoomData &obj, char *buffer) {
    memcpy(buffer, &obj, sizeof(RoomData));
}

void deserialize_RoomData(const char *buffer, RoomData &obj) {
    memcpy(&obj, buffer, sizeof(RoomData));
}

void serialize_Number(int number, char *buffer) {
    memcpy(buffer, &number, sizeof(int));
}

void deserialize_Number(const char *buffer, int& number) {
    memcpy(&number, buffer, sizeof(int));
}

#endif