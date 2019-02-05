#ifndef RSA_H
#define RSA_H 1

#include <crypto++/cryptlib.h>
#include <crypto++/osrng.h>
#include <crypto++/base64.h>
#include <crypto++/randpool.h>
#include <crypto++/files.h>
#include <crypto++/rsa.h>
#include <crypto++/integer.h>

using namespace std;
using namespace CryptoPP;

#include <boost/filesystem.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>

#include "shared.h"

const string keyFolder = "mykeys/";
const string savedKeyFolder = "saved_keys/";
const string pubKeyName = "public.key";
const string privKeyName = "private.key";

ByteQueue loadKeyFileBytes(string keyPath);
void generateRSAKeys(string username);
Integer encryptMessageWithPublicKey(string message, string username);
string decryptMessageWithPrivateKey(Integer encrypted, string username);
void sendRSAPublicKey(const char *username, int sock_fd);
char *saveRSAPublicKey(string data_rcv, int totalBytes);

#endif
