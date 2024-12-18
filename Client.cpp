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


void handle_sigchld(int signo) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0); // Reap zombie processes
    return;
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
    strcpy(cmd.room_name, room_name.c_str());
    cmd.user.id = -1; //TODO: id is not implemented and have no usage.
    strcpy(cmd.user.name, username.c_str());
    cmd.user.identity = IDENT_PROVIDER;

    char buffer[BUFFER_SIZE];
    //Send request    
    serialize_Command(cmd, buffer);
    if(write(connection_fd, buffer, sizeof(buffer)) < 0) {
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

    if(!Join_room_by_port(new_room.running_port, IDENT_PROVIDER)) {
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
    cout << "-------------------------\n";
    for(int i = 0; i < rooms.size(); i++) {
        cout << std::setfill(' ') << std::setw(3) << i;
        cout << "\t" << std::string(rooms[i].room_name) << "\t" << std::string(rooms[i].host_user.name) << "\n\n";
    }
}

bool Client::Join_room_by_port(int port, Identiy user_type) {
    //Tries to connect to the room
    int new_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in roomaddr;
    bzero(&roomaddr, sizeof(roomaddr));
    roomaddr.sin_family = AF_INET;
    roomaddr.sin_addr = server_ip_addr;
    roomaddr.sin_port = htons(port);

    int tryCount = 5; //Retry every second, for maximum of tryCount times.
    while(tryCount--) {        
        if(connect(new_sock_fd, (sockaddr*)&roomaddr, sizeof(roomaddr)) >= 0)
            break; //Connection succeeded

        perror("Retry");
        close(new_sock_fd);
        new_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        sleep(1);
    }
    if(tryCount < 0) {
        cout << "[Client][Warning] Join_room_by_port(): cannot connect to the room at port "<< port << "\n";
        cout << "Cannot connect to the room at port. Please try to list rooms again.\n";
        return false;
    }

    Close_connetion(); //Close current connection with central server
    connection_fd = new_sock_fd; //Switch current connection to the new room;

    char buffer[BUFFER_SIZE];
    UserData user;
    user.id = -1;
    user.identity = user_type;
    strcpy(user.name, username.c_str());
    //Send user data
    serialize_UserData(user, buffer);
    if(write(connection_fd, buffer, sizeof(UserData)) < 0) {
        std::cerr << "[Client][Error] Join_room_by_port(): failed to send user data to server\n";
        perror("[Client][Error] Join_room_by_port()");
        return false;
    }

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

    return Join_room_by_port(target_room_data.running_port, IDENT_AUDIENCE);
}

void Client::Room_loop() {
    std::string input;
    while(true) {
        cin >> input;
        if(write(connection_fd, input.c_str(), input.size()) < 0) {
            std::cerr << "[Client][Error] Room_loop(): failed to send message\n";
            perror("[Client][Error] Room_loop()");
            return;
        }
    }
}

// void Client::Handle_message() {
//     fd_set set, rset;
//     FD_ZERO(&set);
//     FD_SET(&connection_fd, );
// }


void Client::Handle_message() {

    fd_set set, rset;
    FD_ZERO(&set);
    FD_SET(connection_fd , &set);
    FD_SET(fileno(stdin), &set);
    int maxfdp1 = std::max(connection_fd, fileno(stdin)) + 1;

    while(true){
        rset = set;
        select(maxfdp1, &rset, NULL, NULL, NULL);

        if(FD_ISSET(connection_fd, &rset)){
            char buffer[1024];
            int status = recv(connection_fd, buffer, sizeof(buffer), 0);
            if(status == 0){
                printf("Server close connection\n");
                break;
            }
            printf("%s\n", buffer);
        }else if(FD_ISSET(fileno(stdin), &rset)){
            char buffer[1024];
            fgets(buffer, sizeof(buffer), stdin);
            int status = send(connection_fd, buffer, strlen(buffer), 0);
            if(status < 0){
                printf("Error send message\n");
            }
        }
    }

}



void Send_audio();
void Send_video();
void Receive_audio();
void Receive_video();

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