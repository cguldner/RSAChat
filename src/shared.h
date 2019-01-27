#ifndef SHARED_H
#define SHARED_H 2

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

#define MAXDATASIZE 100 // max number of bytes we can get at once


void diep(const char *s);
void *get_in_addr(struct sockaddr *sa);

#endif
