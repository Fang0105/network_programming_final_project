#include "RoomServer.hpp"
#include "structures.h"
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <opencv2/opencv.hpp>
#include <portaudio.h>
#include <assert.h>

std::queue<cv::Mat> frameQueue;
std::mutex queueMutex;
std::condition_variable frameCondVar;
std::atomic<bool> stopFlag(false);

#define SERV_PORT 10000

fd_set master_set, rset;
int maxfdp1;

void handle_message(int sockfd){
    printf("handle message\n");

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

void displayFrames() {

    printf("start display\n");

    while (!stopFlag) {
        std::unique_lock<std::mutex> lock(queueMutex);
        frameCondVar.wait(lock, [] { return !frameQueue.empty() || stopFlag; });

        while (!frameQueue.empty()) {
            cv::Mat frame = frameQueue.front();
            frameQueue.pop();
            lock.unlock();

            cv::imshow("Received Frame", frame);
            if (cv::waitKey(1) == 27) {  // Stop on ESC key
                stopFlag = true;
                return;
            }

            lock.lock();
        }
    }
}

void receiveVideo(RoomData room, int port) {
    int videoSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (videoSocket < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return;
    }

    struct sockaddr_in videoAddr = {};
    videoAddr.sin_family = AF_INET;
    videoAddr.sin_port = htons(port);
    videoAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(videoSocket, (struct sockaddr *)&videoAddr, sizeof(videoAddr)) < 0) {
        std::cerr << "Failed to bind socket." << std::endl;
        close(videoSocket);
        return;
    }

    std::unordered_map<uint32_t, std::vector<std::vector<uchar>>> frameChunks;
    std::unordered_map<uint32_t, size_t> frameChunkCounts;

    while (!stopFlag) {
        std::vector<uchar> packet(BUFFER_SIZE);
        printf("try to receive video\n");
        ssize_t receivedSize = recvfrom(videoSocket, packet.data(), BUFFER_SIZE, 0, nullptr, nullptr);
        if (receivedSize < 0) {
            std::cerr << "Failed to receive data: " << strerror(errno) << std::endl;
            continue;
        }

        uint32_t frameID = ntohl(*reinterpret_cast<uint32_t*>(&packet[0]));
        uint16_t chunkNumber = ntohs(*reinterpret_cast<uint16_t*>(&packet[4]));
        uint16_t totalChunks = ntohs(*reinterpret_cast<uint16_t*>(&packet[6]));

        std::vector<uchar> chunk(packet.begin() + HEADER_SIZE, packet.begin() + receivedSize);

        frameChunks[frameID].resize(totalChunks);
        frameChunks[frameID][chunkNumber] = std::move(chunk);
        frameChunkCounts[frameID]++;

        if (frameChunkCounts[frameID] == totalChunks) {
            std::vector<uchar> completeBuffer;
            for (const auto& chunk : frameChunks[frameID]) {
                completeBuffer.insert(completeBuffer.end(), chunk.begin(), chunk.end());
            }

            cv::Mat frame = cv::imdecode(completeBuffer, cv::IMREAD_COLOR);
            if (!frame.empty()) {
                std::unique_lock<std::mutex> lock(queueMutex);
                frameQueue.push(frame);
                frameCondVar.notify_one();
            }

            frameChunks.erase(frameID);
            frameChunkCounts.erase(frameID);
        }
    }

    close(videoSocket);
}

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512

void receiveAudio(RoomData room, int port) {
    printf("lalalalalaall\n");
    int audioSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (audioSocket < 0) {
        std::cerr << "Failed to create audio socket." << std::endl;
        return;
    }
    printf("A\n");
    struct sockaddr_in audioAddr = {};
    audioAddr.sin_family = AF_INET;
    audioAddr.sin_port = htons(port);
    audioAddr.sin_addr.s_addr = INADDR_ANY;
    printf("B\n");
    if (bind(audioSocket, (struct sockaddr *)&audioAddr, sizeof(audioAddr)) < 0) {
        std::cerr << "Failed to bind audio socket." << std::endl;
        close(audioSocket);
        return;
    }
    printf("C\n");
    // Initialize PortAudio
    Pa_Initialize();
    
    
    printf("D\n");
    PaStream *stream;
    Pa_OpenDefaultStream(&stream,
                         0, // Input channels
                         1, // Output channels
                         paFloat32, // Sample format
                         SAMPLE_RATE,
                         FRAMES_PER_BUFFER,
                         nullptr, // No callback
                         nullptr); // No user data

    
    Pa_StartStream(stream);
    

    float buffer[FRAMES_PER_BUFFER];
    while (!stopFlag) {
        printf("try to lololololol\n");
        ssize_t receivedSize = recvfrom(audioSocket, buffer, sizeof(buffer), 0, nullptr, nullptr);
        if (receivedSize < 0) {
            std::cerr << "Failed to receive audio data: " << strerror(errno) << std::endl;
            break;
        }
        Pa_WriteStream(stream, buffer, FRAMES_PER_BUFFER);
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    close(audioSocket);
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
    user.id = 16666;
    strcpy(user.name, "nody_client");
    user.identity = IDENT_AUDIENCE;

    Command command;
    command.type = LIST_ROOM;
    strcpy(command.room_name, "nody_haha_room");
    command.user = user;

    char buffer[sizeof(Command)];
    serialize_Command(command, buffer);

    send(sockfd, buffer, sizeof(buffer), 0);
    
    
    char recv_buffer_all_rooms_size[sizeof(int)];
    recv(sockfd, recv_buffer_all_rooms_size, sizeof(recv_buffer_all_rooms_size), 0);
    int all_rooms_size;
    deserialize_Number(recv_buffer_all_rooms_size, all_rooms_size);

    printf("all rooms size : %d\n", all_rooms_size);
    std::vector<RoomData> all_rooms;
    for(int i=0;i<all_rooms_size;i++){
        char recv_buffer_RoomData[sizeof(RoomData)];
        recv(sockfd, recv_buffer_RoomData, sizeof(recv_buffer_RoomData), 0);
        RoomData room;
        deserialize_RoomData(recv_buffer_RoomData, room);
        all_rooms.push_back(room);
        printf("(%d) room name : %s, host name : %s, running port : %d\n", i+1, room.room_name, room.host_user.name, room.running_port);
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

    sockaddr_in addr = {};
    int len = sizeof(addr);
    getsockname(sockfd, (sockaddr*)&addr, (socklen_t*)&len);

    char user_buffer[sizeof(UserData)];
    serialize_UserData(user, user_buffer);

    send(sockfd, user_buffer, sizeof(user_buffer), 0);

    
    FD_ZERO(&master_set);
    FD_SET(sockfd, &master_set);
    FD_SET(fileno(stdin), &master_set);
    maxfdp1 = std::max(sockfd, fileno(stdin)) + 1;

    printf("inital : %d\n", ntohs(addr.sin_port));

    
    std::thread messageThread(handle_message, sockfd);
    std::thread videoThread(receiveVideo, all_rooms[choose_room_id - 1], ntohs(addr.sin_port) + 1);
    std::thread audioThread(receiveAudio, all_rooms[choose_room_id - 1], ntohs(addr.sin_port) + 2);
    displayFrames();

    stopFlag = true;
    messageThread.join();
    videoThread.join();
    audioThread.join();


    
	
    
    


    
}