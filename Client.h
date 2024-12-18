#ifndef __FINAL_PROJECT_CLIENT__
#define __FINAL_PROJECT_CLIENT__

#include "structures.h"

class Client {
    private:
    UserData data;

    public:
    Client() = default;
    ~Client() = default;

    void set_name(const std::string& name) { data.name = name; }
    void set_id(int id) {data.id = id; }
    void set_identity(Identiy identity ) { data.identity = identity; }

    std::string get_name() {return data.name ;}
    int         get_id() {return data.id ;}
    int         get_identity () {return data.identity ;}      
    

    UserData get_data() const { return data; }
};

#endif