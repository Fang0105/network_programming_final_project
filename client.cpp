#include "RoomServer.hpp"
#include "structures.h"
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>

#define BUILD_A_NEW_ROOM "A"
#define LIST_ALL_ROOMS "B"
#define JOIN_A_ROOM "C"

using namespace std;

#define SERV_PORT 10000
#define MAXLINE 10000

char send_buffer[MAXLINE];
char recv_buffer[MAXLINE];


/* the following two functions use ANSI Escape Sequence */
/* refer to https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797 */
void clr_scr() {
	printf("\x1B[2J");
};

void set_scr() {		// set screen to 80 * 25 color mode
	printf("\x1B[=3h");
};


void task(FILE *fp, int sockfd);

int main(int argc, char **argv){

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;

    if (argc != 2){
        printf("usage: tcpcli <IPaddress>");
        return 0;
    }
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	connect(sockfd, (sockaddr*) &servaddr, sizeof(servaddr));

    set_scr();
    clr_scr();
    
    task(stdin,sockfd);
    


    return 0;
}

void task(FILE *fp, int sockfd){
    
    client current_client;
    string input;
    current.set_identity(ERROR);

    cout << "Enter your username: ";
    cin >> input;
    current_client.set_name(input);

    /////////////////////////////
    // To do 
    // Set id from central server
    current_client.set_id(123456);

    fd_set master_set, rset;
    FD_ZERO(&master_set);
    FD_SET(sockfd, &master_set);
    FD_SET(fileno(stdin), &master_set);
    int maxfdp1 = max(sockfd, fileno(stdin)) + 1;
    
    // Command Loop
    while (true) {
        // Show menu if we're waiting for input
        cout << "\nOptions:\n";
        cout << "A - Build a new room\n";
        cout << "B - List all rooms\n";
        cout << "C - Join a room\n";
        cout << "Q - Quit\n";
        cout << "Enter your choice: ";

        rset = master_set;
        select(maxfdp1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &rset)) {
            // Handle incoming messages from server
            char recv_buffer[1024];
            int status = recv(sockfd, recv_buffer, sizeof(recv_buffer), 0);
            if (status == 0) {
                printf("Server closed connection\n");
                break;
            }
            printf("Received: %s\n", recv_buffer);
        }
        
        if (FD_ISSET(fileno(stdin), &rset)) {
            // Handle user input
            string input;
            cin >> input;

            if (input == "Q" || input == "q") {
                cout << "Exiting...\n";
                break;
            }

            // Process commands
            if (input == BUILD_A_NEW_ROOM) {
                // Set identity as PROVIDER when creating a room
                current_client.set_identity(PROVIDER);
                
                // Serialize and send updated user data
                char buffer[sizeof(UserData)];
                serialize_UserData(current_client, buffer);
                send(sockfd, buffer, sizeof(buffer), 0);
                
                // Add room creation logic here
                cout << "Creating new room...\n";
                // Add specific room creation code
            }
            else if (input == LIST_ALL_ROOMS || input == JOIN_A_ROOM) {
                // Set identity as AUDIENCE when listing or joining rooms
                if (current_client.get_identity() != PROVIDER) {
                    current_client.set_identity(AUDIENCE);
                    
                    // Serialize and send updated user data
                    char buffer[sizeof(UserData)];
                    serialize_UserData(current_client, buffer);
                    send(sockfd, buffer, sizeof(buffer), 0);
                }

                if (input == LIST_ALL_ROOMS) {
                    send(sockfd, "LIST", 4, 0);
                } else {
                    cout << "Enter room ID to join: ";
                    string room_id;
                    cin >> room_id;
                    string join_cmd = "JOIN " + room_id;
                    send(sockfd, join_cmd.c_str(), join_cmd.length(), 0);
                }
            }

            // Send regular message if in a room
            if (!input.empty() && input[0] != 'A' && input[0] != 'B' && input[0] != 'C') {
                // Only allow sending messages if identity is set
                if (current_client.get_identity() != ERROR) {
                    int status = send(sockfd, input.c_str(), input.length(), 0);
                    if (status < 0) {
                        printf("Error sending message\n");
                    }
                } else {
                    cout << "Please select an action (A/B/C) first to determine your role.\n";
                }
            }
        }
    }
}