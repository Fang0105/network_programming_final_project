#include "RoomServer.hpp"
#include "structures.h"
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <opencv2/opencv.hpp>

#define SERV_PORT 10000

void sendVideo(RoomData room) {
    #define CHUNK_SIZE 1400
    #define HEADER_SIZE 8  // 4 bytes for Frame ID, 2 for Chunk Number, 2 for Total Chunks
    
    int videoSocket = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in videoAddr = {};
    videoAddr.sin_family = AF_INET;
    videoAddr.sin_port = htons(room.running_port + 1);
    inet_pton(AF_INET, "127.0.0.1", &videoAddr.sin_addr);

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Cannot open the camera" << std::endl;
        return;
    }

    uint32_t frameID = 0;

    printf("Start sending video\n");

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Failed to capture frame" << std::endl;
            break;
        }else{
            printf("Capture frame\n");
        }

        // Show the captured video
        //cv::imshow("Captured Video", frame);

        // Press 'q' to exit
        if (cv::waitKey(1) == 'q') {
            break;
        }

        cv::resize(frame, frame, cv::Size(640, 480));
        std::vector<int> compressionParams = {cv::IMWRITE_JPEG_QUALITY, 50};
        std::vector<uchar> buffer;
        cv::imencode(".jpg", frame, buffer, compressionParams);

        frameID++;
        size_t totalSize = buffer.size();
        size_t chunks = (totalSize + CHUNK_SIZE - HEADER_SIZE - 1) / (CHUNK_SIZE - HEADER_SIZE);

        for (size_t i = 0; i < chunks; ++i) {
            size_t start = i * (CHUNK_SIZE - HEADER_SIZE);
            size_t end = std::min(start + (CHUNK_SIZE - HEADER_SIZE), totalSize);
            size_t chunkSize = end - start;

            std::vector<uchar> packet(HEADER_SIZE + chunkSize);
            uint32_t* pFrameID = reinterpret_cast<uint32_t*>(&packet[0]);
            uint16_t* pChunkNumber = reinterpret_cast<uint16_t*>(&packet[4]);
            uint16_t* pTotalChunks = reinterpret_cast<uint16_t*>(&packet[6]);

            *pFrameID = htonl(frameID);
            *pChunkNumber = htons(i);
            *pTotalChunks = htons(chunks);

            std::copy(buffer.begin() + start, buffer.begin() + end, packet.begin() + HEADER_SIZE);

            if (sendto(videoSocket, packet.data(), packet.size(), 0, (struct sockaddr*)&videoAddr, sizeof(videoAddr)) < 0) {
                std::cerr << "Failed to send chunk " << i + 1 << "/" << chunks << ": " << strerror(errno) << std::endl;
            }
        }
    }

    close(videoSocket);
}












int main(){
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

	connect(sockfd, (sockaddr*) &servaddr, sizeof(servaddr));

    UserData user;
    user.id = 1;
    strcpy(user.name, "nody");
    user.identity = IDENT_PROVIDER;

    Command command;
    command.type = CREATE_ROOM;
    strcpy(command.room_name, "nody_haha_room");
    command.user = user;

    char buffer[sizeof(Command)];
    serialize_Command(command, buffer);

    send(sockfd, buffer, sizeof(buffer), 0);


    char recv_buffer[sizeof(RoomData)];
    recv(sockfd, recv_buffer, sizeof(recv_buffer), 0);

    RoomData room;
    deserialize_RoomData(recv_buffer, room);

    close(sockfd);
    
    // sleep(10);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //sockaddr_in servaddr;
    // sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(room.running_port);
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

    std::thread videoThread(sendVideo, room);

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