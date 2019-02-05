#include "rsa.h"

using namespace boost;

AutoSeededRandomPool rnd;

ByteQueue loadKeyFileBytes(string keyPath) {
    ByteQueue bytes;
    try {
        FileSource file(keyPath.c_str(), true, new Base64Decoder);
        file.TransferTo(bytes);
    } catch (FileStore::OpenErr e) {
        cout << "\"" << keyPath << "\" not found, needs to be generated" << endl;
        exit(1);
    }
    bytes.MessageEnd();
    return bytes;
}

void generateRSAKeys(string username) {
   string pathPrefix = keyFolder + username;
   if ( filesystem::exists(pathPrefix + privKeyName) && filesystem::exists(pathPrefix + pubKeyName) ) {
      return;
   }
   filesystem::path dir(keyFolder.c_str());
   filesystem::create_directories(dir);

   // InvertibleRSAFunction is used directly only because the private key
   // won't actually be used to perform any cryptographic operation;
   // otherwise, an appropriate typedef'ed type from rsa.h would have been used.
   RSA::PrivateKey rsaPrivate;
   rsaPrivate.GenerateRandomWithKeySize(rnd, 2048);

   // With the current version of Crypto++, MessageEnd() needs to be called
   // explicitly because Base64Encoder doesn't flush its buffer on destruction.
   Base64Encoder privkeysink(new FileSink((pathPrefix + privKeyName).c_str()));
   rsaPrivate.DEREncode(privkeysink);
   privkeysink.MessageEnd();

   // Suppose we want to store the public key separately,
   // possibly because we will be sending the public key to a third party.
   RSA::PublicKey rsaPublic(rsaPrivate);

   Base64Encoder pubkeysink(new FileSink((pathPrefix + pubKeyName).c_str()));
   rsaPublic.DEREncode(pubkeysink);
   pubkeysink.MessageEnd();
}

// TODO: Don't load from file every encryption/decryption
Integer encryptMessageWithPublicKey(string message, string username) {
    ByteQueue bytes = loadKeyFileBytes((savedKeyFolder + username + pubKeyName).c_str());
    RSA::PublicKey pubKey;
    pubKey.Load(bytes);

    Integer message_int = Integer((const byte *)message.data(), message.size());
    Integer encrypted = pubKey.ApplyFunction(message_int);
    return encrypted;
}

string decryptMessageWithPrivateKey(Integer encrypted, string username) {
    ByteQueue bytes = loadKeyFileBytes((keyFolder + username + privKeyName).c_str());
    RSA::PrivateKey privKey;
    privKey.Load(bytes);

    string recovered;

    Integer r = privKey.CalculateInverse(rnd, encrypted);
    recovered.resize(r.MinEncodedSize());
    r.Encode((byte *)&recovered.data()[0], recovered.size());
    return recovered;
}

void sendRSAPublicKey(const char *username, int sock_fd) {
    std::ifstream t((keyFolder + username + pubKeyName).c_str());
    std::stringstream buffer;
    buffer << t.rdbuf();
    communication_info info = { INIT_P, {}, {}, {} };
    memcpy(info.username, username, strlen(username));
    memcpy(info.pubKey, &buffer.str()[0], buffer.str().length());
    if (send(sock_fd, &info, sizeof(communication_info), 0) == -1) {
      perror("Couldn't send the communication info");
    }
}

char *saveRSAPublicKey(string data_rcv, int totalBytes) {
    communication_info *info = (communication_info *) malloc(totalBytes);
    memcpy(info, &data_rcv[0], totalBytes);
    cout << "You are now connected to " << info->username << endl;

    filesystem::path dir(savedKeyFolder);
    filesystem::create_directories(dir);
    ofstream outfile((savedKeyFolder + info->username + pubKeyName).c_str());
    outfile << info->pubKey;

    return info->username;
}
