#include "Client.h"
#include "RoomServer.hpp"
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>

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
        command = getchar();

        switch(command) {
            case 'a': case 'A':
            Build_room();
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
    cout << "A - Build a new room\n";
    cout << "B - List all rooms\n";
    cout << "C - Join a room\n";
    cout << "Q - Quit\n";
    cout << "Enter your choice: ";
}

int Client::Run() {
    set_scr();
    clr_scr();
    
    std::string input;
    std::cout << "Enter your username: ";
    std::cin >> input;
    data.name = input;

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