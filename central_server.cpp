#include <string>
#include <vector>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "structures.h"

const int SERV_PORT = 10000;

void sig_chld(int signo) {
    pid_t pid;
    int stat;

    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        ;
    return;
}

class CentralServer {
    private:
    bool inited;
    int listen_fd;
    sockaddr_in server_address;
    std::vector<int> rooms_fd;
    std::vector<int> connected_user_fd;

    const int LISTENQ = 10;
    
    public:
    CentralServer() : 
        inited(false) {
        bzero(&server_address, sizeof(server_address));
        server_address.sin_family      = AF_INET;
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);
        server_address.sin_port        = htons(SERV_PORT);
    }

    void init() {
        inited = true;
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        bind(listen_fd, (sockaddr*)&server_address, sizeof(server_address));
        listen(listen_fd, LISTENQ);
    }

    void closeServer() {
        close(listen_fd);
        inited = false;
    }

    void createRoom(UserData creater) {
        //TODO
    }

    std::vector<int> listRooms() {
        return rooms_fd;
    }

    int run() {
        while(true) {
            int conn = accept(listen_fd, NULL, NULL);
            if (conn < 0) {
                if (errno == EINTR){
                    continue;
                } else {
                    printf("Central Server: Failed to accept client\n.");
                } 
            }
        }
    }
};



int main() {
    signal(SIGCHLD, sig_chld);

    CentralServer central_server;
    central_server.init();
    central_server.run();

    return 0;
}
