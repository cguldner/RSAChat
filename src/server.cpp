#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/wait.h>
#include <signal.h>
#include <map>
#include <vector>
#include <sstream>

#include <boost/lexical_cast.hpp>

#include "rsa.h"
#include "shared.h"

using namespace std;

const char *SERVER_PORT = "3000";
string server_name, client_name;

struct sockaddr_storage their_addr; // connector's address information
int sockfd;  // listen on sock_fd, new connection on new_fd

map<int, pair<pthread_t, pthread_t>> threadLookup;

void sigchld_handler(int s) {
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

void *listenForMessagesFromClient(void *new_fd) {
    int numbytes, totalBytes;
    char buf[MAXDATASIZE+1];
    int fd_int = *((int *) new_fd);
    while(1) {
        string data_rcv;
        totalBytes = 0;
        do {
            if ((numbytes = recv(fd_int, buf, MAXDATASIZE, 0)) == -1) {
                diep("Couldn't correctly get the data");
            }
            if (numbytes == 0) {
                perror((client_name + " has disconnected </3").c_str());
                close(fd_int);
                // Kill the corresponding thread that is listening for user input
                pthread_cancel(threadLookup[fd_int].second);
                threadLookup.erase(fd_int);
                pthread_exit(NULL);
            } else {
                buf[numbytes] = '\0';
                data_rcv.append(buf, numbytes);
            }
            totalBytes += numbytes;
        } while (numbytes == MAXDATASIZE);

        // TODO: Only send if needed
        if (data_rcv[0] == INIT_P) {
            client_name = saveRSAPublicKey(data_rcv, totalBytes);
            sendRSAPublicKey(server_name.c_str(), fd_int);
        } else if (data_rcv[0] == MSG_P) {
            communication_info *info = (communication_info *) malloc(totalBytes);
            memcpy(info, &data_rcv[0], totalBytes);
            string decrypted = decryptMessageWithPrivateKey(info->msg, server_name);
            cout << client_name << ": " << decrypted << endl;
        } else {
            cout << "Unknown message format" << endl;
        }

        if(new_fd != NULL) {
            free(new_fd);       // Free down here so to make sure both threads have time to obtain data
            new_fd = NULL;
        }
    }
}

void *listenForUserInput(void *new_fd) {
    int fd_int = *((int *) new_fd);
    while (1) {
        string message;

        if (!(getline(cin, message))) {
            cin.clear(); // Reset stream
        }

        communication_info *info = (communication_info *)(malloc(sizeof(communication_info)));
        info->type = MSG_P;
        string encrypted = encryptMessageWithPublicKey(message, client_name);
        memcpy(info->msg, &encrypted.c_str()[0], encrypted.length());

        if (send(fd_int, info, sizeof(communication_info), 0) == -1) {
            perror("Issue sending");
            close(fd_int);
            // Kill the corresponding thread that is listening for client messages
            pthread_cancel(threadLookup[fd_int].first);
            threadLookup.erase(fd_int);
            pthread_exit(NULL);
        }
    }
}

void openTCPPort() {
    struct addrinfo hints, *servinfo, *p;
    struct sigaction sa;
    int yes=1;
    int rv;
    char addr_info[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, SERVER_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    if (p == NULL)  {
        diep("Couldn't bind to a port, try killing a process");
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, 1) == -1) {
        diep("Unable to listen");
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        diep("Failed on the sigaction");
    }

    cout << "server: waiting for connections..." << endl;

    while(1) {
        pthread_t clientListener, inputListener;
        socklen_t sin_size = sizeof their_addr;
        int *new_fd = (int *)malloc(sizeof(*new_fd));
        *new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

        if (*new_fd == -1) {
            diep("Couldn't accept the connection for some reason");
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), addr_info, sizeof addr_info);
        printf("server: got connection from %s\n", addr_info);

        int client_err = pthread_create(&clientListener, NULL, listenForMessagesFromClient, (void *)new_fd);
        if (client_err) {
            diep("Couldn't create thread to listen for messages from client");
        }
        int input_err = pthread_create(&inputListener, NULL, listenForUserInput, (void *)new_fd);
        if (input_err) {
            diep("Couldn't create thread to listen for user input");
        }
        // Push the threads to a map so they can be jointly killed
        threadLookup[*new_fd] = make_pair(clientListener, inputListener);
    }
}


int main(int argc, char** argv) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <username>" << endl;
        exit(1);
    }
    server_name = argv[1];
    if (server_name.length() > MAX_USERNAME_LENGTH) {
        cout << "Username can be a max of " << MAX_USERNAME_LENGTH << " characters long" << endl;
    }

    generateRSAKeys(server_name);

    // Integer encrypted = encryptMessageWithPublicKey("f", server_name);
    // cout << encrypted << endl;
    // cout << decryptMessageWithPrivateKey(encrypted, server_name) << endl;


    openTCPPort();
}
