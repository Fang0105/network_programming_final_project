#include "unp.h"
#include <iostream>
#include <sys/socket.h>

#define BUILD_A_NEW_ROOM "A"
#define LIST_ALL_ROOMS "B"
#define JOIN_A_ROOM "C"

using namespace std;

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

// sending usage
struct user_data {
    int id;                 // server give
    std::string name;       // user provided
    int identity;           // 1 : audience 2 : provider 0 : not set
    
};

class client {
private:
    user_data data;
public:

    client() = default;
    ~client() = default;

    void    set_name(const std::string& name) { data.name = name; }
    void    set_id(int id) {data.id = id; }
    void    set_identity(int id) { data.identity = id; }

    string  get_name() {return data.name ;}
    int     get_id() {return data.id ;}
    int     get_indentity () {return data.identity ;}      
    

    user_data get_data() const { return data; }
};


void tesk(FILE *fp, int sockfd);

int main(int argc, char **argv){

    int sockfd;
    struct sockaddr_in servaddr;
    if (argc != 2)
		err_quit("usage: tcpcli <IPaddress>");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

    set_scr();
    clr_scr();
    
    task(stdin,sockfd);
    


    return 0;
}

void task(FILE *fp, int sockfd){
    client current_client;
    string input;

    cout << "Enter your username: ";
    cin >> input;
    current_client.set_name(input);

    // To receive
    // client reveive;
    // read(fd, buffer, sizeof(client));
    // memcpy(reveive, buffer, sizeof(client));

    Write(sockfd, &current_client, sizeof(current_client));

    // Wait for server confirmation
    // server return id for this client 
    Read(sockfd, recv_buffer, MAXLINE);
    cout << "Server Response: " << recv_buffer << endl;


    // Command Loop
    while (true) {
        cout << "\nOptions:\n";
        cout << "A - Build a new room\n";
        cout << "B - List all rooms\n";
        cout << "C - Join a room\n";
        cout << "Q - Quit\n";
        cout << "Enter your choice: ";

        cin >> input;

        if (input == "Q" || input == "q") {
            cout << "Exiting...\n";
            break;
        }

        if (input == BUILD_A_NEW_ROOM) {
            cout << "Enter room name: ";
            cin >> input;
            snprintf(send_buffer, sizeof(send_buffer), "BUILD %s", input.c_str());
        } else if (input == LIST_ALL_ROOMS) {
            snprintf(send_buffer, sizeof(send_buffer), "LIST");
        } else if (input == JOIN_A_ROOM) {
            cout << "Enter room ID to join: ";
            cin >> input;
            snprintf(send_buffer, sizeof(send_buffer), "JOIN %s", input.c_str());
        } else {
            cout << "Invalid choice. Try again.\n";
            continue;
        }

        // Send the command to the server
        Write(sockfd, send_buffer, strlen(send_buffer));

        // Receive the server's response
        ssize_t n = Read(sockfd, recv_buffer, MAXLINE);
        recv_buffer[n] = '\0';
        cout << "Server Response: " << recv_buffer << endl;
    }
}