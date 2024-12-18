#include "RoomServer.hpp"
#include "structures.h"
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>

#define SERV_PORT 10000

int main(){
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

	connect(sockfd, (sockaddr*) &servaddr, sizeof(servaddr));

    UserData user;
    user.id = 16666;
    user.name = "nody_client";
    user.identity = AUDIENCE;

    Command command;
    command.type = LIST_ROOM;
    command.room_name = "nody_haha_room";
    command.user = user;

    char buffer[sizeof(Command)];
    serialize_Command(command, buffer);

    send(sockfd, buffer, sizeof(buffer), 0);
    
    
    char recv_buffer_all_rooms_size[sizeof(Number)];
    recv(sockfd, recv_buffer_all_rooms_size, sizeof(recv_buffer_all_rooms_size), 0);
    Number all_rooms_size;
    deserialize_Number(recv_buffer_all_rooms_size, all_rooms_size);

    printf("all rooms size : %d\n", all_rooms_size.num);
    std::vector<RoomData> all_rooms;
    for(int i=0;i<all_rooms_size.num;i++){
        char recv_buffer_RoomData[sizeof(RoomData)];
        recv(sockfd, recv_buffer_RoomData, sizeof(recv_buffer_RoomData), 0);
        RoomData room;
        deserialize_RoomData(recv_buffer_RoomData, room);
        all_rooms.push_back(room);
        printf("(%d) room name : %s, host name : %s, running port : %d\n", i+1, room.room_name.c_str(), room.host_user.name.c_str(), room.running_port);
    }

    int choose_room_id;
    printf("Choose a room id : ");
    scanf("%d", &choose_room_id);


    // clean the enter in the buffer
    char c;
    while((c = getchar()) != '\n' && c != EOF);




    close(sockfd);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(all_rooms[choose_room_id - 1].running_port);
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    
    while(true){
        int status = connect(sockfd, (sockaddr*) &servaddr, sizeof(servaddr));
        if(status == 0){
            printf("Connect to room server success\n");
            break;
        }else{
            // sockaddr_in servaddr;
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            //sleep(1);  // 重試間隔
        }
    }

    char user_buffer[sizeof(UserData)];
    serialize_UserData(user, user_buffer);

    send(sockfd, user_buffer, sizeof(user_buffer), 0);

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