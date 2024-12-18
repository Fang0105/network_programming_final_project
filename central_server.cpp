#include <string>
#include <vector>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include "RoomServer.hpp"
#include "structures.h"


void sig_chld(int signo) {
    pid_t pid;
    int stat;

    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
    return;
}

class CentralServer {
    private:
        bool inited;
        int listen_fd;
        sockaddr_in server_address;
        std::vector<int> rooms_fd;
        std::vector<int> connected_user_fd;

        std::vector<RoomData> all_rooms;
        int room_id_counter = 1;
    
    public:
        CentralServer() : inited(false) {
            bzero(&server_address, sizeof(server_address));
            server_address.sin_family      = AF_INET;
            server_address.sin_addr.s_addr = htonl(INADDR_ANY);
            server_address.sin_port        = htons(SERV_PORT);
        }

        void init() {
            inited = true;
            listen_fd = socket(AF_INET, SOCK_STREAM, 0);
            bind(listen_fd, (sockaddr*)&server_address, sizeof(server_address));
            printf("Central Server: Server started\n");
        }

        void closeServer() {
            close(listen_fd);
            inited = false;
        }

        RoomData createRoom(std::string room_name, UserData host) {
            RoomData room;
            room.room_id = room_id_counter++;
            strcpy(room.room_name, room_name.c_str());
            room.host_user = host;
            room.running_port = SERV_PORT + 8 * room.room_id; // 8 ports for a room server
            all_rooms.push_back(room);

            printf("Central Server: [%s] Room created, the host is [%s].\n", room_name.c_str(), host.name);

            return room;
        }

        std::vector<int> listRooms() {
            return rooms_fd;
        }

        void runningRoom(RoomData room) {

            // use fork to create a new process
            pid_t pid = fork();
            if (pid == 0) {
                close(listen_fd);
                RoomServer room_server(server_address, room.running_port);
                room_server.run();

                // printf("remove room id = %d\n", room.room_id);
                // for(int i=0;i<all_rooms_child.size();i++){
                //     if(all_rooms_child[i].room_id == room.room_id){
                //         printf("%d stop\n", i);
                //         all_rooms_child.erase(all_rooms_child.begin() + i);
                //         break;
                //     }
                // }
                // printf("success remove room id = %d\n", room.room_id);
                // printf("all rooms size = %d\n", all_rooms_child.size());


                exit(0);

            } else if (pid > 0) {
            } else {
                printf("Central Server: Failed to create room server\n");
            }
            
        }

        int run() {
            printf("Central Server: Running\n");
            listen(listen_fd, LISTENQ);


            while(true) {
                int conn_fd = accept(listen_fd, NULL, NULL);
                if (conn_fd < 0) {
                    if (errno == EINTR){
                        continue;
                    } else {
                        printf("Central Server: Failed to accept client\n.");
                    } 
                }else{
                    
                    char buffer[sizeof(Command)];
                    int status = recv(conn_fd, buffer, sizeof(buffer), 0);
                    if(status < 0){
                        printf("Central Server: Failed to receive command\n");
                    }

                    Command command;
                    deserialize_Command(buffer, command);

                    if(command.type == CREATE_ROOM){
                        printf("Central Server: Create Room\n");
                        RoomData room = createRoom(command.room_name, command.user);

                        // send the room data to the host
                        char send_buffer[sizeof(RoomData)];
                        serialize_RoomData(room, send_buffer);
                        send(conn_fd, send_buffer, sizeof(send_buffer), 0);

                        runningRoom(room);

                        close(conn_fd);
                    }else if(command.type == LIST_ROOM){
                        printf("Central Server: List Room\n");
                        
                        int pid = fork();
                        if(pid == 0){
                            close(listen_fd);
                            
                            std::vector<RoomData> rooms_still_alive;    
                            for(int i=0;i<all_rooms.size();i++){
                                // test whether the room is alive
                                int test_socket = socket(AF_INET, SOCK_STREAM, 0);
                                sockaddr_in test_addr;
                                bzero(&test_addr, sizeof(test_addr));
                                test_addr.sin_family = AF_INET;
                                test_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                                test_addr.sin_port = htons(all_rooms[i].running_port + 6);
                                if(connect(test_socket, (sockaddr*)&test_addr, sizeof(test_addr)) < 0){
                                    printf("room id = %d is not alive\n", all_rooms[i].room_id);
                                }else{
                                    printf("room id = %d is alive\n", all_rooms[i].room_id);
                                    rooms_still_alive.push_back(all_rooms[i]);
                                }      
                                close(test_socket);
                            }

                            all_rooms = rooms_still_alive;

                            int all_rooms_size;
                            all_rooms_size = all_rooms.size();
                            char send_buffer_all_rooms_size[sizeof(int)];
                            serialize_Number(all_rooms_size, send_buffer_all_rooms_size);
                            send(conn_fd, send_buffer_all_rooms_size, sizeof(send_buffer_all_rooms_size), 0);

                            for(int i=0;i<all_rooms.size();i++){
                                char send_buffer_RoomData[sizeof(RoomData)];
                                serialize_RoomData(all_rooms[i], send_buffer_RoomData);
                                send(conn_fd, send_buffer_RoomData, sizeof(send_buffer_RoomData), 0);
                            }


                            close(conn_fd);
                            exit(0);
                        }
                        close(conn_fd);

                    }else if(command.type == JOIN_ROOM){
                        printf("Central Server: Join Room\n");
                    }else{
                        printf("Central Server: Error Command\n");
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
