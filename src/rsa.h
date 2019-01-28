#ifndef RSA_H
#define RSA_H 1

#include <memory>
using std::unique_ptr;

using namespace std;

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/x509.h>

#include <string.h>

void generateRSAFiles(char *username);

#endif
