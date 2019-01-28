#include "rsa.h"

using BN_ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
using RSA_ptr = std::unique_ptr<RSA, decltype(&::RSA_free)>;
using EVP_KEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
using BIO_FILE_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;

void generateRSAFiles(char * username) {
    RSA_ptr rsa_key(RSA_new(), ::RSA_free);
    BN_ptr bn(BN_new(), ::BN_free);
    BN_set_word(bn.get(), RSA_F4);

    char privKeyFileName[80], pubKeyFileName[80];
    strcpy(privKeyFileName, username);
    strcat(privKeyFileName, "private.pem");
    BIO_FILE_ptr privKey(BIO_new_file(privKeyFileName, "w"), ::BIO_free);

    strcpy(pubKeyFileName, username);
    strcat(pubKeyFileName, "public.pem");
    BIO_FILE_ptr pubKey(BIO_new_file(pubKeyFileName, "w"), ::BIO_free);

    EVP_KEY_ptr pkey(EVP_PKEY_new(), ::EVP_PKEY_free);
    EVP_PKEY_set1_RSA(pkey.get(), rsa_key.get());

    int rc = RSA_generate_key_ex(rsa_key.get(), 2048, bn.get(), NULL);

    PEM_write_bio_PrivateKey(privKey.get(), pkey.get(), NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_PUBKEY(pubKey.get(), pkey.get());
}
