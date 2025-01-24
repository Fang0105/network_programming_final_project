#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

#define LISTEN_IP "127.0.0.1"
#define LISTEN_PORT 8888
#define BUFFER_SIZE 1400
#define HEADER_SIZE 8  // 4 bytes for Frame ID, 2 for Chunk Number, 2 for Total Chunks

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return -1;
    }

    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(LISTEN_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
        close(sock);
        return -1;
    }

    std::unordered_map<uint32_t, std::vector<std::vector<uchar>>> frameChunks;
    std::unordered_map<uint32_t, size_t> frameChunkCounts;

    while (true) {
        std::vector<uchar> packet(BUFFER_SIZE);
        ssize_t receivedSize = recvfrom(sock, packet.data(), BUFFER_SIZE, 0, nullptr, nullptr);
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
                cv::imshow("Received Frame", frame);
                cv::waitKey(1);
            }

            frameChunks.erase(frameID);
            frameChunkCounts.erase(frameID);
        }
    }

    close(sock);
    return 0;
}
