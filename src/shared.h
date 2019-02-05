#ifndef SHARED_H
#define SHARED_H 2

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>

#include <crypto++/rsa.h>

#define MAXDATASIZE 1024  // max number of bytes we can get at once
#define MAX_USERNAME_LENGTH 30
#define PUBLIC_KEY_LENGTH 398

// Apparently this is octal
#define INIT_P 21
#define MSG_P 15


typedef struct communication_info {
    char type;
    char username[MAX_USERNAME_LENGTH];
    char pubKey[PUBLIC_KEY_LENGTH];
    char msg[1000];
} communication_info;

typedef struct comm_message {
    char type;
    char msg[1023];
} comm_message;

void diep(const char *s);
void *get_in_addr(struct sockaddr *sa);

#endif
