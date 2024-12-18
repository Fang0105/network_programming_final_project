#ifndef __FINAL_PROJECT_STRUCTURES__
#define __FINAL_PROJECT_STRUCTURES__

#include <string>

#define LISTENQ 1024
#define FRAMES_PER_BUFFER 512

enum Identiy {ERROR, AUDIENCE, PROVIDER};

struct UserData {
    int id;                 // server give
    std::string name;       // user provided
    Identiy identity;       // 1 : audience 2 : provider 0 : not set
};

struct RoomData {
    int room_id;
    std::string room_name;
    UserData host_user;
    int running_port;
};


void serialize_UserData(const UserData &obj, char *buffer) {
    memcpy(buffer, &obj, sizeof(UserData));
}

void deserialize_UserData(const char *buffer, UserData &obj) {
    memcpy(&obj, buffer, sizeof(UserData));
}

enum CommandType {CREATE_ROOM, LIST_ROOM, JOIN_ROOM};

struct Command{
    CommandType type;
    UserData user; // store user data for host information
    int room_id; // for audience to JOIN_ROOM
    std::string room_name; // for host to CREATE_ROOM
};

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

struct Number {
    int num;
};

void serialize_Number(const Number &obj, char *buffer) {
    memcpy(buffer, &obj, sizeof(Number));
}

void deserialize_Number(const char *buffer, Number &obj) {
    memcpy(&obj, buffer, sizeof(Number));
}

#endif