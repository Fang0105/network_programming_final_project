#include "Client.h"
#include "RoomServer.hpp"
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <iomanip>
#include <assert.h>

using std::cin;
using std::cout;

/* the following two functions use ANSI Escape Sequence */
/* refer to https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797 */
inline void clr_scr() {
	printf("\x1B[2J");
};

inline void set_scr() {		// set screen to 80 * 25 color mode
	printf("\x1B[=3h");
};

inline int max(int a, int b) {
    return (a > b) ? a : b;
}

Client::Client(const std::string server_ip_addr) :
    connection_fd(-1) {
    inet_pton(AF_INET, server_ip_addr.c_str(), &(this->server_ip_addr));
}

Client::~Client() {
    Close_connetion();
}

inline void Client::Close_connetion() {
    if(connection_fd == -1) return;

    if(close(connection_fd) < 0) {
        perror("[Client][Error] Closefd()");
        fprintf(stderr, "[Client][Error] Closefd(): Error fd=%d", connection_fd);
    } else {
        connection_fd = -1;
    }
}

void Client::Connect_central_server() {
    Close_connetion(); // end any previous connection
    connection_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	servaddr.sin_addr = server_ip_addr;

	if(connect(connection_fd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("[Client][Error] Connect_central_server()");
    }
}

void Client::Central_loop() {
    char command;
    bool loop = true;
    clr_scr();

    while(loop) {
        // Show menu        
        Print_commands();
        cin >> command;
        std::string room_name;

        switch(command) {
            case 'a': case 'A':
            cout << "Enter the room's name: ";
            cin >> room_name;
            Create_and_join_room(room_name);
            break;
            
            case 'b': case 'B':
            Request_room_list();
            Print_room_list();
            break;

            case 'c': case 'C':            
            int room;
            cout << "Please enter the room number you want to join:";
            cin >> room;
            Join_room(room);
            break;

            case 'q': case 'Q':
            loop = false;
            break;

            default:
            cout << "Unknown command: " << command << "\n";
        }
    }
}

inline void Client::Print_commands() {
    cout << "\nCommands:\n";
    cout << "A - Create a new room\n";
    cout << "B - List all rooms\n";
    cout << "C - Join a room\n";
    cout << "Q - Quit\n";
    cout << "Enter your choice: ";
}

void Client::Create_and_join_room(const std::string& room_name) {
    Command cmd;
    cmd.type = CREATE_ROOM;
    strcpy(cmd.room_name, room_name.c_str());
    identity = IDENT_PROVIDER;
    strcpy(cmd.user.name, username.c_str());
    cmd.user.identity = identity;

    char buffer[BUFFER_SIZE];
    //Send request    
    serialize_Command(cmd, buffer);
    if(write(connection_fd, buffer, sizeof(buffer)) < 0) {
        std::cerr << "[Client][Error] Create_and_join_room(): failed to send request to server\n";
        perror("[Client][Error] Create_and_join_room()");
        return;
    }

    //Get response
    if(read(connection_fd, buffer, sizeof(RoomData)) < 0) {
        std::cerr << "[Client][Error] Create_and_join_room(): failed to get response from server\n";
        perror("[Client][Error] Create_and_join_room()");
        return;
    }

    //Join room
    RoomData new_room;
    printf("buffer: %s\n", buffer);
    deserialize_RoomData(buffer, new_room);
    cout << "[Client][Debug] Create_and_join_room(): Received response\n" << to_string(new_room);
    
    if(!Join_room_by_port(new_room.running_port, IDENT_PROVIDER)) {
        std::cerr << "[Client][Error] Create_and_join_room(): failed to join new room\n";
    }
}

void Client::Request_room_list() {
    Command cmd;
    cmd.type = LIST_ROOM;
    
    char buffer[BUFFER_SIZE];
    //Send request    
    serialize_Command(cmd, buffer);
    if(write(connection_fd, buffer, sizeof(Command)) < 0) {
        std::cerr << "[Client][Error] Request_room_list(): failed to send request to server\n";
        perror("[Client][Error] Request_room_list()");
        return;
    }

    //Get response
    if(read(connection_fd, buffer, sizeof(int)) < 0) {
        std::cerr << "[Client][Error] Request_room_list(): failed to get room count from server\n";
        perror("[Client][Error] Request_room_list()");
        return;
    }
    
    int room_count;
    RoomData new_room;
    deserialize_Number(buffer, room_count);
    rooms.resize(room_count);
    for(int i = 0; i < room_count; i++) {
        if(read(connection_fd, buffer, sizeof(RoomData)) < 0) {
            std::cerr << "[Client][Error] Request_room_list(): failed to get room from server\n";
            perror("[Client][Error] Request_room_list()");
            return;
        }

        deserialize_RoomData(buffer, new_room);
        cout << "[Client][Debug] Request_room_list(): Received response\n" << to_string(new_room);
        rooms[i] = new_room;
    }
}

inline void Client::Print_room_list() {
    cout << " # \tRoom Name\tHost\n";
    cout << "-------------------------\n";
    for(int i = 0; i < rooms.size(); i++) {
        cout << std::setfill(' ') << std::setw(3) << i;
        cout << "\t" << std::string(rooms[i].room_name) << "\t" << std::string(rooms[i].host_user.name) << "\n\n";
    }
}

bool Client::Join_room_by_port(int port, Identiy user_type) {
    //Tries to connect to the room
    int new_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in roomaddr;
    bzero(&roomaddr, sizeof(roomaddr));
    roomaddr.sin_family = AF_INET;
    roomaddr.sin_addr = server_ip_addr;
    roomaddr.sin_port = htons(port);    

    int tryCount = 5; //Retry every second, for maximum of tryCount times.
    while(tryCount--) {      
        printf("type: %d\n", user_type);  
        if(connect(new_sock_fd, (sockaddr*)&roomaddr, sizeof(roomaddr)) >= 0)
            break; //Connection succeeded
        
        close(new_sock_fd);
        new_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        sleep(1);
    }
    if(tryCount < 0) {
        cout << "[Client][Warning] Join_room_by_port(): cannot connect to the room at port "<< port << "\n";
        cout << "Cannot connect to the room at port. Please try to list rooms again.\n";
        return false;
    }
    
    Close_connetion(); //Close current connection with central server
    connection_fd = new_sock_fd; //Switch current connection to the new room;
    connection_port = port;

    char buffer[BUFFER_SIZE];
    UserData ud;
    identity = user_type;
    ud.identity = identity;
    strcpy(ud.name, username.c_str());
    //Send user data
    serialize_UserData(ud, buffer);
    if(write(connection_fd, buffer, sizeof(UserData)) < 0) {
        std::cerr << "[Client][Error] Join_room_by_port(): failed to send user data to server\n";
        perror("[Client][Error] Join_room_by_port()");
        return false;
    }
    
    Room_loop();

    Connect_central_server(); //Reconnect to central server after leaving the room
    return true;
}

bool Client::Join_room(int target_room) {
    RoomData target_room_data;
    try {
        target_room_data = rooms.at(target_room);
    } catch (const std::out_of_range&) {
        cout << target_room << " is not on the room list.";
        return false;
    }

    return Join_room_by_port(target_room_data.running_port, IDENT_AUDIENCE);
}

void Client::Room_loop() {
    std::thread videoThread;
    if(identity == IDENT_AUDIENCE) videoThread = std::thread(&Client::Receive_video, this);
    if(identity == IDENT_PROVIDER) videoThread = std::thread(&Client::Send_video, this);
    if(identity == IDENT_AUDIENCE) Display_frames();
    videoThread.join();
}


void Client::Handle_message() {

    fd_set set, rset;
    FD_ZERO(&set);
    FD_SET(connection_fd , &set);
    FD_SET(fileno(stdin), &set);
    int maxfdp1 = std::max(connection_fd, fileno(stdin)) + 1;

    while(true){
        rset = set;
        select(maxfdp1, &rset, NULL, NULL, NULL);

        if(FD_ISSET(connection_fd, &rset)){
            char buffer[1024];
            int status = recv(connection_fd, buffer, sizeof(buffer), 0);
            if(status == 0){
                printf("Server close connection\n");
                break;
            }
            printf("%s\n", buffer);
        }else if(FD_ISSET(fileno(stdin), &rset)){
            char buffer[1024];
            fgets(buffer, sizeof(buffer), stdin);
            int status = send(connection_fd, buffer, strlen(buffer), 0);
            if(status < 0){
                printf("Error send message\n");
            }
        }
    }

}



void Client::Send_audio() {

}

void Client::Send_video() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in vid_addr;
    bzero(&vid_addr, sizeof(vid_addr));
    vid_addr.sin_family = AF_INET;
    vid_addr.sin_addr = server_ip_addr;
    vid_addr.sin_port = htons(connection_port + 1);    

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Cannot open the camera" << std::endl;
        return;
    }

    uint32_t frameID = 0;
    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Failed to capture frame" << std::endl;
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

            if (sendto(sockfd, packet.data(), packet.size(), 0, (struct sockaddr*)&vid_addr, sizeof(vid_addr)) < 0) {
                std::cerr << "Failed to send chunk " << i + 1 << "/" << chunks << ": " << strerror(errno) << std::endl;
            }
        }
    }

    close(sockfd);
}

void Client::Receive_audio() {

}

void Client::Receive_video() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in vid_addr;
    int len = sizeof(vid_addr);
    bzero(&vid_addr, sizeof(vid_addr));
    getsockname(connection_fd, (sockaddr*)&vid_addr, (socklen_t*)&len);
    vid_addr.sin_port = htons(ntohs(vid_addr.sin_port) + 1);    

    if (bind(sockfd, (struct sockaddr *)&vid_addr, sizeof(vid_addr)) < 0) {
        std::cerr << "Failed to bind socket." << std::endl;
        close(sockfd);
        return;
    }

    std::unordered_map<uint32_t, std::vector<std::vector<uchar>>> frameChunks;
    std::unordered_map<uint32_t, size_t> frameChunkCounts;

    while (true) {
        std::vector<uchar> packet(BUFFER_SIZE);
        ssize_t receivedSize = recvfrom(sockfd, packet.data(), BUFFER_SIZE, 0, nullptr, nullptr);
        if (receivedSize < 0) {
            std::cerr << "Failed to receive data: " << strerror(errno) << std::endl;
            continue;
        }
        std::cout << "Recv: " << receivedSize << "\n";

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

    close(sockfd);
}

void Client::Display_frames() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        frameCondVar.wait(lock, [this] { return !frameQueue.empty(); });
        while (!frameQueue.empty()) {
            cv::Mat frame = frameQueue.front();
            frameQueue.pop();
            lock.unlock();
            cv::imshow("Received Frame", frame);
            if (cv::waitKey(1) == 27) {  // Stop on ESC key
                return;
            }

            lock.lock();
        }
    }
}

int Client::Run() {
    set_scr();
    clr_scr();
    
    std::string input;
    std::cout << "Enter your username: ";
    std::cin >> input;
    username = input;

    Connect_central_server();
    Central_loop();
    Close_connetion();

    return 0;    
}

int main(int argc, char** argv) {
    if(argc > 2) {
        fprintf(stderr, "Usage: ./%s [server address]", argv[0]);
        exit(1);
    }

    std::string server_address;
    if(argc == 1) {
        std::cout << "Please enter the server's IP address: ";
        std::cin >> server_address;
    } else if(argc == 2) {
        server_address = argv[1];
    }

    Client client(server_address);
    client.Run();
}