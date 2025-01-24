/* discard */

#include "unp.h"
#include <string.h>

#define BUILD_A_NEW_ROOM "A"
#define LIST_ALL_ROOMS "B"
#define JOIN_A_ROOM "C"



int main(int argc, char **argv){

    int sockfd;
    struct sockaddr_in servaddr;

    if (argc != 2)
		err_quit("usage: tcpcli <IPaddress>");

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

    char send_buffer[MAXLINE];
    snprintf(send_buffer, sizeof(send_buffer), "%s : %s\n", JOIN_A_ROOM, "adasdasd");

    Writen(sockfd, send_buffer, strlen(send_buffer));

    printf("%s", send_buffer);


    return 0;
}


