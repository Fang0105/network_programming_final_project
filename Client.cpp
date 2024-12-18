#include "Client.h"
#include "RoomServer.hpp"
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <iomanip>

using std::cin;
using std::cout;

/* the following two functions use ANSI Escape Sequence */
/* refer to https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797 */
inline void clr_scr() {
	printf("\x1B[2J");
};

inline void set_scr() {		// set screen to 80 * 25 color mode
	printf("\x1B[=3h");
};

inline int max(int a, int b) {
    return (a > b) ? a : b;
}

Client::Client(const std::string server_ip_addr) :
    connection_fd(-1) {
    inet_pton(AF_INET, server_ip_addr.c_str(), &(this->server_ip_addr));
}

Client::~Client() {
    Close_connetion();
}

inline void Client::Close_connetion() {
    if(connection_fd == -1) return;

    if(close(connection_fd) < 0) {
        perror("[Client][Error] Closefd()");
        fprintf(stderr, "[Client][Error] Closefd(): Error fd=%d", connection_fd);
    } else {
        connection_fd = -1;
    }
}

void Client::Connect_central_server() {
    Close_connetion(); // end any previous connection
    connection_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	servaddr.sin_addr = server_ip_addr;

	if(connect(connection_fd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("[Client][Error] Connect_central_server()");
    }
}

void Client::Central_loop() {
    char command;
    bool loop = true;
    clr_scr();

    while(loop) {
        // Show menu        
        Print_commands();
        cin >> command;
        std::string room_name;

        switch(command) {
            case 'a': case 'A':
            cout << "Enter the room's name: ";
            cin >> room_name;
            Create_and_join_room(room_name);
            break;
            
            case 'b': case 'B':
            Request_room_list();
            Print_room_list();
            break;

            case 'c': case 'C':            
            int room;
            cout << "Please enter the room number you want to join:";
            cin >> room;
            Join_room(room);
            break;

            case 'q': case 'Q':
            loop = false;
            break;

            default:
            cout << "Unknown command: " << command << "\n";
        }
    }
}

inline void Client::Print_commands() {
    cout << "\nCommands:\n";
    cout << "A - Create a new room\n";
    cout << "B - List all rooms\n";
    cout << "C - Join a room\n";
    cout << "Q - Quit\n";
    cout << "Enter your choice: ";
}

void Client::Create_and_join_room(const std::string& room_name) {
    Command cmd;
    cmd.type = CREATE_ROOM;
    cmd.room_name = room_name;
    cmd.user.id = -1; //TODO: id is not implemented and have no usage.
    cmd.user.name = username;
    cmd.user.identity = IDENT_PROVIDER;

    char buffer[BUFFER_SIZE];
    //Send request    
    serialize_Command(cmd, buffer);
    if(write(connection_fd, buffer, sizeof(Command)) < 0) {
        std::cerr << "[Client][Error] Create_and_join_room(): failed to send request to server\n";
        perror("[Client][Error] Create_and_join_room()");
        return;
    }

    //Get response
    if(read(connection_fd, buffer, sizeof(RoomData)) < 0) {
        std::cerr << "[Client][Error] Create_and_join_room(): failed to get response from server\n";
        perror("[Client][Error] Create_and_join_room()");
        return;
    }

    //Join room
    RoomData new_room;
    printf("buffer: %s\n", buffer);
    deserialize_RoomData(buffer, new_room);
    cout << "[Client][Debug] Create_and_join_room(): Received response\n" << to_string(new_room);

    if(!Join_room_by_port(new_room.running_port)) {
        std::cerr << "[Client][Error] Create_and_join_room(): failed to join new room\n";
    }
}

void Client::Request_room_list() {
    Command cmd;
    cmd.type = LIST_ROOM;
    
    char buffer[BUFFER_SIZE];
    //Send request    
    serialize_Command(cmd, buffer);
    if(write(connection_fd, buffer, sizeof(Command)) < 0) {
        std::cerr << "[Client][Error] Request_room_list(): failed to send request to server\n";
        perror("[Client][Error] Request_room_list()");
        return;
    }

    //Get response
    if(read(connection_fd, buffer, sizeof(int)) < 0) {
        std::cerr << "[Client][Error] Request_room_list(): failed to get room count from server\n";
        perror("[Client][Error] Request_room_list()");
        return;
    }
    
    int room_count;
    RoomData new_room;
    deserialize_Number(buffer, room_count);
    rooms.resize(room_count);
    for(int i = 0; i < room_count; i++) {
        if(read(connection_fd, buffer, sizeof(RoomData)) < 0) {
            std::cerr << "[Client][Error] Request_room_list(): failed to get room from server\n";
            perror("[Client][Error] Request_room_list()");
            return;
        }

        deserialize_RoomData(buffer, new_room);
        cout << "[Client][Debug] Request_room_list(): Received response\n" << to_string(new_room);
        rooms[i] = new_room;
    }
}

inline void Client::Print_room_list() {
    cout << " # \tRoom Name\tHost\n";
    cout << "-------------------------";
    for(int i = 0; i < rooms.size(); i++) {
        cout << std::setfill(' ') << std::setw(3) << i;
        cout << "\t" << rooms[i].room_name << "\t" << rooms[i].host_user.name << "\n\n";
    }
}

bool Client::Join_room_by_port(int port) {
    //Tries to connect to the room
    int new_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in roomaddr;
    bzero(&roomaddr, sizeof(roomaddr));
    roomaddr.sin_family = AF_INET;
    roomaddr.sin_addr = server_ip_addr;
    roomaddr.sin_port = port;

    if(connect(new_sock_fd, (sockaddr*)&roomaddr, sizeof(roomaddr)) < 0) {
        cout << "[Client][Warning] Join_room_by_port(): cannot connect to the room at port "<< port << "\n";
        return false;
    }

    Close_connetion(); //Close current connection with central server
    connection_fd = new_sock_fd; //Switch current connection to the new room;

    Room_loop();

    Connect_central_server(); //Reconnect to central server after leaving the room
    return true;
}

bool Client::Join_room(int target_room) {
    RoomData target_room_data;
    try {
        target_room_data = rooms.at(target_room);
    } catch (const std::out_of_range&) {
        cout << target_room << " is not on the room list.";
        return false;
    }

    return Join_room_by_port(target_room_data.running_port);
}

void Client::Room_loop() {
    std::cout << "Fuck byebye room\n";
}

int Client::Run() {
    set_scr();
    clr_scr();
    
    std::string input;
    std::cout << "Enter your username: ";
    std::cin >> input;
    username = input;

    Connect_central_server();
    Central_loop();
    Close_connetion();

    return 0;    
}

int main(int argc, char** argv) {
    if(argc > 2) {
        fprintf(stderr, "Usage: ./%s [server address]", argv[0]);
        exit(1);
    }

    std::string server_address;
    if(argc == 1) {
        std::cout << "Please enter the server's IP address: ";
        std::cin >> server_address;
    } else if(argc == 2) {
        server_address = argv[1];
    }

    Client client(server_address);
    client.Run();
}