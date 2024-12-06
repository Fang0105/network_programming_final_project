#include "unp.h"
#include <string.h>

#define BUILD_A_NEW_ROOM "A"
#define LIST_ALL_ROOMS "B"
#define JOIN_A_ROOM "C"


void sig_chld(int signo) {
    pid_t pid;
    int stat;

    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        ;
    return;
}

void buildANewRoom(){

}

void listAllRooms(){

}

void joinARoom(){

}

int main(){

    int listen_socket;
    listen_socket = Socket(AF_INET, SOCK_STREAM, 0);

    printf("socket : %d\n", listen_socket);

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
	server_address.sin_family      = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port        = htons(SERV_PORT);

    Bind(listen_socket, (SA *) &server_address, sizeof(server_address));

    struct sockaddr_in local_address;
    socklen_t len = sizeof(local_address);
    if (getsockname(listen_socket, (SA *)&local_address, &len) == -1) {
        perror("getsockname failed");
        // close(listen_socket);
        // exit(EXIT_FAILURE);
    }

    // 輸出綁定的本地地址和端口
    printf("Socket bound to IP: %s, Port: %d\n", inet_ntoa(local_address.sin_addr), ntohs(local_address.sin_port));

    Listen(listen_socket, LISTENQ);

    Signal(SIGCHLD, sig_chld);

    struct sockaddr_in client_address;
    socklen_t client_address_length;
    int connect_socket;





    while(1){
        client_address_length = sizeof(client_address);
        connect_socket = accept(listen_socket, (SA *) &client_address, &client_address_length);
        if ( connect_socket < 0) {
            if (errno == EINTR){
                continue;
            }else{
                printf("error : %d\n", errno);
                err_sys("accept error");
            } 
        }

        //pid_t childpid;
        pid_t childpid;
        char receive_buffer[MAXLINE];
        if( (childpid = Fork()) == 0){
            Close(listen_socket);

            int n = Readline(connect_socket, receive_buffer, MAXLINE);
            if(n < -1){
                err_sys("read error");
            }

            if(strncmp(receive_buffer, BUILD_A_NEW_ROOM, 1) == 0){
                printf("%s : %s\n", BUILD_A_NEW_ROOM, receive_buffer);
                buildANewRoom();
            }else if(strncmp(receive_buffer, LIST_ALL_ROOMS, 1) == 0){
                printf("%s : %s\n", LIST_ALL_ROOMS, receive_buffer);
                listAllRooms();
            }else if(strncmp(receive_buffer, JOIN_A_ROOM, 1) == 0){
                printf("%s : %s\n", JOIN_A_ROOM, receive_buffer);
                joinARoom();
            }else{
                printf("Undefined request\n");
            }

            exit(0);
        }
        Close(connect_socket);
    }



    return 0;
}
