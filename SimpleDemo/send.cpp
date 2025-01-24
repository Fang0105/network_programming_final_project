#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define CHUNK_SIZE 1400
#define HEADER_SIZE 8  // 4 bytes for Frame ID, 2 for Chunk Number, 2 for Total Chunks

int main() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Cannot open the camera" << std::endl;
        return -1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return -1;
    }

    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    uint32_t frameID = 0;

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Failed to capture frame" << std::endl;
            break;
        }

        // Show the captured video
        cv::imshow("Captured Video", frame);

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

            if (sendto(sock, packet.data(), packet.size(), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                std::cerr << "Failed to send chunk " << i + 1 << "/" << chunks << ": " << strerror(errno) << std::endl;
            }
        }
    }

    close(sock);
    cap.release();
    cv::destroyAllWindows();
    return 0;
}
