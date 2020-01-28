
/* client.c

   (c) Thomas Azelis 2012
*/

#include "utils.h"

#define SERVER_PORT     6767
#define BUF_SIZE        1024
#define MAXHOSTNAMELEN  1024

/*=========================================================*
 *                          main                           *
 *=========================================================*/
int main(int argc, char **argv)
{
    struct sockaddr_in server_addr;
    struct hostent *host_info;
    int socket_fd, numbytes;
    char buffer[BUF_SIZE];

    if (argc != 3) {
        printf("usage: %s <hostname> <message>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // get the host (server) info
    if ((host_info = gethostbyname(argv[1])) == NULL)
        ERROR("gethostbyname()"); 

    // create socket
    if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        ERROR("socket()");

    // create socket adress of server (type, IP-adress and port number)
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr = *((struct in_addr*)host_info->h_addr);
    server_addr.sin_port = htons(SERVER_PORT);

    // connect to the server
    if (connect(socket_fd, (struct sockaddr*) &server_addr, 
		sizeof(server_addr)) == -1)
        ERROR("connect()");

    // send message
    write_str_to_socket(socket_fd, argv[2], strlen(argv[2]));

    // receive message
    numbytes = read_str_from_socket(socket_fd, buffer, BUF_SIZE);
    printf("Reply: %s\n", buffer); // print to stdout

    // close the connection to the server
    close(socket_fd);

    return 0;
}

