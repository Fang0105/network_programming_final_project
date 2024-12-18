#ifndef __FINAL_PROJECT_ROOM_SERVER__
#define __FINAL_PROJECT_ROOM_SERVER__

#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <string>
#include <vector>
#include <thread>
#include "structures.h"
#include <cstring>
#include <iostream>
#include <atomic>
#include <chrono>

void Close(int fd) {
    if (close(fd) < 0) {
        fprintf(stderr, "fd: %d", fd);
        perror("close");
    }
}

void setRecvTimeout(int socket_fd, int seconds, int microseconds) {

    printf("Setting socket timeout\n");
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting socket timeout");
    } else {
        printf("Timeout set to %d seconds and %d microseconds\n", seconds, microseconds);
    }
}

class RoomServer {
    private:
        sockaddr_in addr;
        int accept_user_port;
        int receive_video_port; // accept_user_port + 1
        int receive_audio_port; // accept_user_port + 2
        int send_video_port;    // accept_user_port + 3
        int send_audio_port;    // accept_user_port + 4
        //int message_listen_port; // accept_user_port + 5
        int test_port; // accept_user_port + 6

        int maxfdp1 = 0;

        sockaddr_in cliaddr;
        socklen_t clilen;

        

        // tcp
        int accept_user_listen_socket;
        int accept_user_accept_socket;
        // int message_listen_socket;
        int test_socket;

        // udp
        int receive_video_socket;
        int receive_audio_socket;
        int send_video_socket;
        int send_audio_socket;

        // std::vector<sockaddr_in> audience_address;
        // sockaddr_in host_address;

        fd_set master_set, rset;

        std::vector<ClientData> all_clients;

        //bool ready_to_end = false;
        //std::atomic<bool> ready_to_end(false);
        std::atomic<bool> ready_to_end{false};  // 使用直接初始化


        // threads
        std::thread audio_thread;
        std::thread video_thread;



    public:

        bool is_alive = true;

        RoomServer(struct sockaddr_in addr, int port) {
            accept_user_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
            printf("accept_user_listen_socket : %d\n", accept_user_listen_socket);

            addr.sin_port = htons(port);

            this->accept_user_port = port;
            this->receive_video_port = port + 1;
            this->receive_audio_port = port + 2;
            this->send_video_port = port + 3;
            this->send_audio_port = port + 4;
            //this->message_listen_port = port + 5; //merged with accept_user_port
            this->test_port = port + 6;
            this->addr = addr;

            int bind_status = bind(accept_user_listen_socket, (sockaddr*) &this->addr, sizeof(this->addr));
            printf("bind status : %d\n", bind_status);

            test_socket = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in test_addr = addr;
            test_addr.sin_port = htons(test_port);
            bind(test_socket, (sockaddr*) &test_addr, sizeof(test_addr));

            maxfdp1 = std::max(maxfdp1, accept_user_listen_socket + 1);

            ready_to_end = false;
        }

        void AcceptUser(UserData user, sockaddr_in user_addr, int connfd) {
            
            if(user.identity == IDENT_AUDIENCE){
                printf("Accept audience\n");
                printf("id : %d, name : %s\n\n", user.id, user.name);

                ClientData client;
                client.id = user.id;
                strcpy(client.name, user.name);
                client.is_online = true;
                client.connfd = connfd;
                client.identity = IDENT_AUDIENCE;
                client.address = user_addr;

                // audience_address.push_back(user_addr);
                all_clients.push_back(client);

                FD_SET(connfd, &master_set);

                maxfdp1 = std::max(maxfdp1, connfd + 1);
            }else if(user.identity == IDENT_PROVIDER){
                printf("Accept host\n");
                // host_address = user_addr;

                ClientData client;
                client.id = user.id;
                strcpy(client.name, user.name);
                client.is_online = true;
                client.connfd = connfd;
                client.identity = IDENT_PROVIDER;
                client.address = user_addr;

                all_clients.push_back(client);

                FD_SET(connfd, &master_set);

                maxfdp1 = std::max(maxfdp1, connfd + 1);
            }else if(user.identity == IDENT_NONE){
                printf("Error user\n");
            }else{
                printf("Undefined user\n");
            }


            printf("all users:\n");
            for(const ClientData& client : all_clients){
                printf("id : %d, name : %s, connfd : %d, is_online : %d\n", client.id, client.name, client.connfd, client.is_online);
            }
            printf("maxfdp1 : %d\n", maxfdp1);

        }

        void receiveAndSendAudio() {
            receive_audio_socket = socket(AF_INET, SOCK_DGRAM, 0);
            if(receive_audio_socket < 0){
                printf("Error create receive audio socket\n");
            }else{
                printf("Create receive audio socket\n");
                setRecvTimeout(receive_audio_socket, 1, 0);  // 設置 1 秒超時
            }

            send_audio_socket = socket(AF_INET, SOCK_DGRAM, 0);
            if(send_audio_socket < 0){
                printf("Error create send audio socket\n");
            }else{
                printf("Create send audio socket\n");
            }

            sockaddr_in receive_audio_addr = {};
            receive_audio_addr.sin_family = AF_INET;
            receive_audio_addr.sin_addr.s_addr = addr.sin_addr.s_addr;
            receive_audio_addr.sin_port = htons(receive_audio_port);

            if( (bind(receive_audio_socket, (sockaddr*) &receive_audio_addr, sizeof(receive_audio_addr))) < 0){
                printf("Error bind receive audio socket\n");
            }else{
                printf("Bind receive audio socket\n");
            }

            float buffer[FRAMES_PER_BUFFER];
            while(!ready_to_end){
                printf("waiting audio\n");
                ssize_t receivedSize = recvfrom(receive_audio_socket, buffer, sizeof(buffer), 0, nullptr, nullptr);
                // if(receivedSize < 0){
                //     printf("Error receive audio\n");
                // }

                if (receivedSize < 0) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        printf("Audio receive timeout, retrying...\n");
                        continue;  // 超時則重試
                    } else {
                        perror("Error receiving audio");
                        break;
                    }
                }

                // for(sockaddr_in audience : audience_address){
                //     sendto(send_audio_socket, buffer, sizeof(buffer), 0, (sockaddr*) &audience, sizeof(audience));
                // }
                for(const ClientData& client : all_clients){
                    if(client.is_online && client.identity == IDENT_AUDIENCE){
                        sendto(send_audio_socket, buffer, sizeof(buffer), 0, (sockaddr*) &client.address, sizeof(client.address));
                    }
                }
            }

            printf("A\n");
            Close(receive_audio_socket);
            printf("B\n");
            Close(send_audio_socket);
            printf("End receive and send audio\n");
        }

        void receiveAndSendVideo(){
            receive_video_socket = socket(AF_INET, SOCK_DGRAM, 0);
            if(receive_video_socket < 0){
                printf("Error create receive video socket\n");
            }else{
                printf("Create receive video socket\n");
                setRecvTimeout(receive_video_socket, 1, 0);  // 設置 1 秒超時
            }

            send_video_socket = socket(AF_INET, SOCK_DGRAM, 0);
            if(send_video_socket < 0){
                printf("Error create send video socket\n");
            }else{
                printf("Create send video socket\n");
            }

            sockaddr_in receive_video_addr = {};
            receive_video_addr.sin_family = AF_INET;
            receive_video_addr.sin_addr.s_addr = addr.sin_addr.s_addr;
            receive_video_addr.sin_port = htons(receive_video_port);

            if( (bind(receive_video_socket, (sockaddr*) &receive_video_addr, sizeof(receive_video_addr)) ) < 0){
                printf("Error bind receive video socket\n");
            }else{
                printf("Bind receive video socket\n");
            }

            float buffer[FRAMES_PER_BUFFER];
            while(!ready_to_end){
                printf("waiting video\n");
                ssize_t receivedSize = recvfrom(receive_video_socket, buffer, sizeof(buffer), 0, nullptr, nullptr);
                // if(receivedSize < 0){
                //     printf("Error receive video\n");
                // }

                if (receivedSize < 0) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        printf("Video receive timeout, retrying...\n");
                        continue;  // 超時則重試
                    } else {
                        perror("Error receiving video");
                        break;
                    }
                }

                // for(sockaddr_in audience : audience_address){
                //     sendto(send_video_socket, buffer, sizeof(buffer), 0, (sockaddr*) &audience, sizeof(audience));
                // }

                for(const ClientData& client : all_clients){
                    if(client.is_online && client.identity == IDENT_AUDIENCE){
                        sendto(send_video_socket, buffer, sizeof(buffer), 0, (sockaddr*) &client.address, sizeof(client.address));
                    }
                }
            }

            printf("C\n");
            Close(receive_video_socket);
            printf("D\n");
            Close(send_video_socket);
            printf("End receive and send video\n");
        }


        void receiveUserAndReceiveAndSendMessage(){

            printf("running on port : %d\n", accept_user_port);


            printf("Start receiveing user\n");

            listen(accept_user_listen_socket, LISTENQ);
            listen(test_socket, LISTENQ);

            FD_ZERO(&master_set);
            FD_SET(accept_user_listen_socket, &master_set);
            FD_SET(test_socket, &master_set);

            while(true){
                rset = master_set;
                select(maxfdp1, &rset, NULL, NULL, NULL);

                if(FD_ISSET(accept_user_listen_socket, &rset)){

                    clilen = sizeof(cliaddr);
                    accept_user_accept_socket = accept(accept_user_listen_socket, (sockaddr*)&cliaddr, &clilen);

                    char buffer[sizeof(UserData)];
                    recv(accept_user_accept_socket, buffer, sizeof(buffer), 0);

                    UserData user;
                    deserialize_UserData(buffer, user);

                    AcceptUser(user, cliaddr, accept_user_accept_socket);

                }else if(FD_ISSET(test_socket, &rset)){
                    int tem_connfd = accept(test_socket, NULL, NULL);
                    close(tem_connfd);
                }else{
                    int l = all_clients.size();
                    for(int i=0;i<l;i++){
                        if(FD_ISSET(all_clients[i].connfd, &rset) && all_clients[i].is_online){
                            char recv_buffer[1024];
                            int status = recv(all_clients[i].connfd, recv_buffer, sizeof(recv_buffer), 0);
                            
                            if(status <= 0){
                                printf("Client close connection\n");
                                all_clients[i].is_online = false;
                                FD_CLR(all_clients[i].connfd, &master_set);
                                close(all_clients[i].connfd);

                                if(all_clients[i].identity == IDENT_PROVIDER){
                                    printf("Host close connection\n");
                                    for(int j=0;j<all_clients.size();j++){
                                        if(all_clients[j].is_online){
                                            char send_buffer[1024];
                                            snprintf(send_buffer, sizeof(send_buffer), "[Host left]\n");
                                            send(all_clients[j].connfd, send_buffer, sizeof(send_buffer), 0);
                                        }
                                    }

                                    //TODO: do end room section
                                    ready_to_end = true;
                                    // ready_to_end.store(true);
                                    printf("hahahahahaahahah666666666\n");
                                    printf("ready to end\n");

                                    return;
                                }

                            }else{
                                recv_buffer[status] = '\0';
                                char send_buffer[1024];
                                snprintf(send_buffer, sizeof(send_buffer), "(%s) %s", all_clients[i].name, recv_buffer);
                                printf("send : %s\n", send_buffer);
                                for(int j=0;j<all_clients.size();j++){
                                    if(i != j && all_clients[j].is_online){
                                        send(all_clients[j].connfd, send_buffer, sizeof(send_buffer), 0);
                                    }
                                }
                            }

                            

                        }
                    }
                }
            }
        }



        void run(){
            //ready_to_end.store(false);
            audio_thread = std::thread(&RoomServer::receiveAndSendAudio, this);
            video_thread = std::thread(&RoomServer::receiveAndSendVideo, this);
            receiveUserAndReceiveAndSendMessage();
            //printf("22222222222\n");
            

            audio_thread.join();
            video_thread.join();

            printf("Room server end byebye byebye byebye byebye\n");
            end();
            printf("all Room server end\n");
            
            
            // audio_thread.join();
            // video_thread.join();

            
        }


        void end(){

            //printf("hahahahaahahahah\n");
            printf("Zzzzzzz\n");
            Close(accept_user_listen_socket);
            
            for(ClientData client : all_clients){
                if(client.is_online){
                    Close(client.connfd);
                }
                
            }
            printf("66666666666 end byebye byebye byebye byebye\n");
        }
};

#endif