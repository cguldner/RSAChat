#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <thread>

#include "shared.h"

using namespace std;


const char *SERVER_IP = "localhost";
const char *SERVER_PORT = "3000";
const string CLIENT_NAME = "Janet";

struct sockaddr_storage their_addr; // connector's address information
int sockfd;  // listen on sock_fd, new connection on new_fd

void listenForMessagesFromServer() {
    int numbytes;
    char buf[MAXDATASIZE];
    while(1) {
        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
            diep("Couldn't correctly get the data");
        }
        if (numbytes == 0) {
            diep("Server no longer connected, your buddy has left </3");
        }

        buf[numbytes] = '\0';

        printf("client: received '%s'\n",buf);
    }
}

void listenForUserInput() {
    while (1) {
        char message[MAXDATASIZE];
        cin.getline(message, MAXDATASIZE);

        if (send(sockfd, message, strlen(message), 0) == -1) {
            perror("send");
        }
    }
}

void openTCPPort() {
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char addr_info[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(SERVER_IP, SERVER_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: Issue with the sockets");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) {
        cout << "Failed to connect to the server, try again later" << endl;
        exit(1);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), addr_info, sizeof addr_info);
    printf("client: connecting to %s\n", addr_info);

    freeaddrinfo(servinfo); // all done with this structure

    thread messageListener(listenForMessagesFromServer);
    thread messageSender(listenForUserInput);
    while(1);
}

int main(int argc, char** argv) {
    if (argc != 1) {
        cout << "Usage: " << argv[0] << endl;
        exit(1);
    }

    openTCPPort();
}
