#include "RoomServer.hpp"
#include "structures.h"
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>

#define SERV_PORT 1000

int main(){
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT+1);
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

	connect(sockfd, (sockaddr*) &servaddr, sizeof(servaddr));

    UserData user;
    user.id = 101;
    std::cin >> user.name;
    //user.name = "Nody_audience";
    user.identity = AUDIENCE;

    char buffer[sizeof(UserData)];
    serialize(user, buffer);

    send(sockfd, buffer, sizeof(buffer), 0);

    fd_set master_set, rset;
    FD_ZERO(&master_set);
    FD_SET(sockfd, &master_set);
    FD_SET(fileno(stdin), &master_set);
    int maxfdp1 = std::max(sockfd, fileno(stdin)) + 1;

    while(true){
        rset = master_set;
        select(maxfdp1, &rset, NULL, NULL, NULL);

        if(FD_ISSET(sockfd, &rset)){
            char buffer[1024];
            int status = recv(sockfd, buffer, sizeof(buffer), 0);
            if(status == 0){
                printf("Server close connection\n");
                break;
            }
            printf("%s\n", buffer);
        }else if(FD_ISSET(fileno(stdin), &rset)){
            char buffer[1024];
            fgets(buffer, sizeof(buffer), stdin);
            int status = send(sockfd, buffer, strlen(buffer), 0);
            if(status < 0){
                printf("Error send message\n");
            }
        }
    }


}