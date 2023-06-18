#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include "defines.h"

using namespace std;

unsigned char *decrypt_file(string fileName)
{
    int ret; // Used to return values
    string password = "password";
    string prvkey_file_name = "server_privK.pem";

    // Load my private key
    FILE *prvkey_file = fopen(prvkey_file_name.c_str(), "r");
    if (!prvkey_file)
    {
        cerr << "ERR: cannot open file '" << prvkey_file_name << "' (missing?)\n";
        exit(1);
    }
    EVP_PKEY *prvkey = PEM_read_PrivateKey(prvkey_file, NULL, NULL, (void *)password.c_str());
    fclose(prvkey_file);
    if (!prvkey)
    {
        cerr << "ERR: PEM_read_PrivateKey returned NULL\n";
        exit(1);
    }

    // Open the file to decrypt
    FILE *cphr_file_to_decrypt = fopen(fileName.c_str(), "rb");
    if (!cphr_file_to_decrypt)
    {
        cerr << "ERR: cannot open file '" << fileName << "' (file does not exist?)\n";
        exit(1);
    }

    // Get the file size
    ret = fseek(cphr_file_to_decrypt, 0, SEEK_END);
    if (ret != 0)
    {
        cerr << "ERR: cannot seek_end in  '" << fileName << "' (file corrupted?)\n";
        exit(1);
    }
    long int cphr_file_size = ftell(cphr_file_to_decrypt);
    ret = fseek(cphr_file_to_decrypt, 0, SEEK_SET);
    if (ret != 0)
    {
        cerr << "ERR: cannot seek_set in  '" << fileName << "' (file corrupted?)\n";
        exit(1);
    }

    // Vars
    const EVP_CIPHER *cipher_to_decrypt = EVP_aes_128_cbc();
    int encrypted_key_len_to_decrypt = EVP_PKEY_size(prvkey);
    int iv_len_to_decrypt = EVP_CIPHER_iv_length(cipher_to_decrypt);

    // Check for possible integer overflow in (encrypted_key_len + iv_len)
    if (encrypted_key_len_to_decrypt > INT_MAX - iv_len_to_decrypt)
    {
        cerr << "ERR: integer overflow (encrypted key too big?)\n";
        exit(1);
    }
    // Check for correct format of the encrypted file
    if (cphr_file_size < encrypted_key_len_to_decrypt + iv_len_to_decrypt)
    {
        cerr << "ERR: encrypted file with wrong format\n";
        exit(1);
    }

    // Allocate buffers for encrypted key, IV, ciphertext, and plaintext:
    unsigned char *encrypted_key_to_decrypt = (unsigned char *)malloc(encrypted_key_len_to_decrypt);
    unsigned char *iv_to_decrypt = (unsigned char *)malloc(iv_len_to_decrypt);
    int cphr_size_to_decrypt = cphr_file_size - encrypted_key_len_to_decrypt - iv_len_to_decrypt;
    unsigned char *cphr_buf_to_decrypt = (unsigned char *)malloc(cphr_size_to_decrypt);
    unsigned char *clear_buf = (unsigned char *)malloc(cphr_size_to_decrypt);
    if (!encrypted_key_to_decrypt || !iv_to_decrypt || !cphr_buf_to_decrypt || !clear_buf)
    {
        cerr << "ERR: malloc returned NULL (file too big?)\n";
        exit(1);
    }

    // Read the encrypted key, the IV, and the ciphertext from file:
    ret = fread(encrypted_key_to_decrypt, 1, encrypted_key_len_to_decrypt, cphr_file_to_decrypt);
    if (ret < encrypted_key_len_to_decrypt)
    {
        cerr << "ERR: while reading file '" << fileName << "'\n";
        exit(1);
    }
    ret = fread(iv_to_decrypt, 1, iv_len_to_decrypt, cphr_file_to_decrypt);
    if (ret < iv_len_to_decrypt)
    {
        cerr << "ERR: while reading file '" << fileName << "'\n";
        exit(1);
    }
    ret = fread(cphr_buf_to_decrypt, 1, cphr_size_to_decrypt, cphr_file_to_decrypt);
    if (ret < cphr_size_to_decrypt)
    {
        cerr << "ERR: while reading file '" << fileName << "'\n";
        exit(1);
    }
    fclose(cphr_file_to_decrypt);

    // Create the envelope context:
    EVP_CIPHER_CTX *ctx_to_decrypt = EVP_CIPHER_CTX_new();
    if (!ctx_to_decrypt)
    {
        cerr << "ERR: EVP_CIPHER_CTX_new returned NULL\n";
        exit(1);
    }

    // Decrypt the ciphertext:
    ret = EVP_OpenInit(ctx_to_decrypt, cipher_to_decrypt, encrypted_key_to_decrypt, encrypted_key_len_to_decrypt, iv_to_decrypt, prvkey);
    if (ret == 0)
    {
        cerr << "ERR: EVP_OpenInit returned " << ret << "\n";
        exit(1);
    }
    int nd = 0;    // bytes decrypted at each chunk
    int ndtot = 0; // total decrypted bytes
    ret = EVP_OpenUpdate(ctx_to_decrypt, clear_buf, &nd, cphr_buf_to_decrypt, cphr_size_to_decrypt);
    if (ret == 0)
    {
        cerr << "ERR: EVP_OpenUpdate returned " << ret << "\n";
        exit(1);
    }
    ndtot += nd;
    ret = EVP_OpenFinal(ctx_to_decrypt, clear_buf + ndtot, &nd);
    if (ret == 0)
    {
        cout << "ERR: EVP_OpenFinal returned " << ret << " (corrupted file?)\n";
        exit(1);
    }
    ndtot += nd;
    int clear_size = ndtot;

    string terminator = "";
    unsigned char *result = (unsigned char *)malloc(clear_size + 1);
    memcpy(result, clear_buf, clear_size);
    memcpy(result + clear_size, (unsigned char *)terminator.c_str(), 1);

    // Frees
    EVP_CIPHER_CTX_free(ctx_to_decrypt);
    EVP_PKEY_free(prvkey);
    free(encrypted_key_to_decrypt);
    free(iv_to_decrypt);
    free(clear_buf);

    return result;
}

void encrypt_file(string fileName, string mode, string text)
{
    int ret; // Used to return values
    string pubkey_file_name = "server_pubK.pem";
    unsigned char *text_to_encrypt;
    int text_to_encrypt_length;
    unsigned char *text_to_insert = (unsigned char *)text.c_str();
    int text_to_insert_length = strlen((const char *)text_to_insert);

    if (mode.compare("OVERWRITE") == 0)
    {
        text_to_encrypt_length = text_to_insert_length;
        text_to_encrypt = (unsigned char *)malloc(text_to_encrypt_length);
        memcpy(text_to_encrypt, text_to_insert, text_to_encrypt_length);
    }
    else if (mode.compare("APPEND") == 0)
    {
        unsigned char *text_decrypted = decrypt_file(fileName);
        int text_decrypted_length = strlen((const char *)text_decrypted);
        text_to_encrypt_length = text_decrypted_length + text_to_insert_length;
        text_to_encrypt = (unsigned char *)malloc(text_to_encrypt_length);
        memcpy(text_to_encrypt, text_decrypted, text_decrypted_length);
        memcpy(text_to_encrypt + text_decrypted_length, text_to_insert, text_to_insert_length);
    }

    FILE *pubkey_file = fopen(pubkey_file_name.c_str(), "r");
    if (!pubkey_file)
    {
        cerr << "ERR: cannot open file '" << pubkey_file_name << "' (missing?)\n";
        exit(1);
    }
    EVP_PKEY *pubkey = PEM_read_PUBKEY(pubkey_file, NULL, NULL, NULL);
    fclose(pubkey_file);
    if (!pubkey)
    {
        cerr << "ERR: PEM_read_PUBKEY returned NULL\n";
        exit(1);
    }

    // Vars
    const EVP_CIPHER *cipher = EVP_aes_128_cbc();
    int encrypted_key_len = EVP_PKEY_size(pubkey);
    int iv_len = EVP_CIPHER_iv_length(cipher);
    int block_size = EVP_CIPHER_block_size(cipher);

    // Create the envelope context
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        cerr << "ERR: EVP_CIPHER_CTX_new returned NULL\n";
        exit(1);
    }

    // Allocate buffers for encrypted key and IV:
    unsigned char *encrypted_key = (unsigned char *)malloc(encrypted_key_len);
    unsigned char *iv = (unsigned char *)malloc(iv_len);
    if (!encrypted_key || !iv)
    {
        cerr << "ERR: malloc returned NULL (encrypted key too big?)\n";
        exit(1);
    }

    // Check for possible integer overflow in (clear_size + block_size)
    if (text_to_encrypt_length > INT_MAX - block_size)
    {
        cerr << "ERR: integer overflow (file too big?)\n";
        exit(1);
    }

    // Allocate a buffer for the ciphertext:
    int enc_buffer_size = text_to_encrypt_length + block_size;
    unsigned char *cphr_buf = (unsigned char *)malloc(enc_buffer_size);
    if (!cphr_buf)
    {
        cerr << "ERR: malloc returned NULL (file too big?)\n";
        exit(1);
    }

    // Encrypt the plaintext:
    ret = EVP_SealInit(ctx, cipher, &encrypted_key, &encrypted_key_len, iv, &pubkey, 1);
    if (ret <= 0)
    {
        cerr << "ERR: EVP_SealInit returned " << ret << "\n";
        exit(1);
    }
    int nc = 0;    // Bytes encrypted at each chunk
    int nctot = 0; // Total encrypted bytes
    ret = EVP_SealUpdate(ctx, cphr_buf, &nc, text_to_encrypt, text_to_encrypt_length);
    if (ret == 0)
    {
        cerr << "ERR: EVP_SealUpdate returned " << ret << "\n";
        exit(1);
    }
    nctot += nc;
    ret = EVP_SealFinal(ctx, cphr_buf + nctot, &nc);
    if (ret == 0)
    {
        cerr << "ERR: EVP_SealFinal returned " << ret << "\n";
        exit(1);
    }
    nctot += nc;
    int cphr_size = nctot;

    // Write the encrypted key, the IV, and the ciphertext into a '.enc' file:
    FILE *cphr_file = fopen(fileName.c_str(), "wb");
    if (!cphr_file)
    {
        cerr << "ERR: cannot open file '" << fileName << "' (no permissions?)\n";
        exit(1);
    }
    ret = fwrite(encrypted_key, 1, encrypted_key_len, cphr_file);
    if (ret < encrypted_key_len)
    {
        cerr << "ERR: Couldn't write on file '" << fileName << "'\n";
        exit(1);
    }
    ret = fwrite(iv, 1, EVP_CIPHER_iv_length(cipher), cphr_file);
    if (ret < EVP_CIPHER_iv_length(cipher))
    {
        cerr << "ERR: Couldn't write on file '" << fileName << "'\n";
        exit(1);
    }
    ret = fwrite(cphr_buf, 1, cphr_size, cphr_file);
    if (ret < cphr_size)
    {
        cerr << "ERR: Couldn't write on file '" << fileName << "'\n";
        exit(1);
    }
    fclose(cphr_file);

    // Delete the plaintext from memory:
    memset(text_to_encrypt, 0, text_to_encrypt_length);

    // Frees
    EVP_CIPHER_CTX_free(ctx);
    free(text_to_encrypt);
    free(encrypted_key);
    free(iv);
    free(cphr_buf);
}

EVP_PKEY *generate_dh_key()
{
    EVP_PKEY *dh_params = nullptr;
    EVP_PKEY_CTX *dh_gen_ctx = nullptr;
    EVP_PKEY *dh_key = nullptr;

    int ret;

    try
    {
        // Allocate p and g
        dh_params = EVP_PKEY_new();
        if (!dh_params)
        {
            throw 0;
        }

        // Set default dh parameters for p & g
        DH *default_params = DH_get_2048_224();
        ret = EVP_PKEY_set1_DH(dh_params, default_params);

        // Delete p & g
        DH_free(default_params);

        if (ret != 1)
        {
            EVP_PKEY_free(dh_params);
            throw 1;
        }

        // a or b
        dh_gen_ctx = EVP_PKEY_CTX_new(dh_params, nullptr);
        if (!dh_gen_ctx)
        {
            EVP_PKEY_free(dh_params);
            EVP_PKEY_CTX_free(dh_gen_ctx);
            throw 2;
        }

        ret = EVP_PKEY_keygen_init(dh_gen_ctx);
        if (ret != 1)
        {
            EVP_PKEY_free(dh_params);
            EVP_PKEY_CTX_free(dh_gen_ctx);
            throw 3;
        }

        ret = EVP_PKEY_keygen(dh_gen_ctx, &dh_key);
        if (ret != 1)
        {
            EVP_PKEY_free(dh_params);
            EVP_PKEY_CTX_free(dh_gen_ctx);
            throw 4;
        }
    }
    catch (int error_code)
    {
        switch (error_code)
        {
        case 0:
        {
            cerr << "ERR: Couldn't generate new dh params!" << endl;
            break;
        }
        case 1:
        {
            cerr << "ERR: Couldn't load default params!" << endl;
            break;
        }
        case 2:
        {
            cerr << "ERR: Couldn't load define dh context!" << endl;
            break;
        }
        case 3:
        {
            cerr << "ERR: Couldn't dh keygen init!" << endl;
            break;
        }
        case 4:
        {
            cerr << "ERR: Couldn't dh keygen!" << endl;
            break;
        }
        }
        return nullptr;
    }

    EVP_PKEY_CTX_free(dh_gen_ctx);
    EVP_PKEY_free(dh_params);

    return dh_key;
}

// Derive shared symm key
unsigned char *derive_shared_secret(EVP_PKEY *this_key, EVP_PKEY *other_key)
{

    int ret; // Used to return values

    // Create a new context for deriving DH key
    EVP_PKEY_CTX *key_ctx = EVP_PKEY_CTX_new(this_key, nullptr);
    if (!key_ctx)
    {
        cerr << "ERR: Couldn't load define dh context of the current host!" << endl;
        return nullptr;
    }

    unsigned char *shared_secret = nullptr;
    size_t secret_length = 0;

    // Derive the shared secret between the two hosts
    try
    {
        ret = EVP_PKEY_derive_init(key_ctx);
        if (ret != 1)
        {
            throw 0;
        }
        ret = EVP_PKEY_derive_set_peer(key_ctx, other_key);
        if (ret != 1)
        {
            throw 0;
        }
        ret = EVP_PKEY_derive(key_ctx, nullptr, &secret_length);
        if (ret != 1)
        {
            throw 0;
        }
        shared_secret = (unsigned char *)malloc(secret_length);
        if (!shared_secret)
        {
            throw 1;
        }
    }
    catch (int e)
    {
        if (e == 1)
        {
            cerr << "ERR: Couldn't allocate shared secret!" << endl;
        }
        else
        {
            cerr << "ERR: Couldn't malloc!" << endl;
        }
        EVP_PKEY_CTX_free(key_ctx);
        return nullptr;
    }

    ret = EVP_PKEY_derive(key_ctx, shared_secret, &secret_length);
    EVP_PKEY_CTX_free(key_ctx);
    if (ret != 1)
    {
        memset(shared_secret, 0, secret_length);
        free(shared_secret);
        return nullptr;
    }
    return shared_secret;
}

// Serialize key EVP_PKEY
void *serialize_evp_pkey(EVP_PKEY *key, uint32_t &key_len)
{
    int ret;
    long ret_long;
    BIO *bio = nullptr;
    void *key_buffer = nullptr;

    try
    {
        // Allocate an instance of the BIO structure for serialization
        bio = BIO_new(BIO_s_mem());
        if (!bio)
        {
            throw 0;
        }

        // Serialize a key into PEM format and write it in the BIO
        ret = PEM_write_bio_PUBKEY(bio, key);
        if (ret != 1)
        {
            BIO_free(bio);
            throw 1;
        }

        // Set of the pointer key_buffer to the buffer of the memory bio and return its size
        ret_long = BIO_get_mem_data(bio, &key_buffer);
        if (ret_long <= 0)
        {
            BIO_free(bio);
            throw 2;
        }
        key_len = (uint32_t)ret_long;

        // Allocate memory for the serialized key
        key_buffer = malloc(key_len);
        if (!key_buffer)
        {
            BIO_free(bio);
            throw 3;
        }

        // Read data from bio and extract serialized key
        ret = BIO_read(bio, key_buffer, key_len);
        if (ret < 1)
        {
            BIO_free(bio);
            free(key_buffer);
            throw 4;
        }
    }
    catch (int error_code)
    {
        switch (error_code)
        {
        case 0:
        {
            cerr << "ERR: Couldn't BIO_new!" << endl;
            break;
        }
        case 1:
        {
            cerr << "ERR: Couldn't PEM_write_bio_PUBKEY with error: " << ret << endl;
            break;
        }
        case 2:
        {
            cerr << "ERR: Couldn't BIO_get_mem_data with error: " << ret_long << endl;
            break;
        }
        case 3:
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            break;
        }
        case 4:
        {
            cerr << "ERR: Couldn't BIO_read with error: " << ret << endl;
            break;
        }
        }
        return nullptr;
    }

    // Free
    BIO_free(bio);

    return key_buffer;
}

// Deserialize key EVP_PKEY
EVP_PKEY *deserialize_evp_pkey(const void *_key_buffer, const uint32_t _key_length)
{
    int ret;
    BIO *bio;
    EVP_PKEY *key;

    try
    {
        // Allocate an instance of the BIO structure for serialization
        bio = BIO_new(BIO_s_mem());
        if (!bio)
        {
            throw 0;
        }

        // Write serialized the key from the buffer in bio
        ret = BIO_write(bio, _key_buffer, _key_length);
        if (ret <= 0)
        {
            BIO_free(bio);
            throw 1;
        }

        // Reads a key written in PEM format from the bio and deserialize it
        key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        if (!key)
        {
            BIO_free(bio);
            throw 2;
        }
    }
    catch (int error_code)
    {
        switch (error_code)
        {
        case 0:
        {
            cerr << "ERR: Couldn't BIO_new!" << endl;
            break;
        }
        case 1:
        {
            cerr << "ERR: Couldn't BIO_write with error: " << ret << endl;
            break;
        }
        case 2:
        {
            cerr << "ERR: Couldn't PEM_read_bio_PUBKEY!" << endl;
            break;
        }
        }
        return nullptr;
    }

    // Free
    BIO_free(bio);

    return key;
}

// Sign a message using private key prvkey
unsigned char *sign_message(EVP_PKEY *prvkey, const unsigned char *msg, const size_t msg_len, unsigned int &signature_len)
{
    int ret;
    EVP_MD_CTX *ctx = nullptr;
    unsigned char *signature = nullptr;

    if (!prvkey)
    {
        return nullptr;
    }

    try
    {
        ctx = EVP_MD_CTX_new();
        if (!ctx)
        {
            throw 1;
        }

        ret = EVP_SignInit(ctx, EVP_sha256());
        if (ret != 1)
        {
            EVP_MD_CTX_free(ctx);
            throw 2;
        }

        ret = EVP_SignUpdate(ctx, msg, msg_len);
        if (ret != 1)
        {
            EVP_MD_CTX_free(ctx);
            throw 3;
        }

        signature_len = EVP_PKEY_size(prvkey);
        signature = (unsigned char *)malloc(signature_len);
        if (!signature)
        {
            EVP_MD_CTX_free(ctx);
            throw 4;
        }

        ret = EVP_SignFinal(ctx, signature, &signature_len, prvkey);
        if (ret != 1)
        {
            EVP_MD_CTX_free(ctx);
            free(signature);
            throw 5;
        }
    }
    catch (int error_code)
    {
        switch (error_code)
        {
        case 1:
        {
            cerr << "ERR: Couldn't create new context for signature!" << endl;
            break;
        }
        case 2:
        {
            cerr << "ERR: Couldn't sign init!" << endl;
            break;
        }
        case 3:
        {
            cerr << "ERR: Couldn't sign update!" << endl;
            break;
        }
        case 4:
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            break;
        }
        case 5:
        {
            cerr << "ERR: Couldn't sign final!" << endl;
            break;
        }
        }
        return nullptr;
    }

    // Frees
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(prvkey);

    return signature;
}

// Verify signature with pubkey
int verify_signature(EVP_PKEY *pubkey, const unsigned char *signature, const size_t signature_len, const unsigned char *cleartext, const size_t cleartext_len)
{
    EVP_MD_CTX *ctx = nullptr;

    int ret;

    if (!pubkey)
    {
        return -1;
    }

    // verify signature
    try
    {
        ctx = EVP_MD_CTX_new();
        if (!ctx)
        {
            throw 1;
        }

        ret = EVP_VerifyInit(ctx, EVP_sha256());
        if (ret != 1)
        {
            EVP_MD_CTX_free(ctx);
            throw 2;
        }

        ret = EVP_VerifyUpdate(ctx, cleartext, cleartext_len);
        if (ret != 1)
        {
            EVP_MD_CTX_free(ctx);
            throw 3;
        }

        ret = EVP_VerifyFinal(ctx, signature, signature_len, pubkey);

        if (ret != 1)
        {
            EVP_MD_CTX_free(ctx);
            throw 4;
        }
    }
    catch (int error_code)
    {
        switch (error_code)
        {
        case 1:
        {
            cerr << "ERR:  Couldn't create new context for signature!" << endl;
            break;
        }
        case 2:
        {
            cerr << "ERR: Couldn't verify init for signature!" << endl;
            break;
        }
        case 3:
        {
            cerr << "ERR: Couldn't verify update for signature!" << endl;
            break;
        }
        case 4:
        {
            cerr << "ERR: Couldn't verify final for signature!" << endl;
            break;
        }
        }
        return -1;
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pubkey);

    return 0;
}

// Generate SHA-256, used to hash the shared secrets
int generate_SHA256(unsigned char *msg, size_t msg_len, unsigned char *&digest, uint32_t &digestlen, uint32_t max_msg_size)
{
    int ret;
    EVP_MD_CTX *ctx;

    if (msg_len == 0 || msg_len > max_msg_size)
    {
        cerr << "ERR: Message length is not allowed!" << endl;
        return -1;
    }

    try
    {
        digest = (unsigned char *)malloc(EVP_MD_size(EVP_sha256()));
        if (!digest)
        {
            throw 1;
        }

        ctx = EVP_MD_CTX_new();
        if (!ctx)
        {
            free(digest);
            throw 2;
        }

        ret = EVP_DigestInit(ctx, EVP_sha256());
        if (ret != 1)
        {
            free(digest);
            EVP_MD_CTX_free(ctx);
            throw 3;
        }

        ret = EVP_DigestUpdate(ctx, (unsigned char *)msg, msg_len);
        if (ret != 1)
        {
            free(digest);
            EVP_MD_CTX_free(ctx);
            throw 4;
        }

        ret = EVP_DigestFinal(ctx, digest, &digestlen);
        if (ret != 1)
        {
            free(digest);
            EVP_MD_CTX_free(ctx);
            throw 5;
        }
    }
    catch (int error_code)
    {
        switch (error_code)
        {
        case 1:
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            break;
        }
        case 2:
        {
            cerr << "ERR: Couldn't create context definition!" << endl;
            break;
        }
        case 3:
        {
            cerr << "ERR: Couldn't initialize digest creation!" << endl;
            break;
        }
        case 4:
        {
            cerr << "ERR: Couldn't update digest!" << endl;
            break;
        }
        case 5:
        {
            cerr << "ERR: Couldn't finalize digest!" << endl;
            break;
        }
        }
        return -1;
    }

    EVP_MD_CTX_free(ctx);
    return 0;
}

// Verify if 2 digest SHA-256 are the same
bool verify_SHA256(unsigned char *digest, unsigned char *received_digest)
{

    if (CRYPTO_memcmp(digest, received_digest, EVP_MD_size(EVP_sha256())) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

// Generate SHA-256 HMAC with a 256 bit key
int generate_SHA256_HMAC(unsigned char *msg, size_t msg_len, unsigned char *&digest, uint32_t &digestlen, unsigned char *key, uint32_t max_msg_size)
{
    int ret;
    HMAC_CTX *ctx;

    if (msg_len == 0 || msg_len > max_msg_size)
    {
        cerr << "ERR: Message length is not allowed!" << endl;
        return -1;
    }

    try
    {
        digest = (unsigned char *)malloc(EVP_MD_size(EVP_sha256()));
        if (!digest)
        {
            throw 1;
        }

        memset(digest, 0, EVP_MD_size(EVP_sha256()));

        ctx = HMAC_CTX_new();
        if (!ctx)
        {
            free(digest);
            throw 2;
        }

        ret = HMAC_Init_ex(ctx, key, EVP_MD_size(EVP_sha256()), EVP_sha256(), NULL);
        if (ret != 1)
        {
            free(digest);
            HMAC_CTX_free(ctx);
            throw 3;
        }

        ret = HMAC_Update(ctx, (unsigned char *)msg, msg_len);
        if (ret != 1)
        {
            free(digest);
            HMAC_CTX_free(ctx);
            throw 4;
        }

        ret = HMAC_Final(ctx, digest, &digestlen);
        if (ret != 1)
        {
            free(digest);
            HMAC_CTX_free(ctx);
            throw 5;
        }
    }
    catch (int error_code)
    {
        switch (error_code)
        {
        case 1:
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            break;
        }
        case 2:
        {
            cerr << "ERR: Couldn't create context definition!" << endl;
            break;
        }
        case 3:
        {
            cerr << "ERR: Couldn't initialize digest creation!" << endl;
            break;
        }
        case 4:
        {
            cerr << "ERR: Couldn't update digest!" << endl;
            break;
        }
        case 5:
        {
            cerr << "ERR: Couldn't finalize digest!" << endl;
            break;
        }
        }
        return -1;
    }

    HMAC_CTX_free(ctx);
    return 0;
}

// Hash symmetric key and take a portion of the total
int hash_symmetric_key(unsigned char *&symm_key, unsigned char *symm_key_no_hashed)
{
    unsigned char *hash;
    uint32_t len;
    int ret;
    int key_size_aes = EVP_CIPHER_key_length(EVP_aes_128_cbc());

    ret = generate_SHA256(symm_key_no_hashed, key_size_aes, hash, len, key_size_aes);
    if (ret != 0)
    {
        cerr << "ERR: Couldn't hash symmetric key!" << endl;
        return ret;
    }

    symm_key = (unsigned char *)malloc(key_size_aes); // AES-128
    if (!symm_key)
    {
        cerr << "ERR: Couldn't malloc!" << endl;
        return -1;
    }

    // Take a portion of the mac for 128 bits key (AES)
    memcpy(symm_key, hash, key_size_aes);

    // Free
    free(hash);

    return 0;
}

// Hash hmac key and take a portion of HMAC_KEY_SIZE
int hash_hmac_key(unsigned char *&hmac_key, unsigned char *hmac_key_no_hashed)
{
    unsigned char *hash;
    uint32_t len;
    int ret;

    ret = generate_SHA256(hmac_key_no_hashed, HMAC_KEY_SIZE, hash, len, HMAC_KEY_SIZE);
    if (ret != 0)
    {
        cerr << "ERR: Couldn't hash symmetric key!" << endl;
        return ret;
    }

    hmac_key = (unsigned char *)malloc(HMAC_KEY_SIZE);
    if (!hmac_key)
    {
        cerr << "ERR: Couldn't malloc!" << endl;
        return -1;
    }

    // Take only HMAC_KEY_SIZE bits out of 256
    memcpy(hmac_key, hash, HMAC_KEY_SIZE);

    // Free
    free(hash);

    return 0;
}

unsigned char *generate_iv()
{
    unsigned char *iv = nullptr;
    int iv_len = EVP_CIPHER_iv_length(EVP_aes_128_cbc());
    iv = (unsigned char *)malloc(iv_len);
    int ret = RAND_bytes(iv, iv_len);
    if (ret != 1 || !iv)
    {
        // Must free if we have an error!
        free(iv);
        iv = nullptr;
        throw 0;
    }
    return iv;
}

int cbc_encrypt(unsigned char *msg, int msg_len, unsigned char *&ciphertext, int &cipherlen, unsigned char *symmetric_key, unsigned char *iv)
{
    int outlen;
    int block_size = EVP_CIPHER_block_size(EVP_aes_128_cbc());
    int ret;

    EVP_CIPHER_CTX *ctx;

    if (msg_len == 0 || msg_len > MAX_PKT_SIZE)
    {
        cerr << "ERR: Message length is not allowed!" << endl;
        return -1;
    }

    try
    {
        // Buffer for the ciphertext + padding (always addded as #PCKS7)
        ciphertext = (unsigned char *)malloc(msg_len + block_size);
        if (!ciphertext)
        {
            throw 1;
        }

        memset(ciphertext, 0, msg_len + block_size);

        // Context definition
        ctx = EVP_CIPHER_CTX_new();
        if (!ctx)
        {
            free(ciphertext);
            throw 2;
        }

        // Init encryption
        ret = EVP_EncryptInit(ctx, EVP_aes_128_cbc(), symmetric_key, iv);
        if (ret != 1)
        {
            free(ciphertext);
            EVP_CIPHER_CTX_free(ctx);
            throw 3;
        }

        outlen = 0;
        cipherlen = 0;

        // Encrypt update on the message
        ret = EVP_EncryptUpdate(ctx, ciphertext, &outlen, (unsigned char *)msg, msg_len);

        if (ret != 1)
        {
            free(ciphertext);
            EVP_CIPHER_CTX_free(ctx);
            free(iv);
            throw 4;
        }
        cipherlen += outlen;

        ret = EVP_EncryptFinal(ctx, ciphertext + outlen, &outlen);
        if (ret != 1)
        {
            free(ciphertext);
            EVP_CIPHER_CTX_free(ctx);
            free(iv);
            throw 5;
        }

        // Check on the cipherlen overflow
        if (cipherlen > numeric_limits<int>::max() - outlen)
        {
            free(ciphertext);
            EVP_CIPHER_CTX_free(ctx);
            free(iv);
            throw 6;
        }
        cipherlen += outlen;
    }
    catch (int error_code)
    {
        switch (error_code)
        {
        case 1:
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            break;
        }
        case 2:
        {
            cerr << "ERR: Couldn't create a context definition!" << endl;
            break;
        }
        case 3:
        {
            cerr << "ERR: Couldn't initialize encryption!" << endl;
            break;
        }
        case 4:
        {
            cerr << "ERR: Couldn't encrypt update!" << endl;
            break;
        }
        case 5:
        {
            cerr << "ERR: Couldn't encrypt final!" << endl;
            break;
        }
        case 6:
        {
            cerr << "ERR: Overflow error on cipherlen!" << endl;
            break;
        }
        }
        return -1;
    }
    return 0;
}

int cbc_decrypt(unsigned char *ciphertext, int cipherlen, unsigned char *&plaintext, uint32_t &plainlen, unsigned char *symmetric_key, unsigned char *iv)
{
    int outlen;
    int ret;

    EVP_CIPHER_CTX *ctx;

    if (cipherlen == 0 || cipherlen > MAX_PKT_SIZE)
    {
        cerr << "ERR: Message length is not allowed!" << endl;
        return -1;
    }

    // Error if iv is not set
    if (!iv)
    {
        cerr << "ERR: Missing iv for decryption!" << endl;
        return -1;
    }

    try
    {
        // Plaintext
        plaintext = (unsigned char *)malloc(cipherlen);
        if (!plaintext)
        {
            throw 1;
        }

        memset(plaintext, 0, cipherlen);

        // Context definition
        ctx = EVP_CIPHER_CTX_new();
        if (!ctx)
        {
            free(plaintext);
            throw 2;
        }

        // Init encryption
        ret = EVP_DecryptInit(ctx, EVP_aes_128_cbc(), symmetric_key, iv);
        if (ret != 1)
        {
            free(plaintext);
            EVP_CIPHER_CTX_free(ctx);
            throw 3;
        }

        outlen = 0;
        plainlen = 0;

        ret = EVP_DecryptUpdate(ctx, plaintext + outlen, &outlen, (unsigned char *)ciphertext + outlen, cipherlen);
        if (ret != 1)
        {
            free(plaintext);
            EVP_CIPHER_CTX_free(ctx);
            throw 4;
        }
        plainlen += outlen;

        ret = EVP_DecryptFinal(ctx, plaintext + outlen, &outlen);
        if (ret != 1)
        {
            free(plaintext);
            EVP_CIPHER_CTX_free(ctx);
            throw 5;
        }

        // Check on the cipherlen overflow
        if (plainlen > numeric_limits<uint32_t>::max() - outlen)
        {
            free(plaintext);
            EVP_CIPHER_CTX_free(ctx);
            throw 6;
        }
        plainlen += outlen;
    }
    catch (int error_code)
    {
        switch (error_code)
        {
        case 1:
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            break;
        }
        case 2:
        {
            cerr << "ERR: Couldn't create a context definition!" << endl;
            break;
        }
        case 3:
        {
            cerr << "ERR: Couldn't initialize decryption!" << endl;
            break;
        }
        case 4:
        {
            cerr << "ERR: Couldn't decrypt update!" << endl;
            break;
        }
        case 5:
        {
            cerr << "ERR: Couldn't decrypt final!" << endl;
            break;
        }
        case 6:
        {
            cerr << "ERR: Overflow error on plainlen!" << endl;
            break;
        }
        }
        return -1;
    }
    return 0;
}