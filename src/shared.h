#ifndef SHARED_H
#define SHARED_H 2

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

#define MAXDATASIZE 1024  // max number of bytes we can get at once

// Apparently this is octal
#define USERNAME_HEADER "\45\33\22\11"

void diep(const char *s);
void *get_in_addr(struct sockaddr *sa);

#endif
