#ifndef __FINAL_PROJECT_STRUCTURES__
#define __FINAL_PROJECT_STRUCTURES__

#include <string>

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
};



#endif