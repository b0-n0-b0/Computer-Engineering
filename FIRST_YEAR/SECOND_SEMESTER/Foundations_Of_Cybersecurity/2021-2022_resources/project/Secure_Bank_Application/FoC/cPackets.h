#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>
#include <fstream>
#include <vector>
#include <string>
#include <limits>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <iostream>
#include <string.h>
#include "defines.h"
#include "cryptoFunctions.cpp"

using namespace std;

struct wave_pkt
{
    uint16_t code = WAVE;
    uint16_t username_len;
    string username;
    uint32_t symmetric_key_param_len;
    uint32_t hmac_key_param_len;
    EVP_PKEY* symmetric_key_param = nullptr;
    EVP_PKEY* hmac_key_param = nullptr;

    //Serialize the message to send it through the network
    void* serialize_message(int &len)
    {
        uint8_t* serialized_pkt = nullptr;
        int pointer_counter = 0;

        void* key_buffer_symmetric = nullptr;
        void* key_buffer_hmac = nullptr;   

        // Serializes key to send it throught the network using BIO structure
        key_buffer_symmetric = serialize_evp_pkey(symmetric_key_param, symmetric_key_param_len);
        if (key_buffer_symmetric == nullptr)
        {
            return nullptr;
        }

        // Serializes key to send it throught the network using BIO structure
        key_buffer_hmac = serialize_evp_pkey(hmac_key_param, hmac_key_param_len);
        if (key_buffer_hmac == nullptr)
        {
            return nullptr;
        }

        uint16_t certified_code = htons(code);
        username_len = username.length();
        uint16_t certified_username_len = htons(username_len);

        // Total length of the serialized packet
        len = sizeof(certified_code) + sizeof(certified_username_len) + username_len + sizeof(symmetric_key_param_len) + sizeof(hmac_key_param_len) + symmetric_key_param_len + hmac_key_param_len;
        serialized_pkt = (uint8_t *)malloc(len);
        if (!serialized_pkt)
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            return nullptr;
        }

        // Copy of the code
        memcpy(serialized_pkt, &certified_code, sizeof(certified_code));
        pointer_counter += sizeof(code);

        // Copy username_len
        memcpy(serialized_pkt + pointer_counter, &certified_username_len, sizeof(certified_username_len));
        pointer_counter += sizeof(username_len);

        // Copy of the username
        uint8_t* username_certified = (uint8_t *)username.c_str();
        memcpy(serialized_pkt + pointer_counter, username_certified, username_len);
        pointer_counter += username_len;

        // Copy of symmetric_key_param_len
        uint32_t certified_symmetric_len = htonl(symmetric_key_param_len);
        memcpy(serialized_pkt + pointer_counter, &certified_symmetric_len, sizeof(certified_symmetric_len));
        pointer_counter += sizeof(certified_symmetric_len);

        // Copy of hmac_key_param_len
        uint32_t certified_hmac_len = htonl(hmac_key_param_len);
        memcpy(serialized_pkt + pointer_counter, &certified_hmac_len, sizeof(certified_hmac_len));
        pointer_counter += sizeof(certified_hmac_len);

        // Copy of the symmetric_key_param buffer
        memcpy(serialized_pkt + pointer_counter, key_buffer_symmetric, symmetric_key_param_len);
        pointer_counter += symmetric_key_param_len;

        // Copy of the hmac_key_param buffer
        memcpy(serialized_pkt + pointer_counter, key_buffer_hmac, hmac_key_param_len);

        free(key_buffer_symmetric);
        free(key_buffer_hmac);

        return serialized_pkt;
    }

    //Deserializes a message received from the network
    bool deserialize_message(uint8_t* serialized_pkt)
    {
        uint64_t pointer_counter = 0;

        // Copy of the code
        memcpy(&code, serialized_pkt, sizeof(code));
        code = ntohs(code);

        if (pointer_counter > numeric_limits<uint64_t>::max() - sizeof(code))
        {
            return false;
        }
        pointer_counter += sizeof(code);

        //Checks code of the packet
        if (code != WAVE)
        {
            cerr << "ERR: invalid packet code!" << endl;
            return false;
        }

        // Copy username_len
        memcpy(&username_len, serialized_pkt + pointer_counter, sizeof(username_len));
        username_len = ntohs(username_len);
        if (pointer_counter > numeric_limits<uint64_t>::max() - sizeof(username_len))
        {
            return false;
        }
        pointer_counter += sizeof(username_len);

        // Copy username
        username.assign((char *)serialized_pkt + pointer_counter, username_len);
        if (pointer_counter > numeric_limits<uint64_t>::max() - username_len)
        {
            return false;
        }
        pointer_counter += username_len;

        // Copy of symmetric_key_param_len
        memcpy(&symmetric_key_param_len, serialized_pkt + pointer_counter, sizeof(symmetric_key_param_len));
        symmetric_key_param_len = ntohl(symmetric_key_param_len);
        if (pointer_counter > numeric_limits<uint64_t>::max() - sizeof(symmetric_key_param_len))
        {
            return false;
        }
        pointer_counter += sizeof(symmetric_key_param_len);

        // Copy of hmac_key_param_len
        memcpy(&hmac_key_param_len, serialized_pkt + pointer_counter, sizeof(hmac_key_param_len));
        hmac_key_param_len = ntohl(hmac_key_param_len);
        if (pointer_counter > numeric_limits<uint64_t>::max() - sizeof(hmac_key_param_len))
        {
            return false;
        }
        pointer_counter += sizeof(hmac_key_param_len);

        // Copy of the symmetric parameter
        symmetric_key_param = deserialize_evp_pkey(serialized_pkt + pointer_counter, symmetric_key_param_len);
        if (pointer_counter > numeric_limits<uint64_t>::max() - symmetric_key_param_len)
        {
            return false;
        }
        pointer_counter += symmetric_key_param_len;
        // Copy of the hmac parameter
        hmac_key_param = deserialize_evp_pkey(serialized_pkt + pointer_counter, hmac_key_param_len);

        if (hmac_key_param == nullptr || symmetric_key_param == nullptr)
        {
            cerr << "ERR: Couldn't deserialize correctly a key! " << endl;
            return false;
        }
        return true;
    }
};

struct login_authentication_pkt
{
    // Clear
    uint32_t cert_len = 0;
    uint32_t symmetric_key_param_server_clear_len = 0;
    uint32_t hmac_key_param_server_clear_len = 0;
    uint32_t encrypted_signing_len = 0;
    uint8_t* iv_cbc = nullptr;
    EVP_PKEY* symmetric_key_param_server_clear = nullptr;
    EVP_PKEY* hmac_key_param_server_clear = nullptr;

    //Encrypted sign
    uint8_t* encrypted_signing = nullptr;

    // Encrypted signed part to be serialized
    uint32_t symmetric_key_param_len_server = 0;
    uint32_t hmac_key_param_len_server = 0;
    uint32_t symmetric_key_param_len_client = 0;
    uint32_t hmac_key_param_len_client = 0;

    EVP_PKEY* symmetric_key_param_server = nullptr;
    EVP_PKEY* hmac_key_param_server = nullptr;
    EVP_PKEY* symmetric_key_param_client = nullptr;
    EVP_PKEY* hmac_key_param_client = nullptr;

    void* serialize_part_to_encrypt(int &len)
    {
        int pointer_counter = 0;
        uint8_t* serialized_pte;

        // Evp serializations to pass data through the network
        void* key_buffer_symmetric_server = serialize_evp_pkey(symmetric_key_param_server, symmetric_key_param_len_server);
        void* key_buffer_hmac_server = serialize_evp_pkey(hmac_key_param_server, hmac_key_param_len_server);
        void* key_buffer_symmetric_client = serialize_evp_pkey(symmetric_key_param_client, symmetric_key_param_len_client);
        void* key_buffer_hmac_client = serialize_evp_pkey(hmac_key_param_client, hmac_key_param_len_client);

        // Total length
        len = sizeof(symmetric_key_param_len_server) + sizeof(hmac_key_param_len_server) + sizeof(symmetric_key_param_len_client) +
              sizeof(hmac_key_param_len_client) + symmetric_key_param_len_server + hmac_key_param_len_server + symmetric_key_param_len_client +
              hmac_key_param_len_client;

        serialized_pte = (uint8_t *)malloc(len);
        if (!serialized_pte)
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            return nullptr;
        }

        // Get lengths of 4 keys
        uint32_t certified_symmetric_key_param_len_server = htonl(symmetric_key_param_len_server);
        uint32_t certified_hmac_key_param_len_server = htonl(hmac_key_param_len_server);
        uint32_t certified_symmetric_key_param_len_client = htonl(symmetric_key_param_len_client);
        uint32_t certified_hmac_key_param_len_client = htonl(hmac_key_param_len_client);

        // Start copying
        memcpy(serialized_pte + pointer_counter, &certified_symmetric_key_param_len_server, sizeof(certified_symmetric_key_param_len_server));
        pointer_counter += sizeof(certified_symmetric_key_param_len_server);
        memcpy(serialized_pte + pointer_counter, &certified_hmac_key_param_len_server, sizeof(certified_hmac_key_param_len_server));
        pointer_counter += sizeof(certified_hmac_key_param_len_server);
        memcpy(serialized_pte + pointer_counter, &certified_symmetric_key_param_len_client, sizeof(certified_symmetric_key_param_len_client));
        pointer_counter += sizeof(certified_symmetric_key_param_len_client);
        memcpy(serialized_pte + pointer_counter, &certified_hmac_key_param_len_client, sizeof(certified_hmac_key_param_len_client));
        pointer_counter += sizeof(certified_hmac_key_param_len_client);
        memcpy(serialized_pte + pointer_counter, key_buffer_symmetric_server, symmetric_key_param_len_server);
        pointer_counter += symmetric_key_param_len_server;
        memcpy(serialized_pte + pointer_counter, key_buffer_hmac_server, hmac_key_param_len_server);
        pointer_counter += hmac_key_param_len_server;
        memcpy(serialized_pte + pointer_counter, key_buffer_symmetric_client, symmetric_key_param_len_client);
        pointer_counter += symmetric_key_param_len_client;
        memcpy(serialized_pte + pointer_counter, key_buffer_hmac_client, hmac_key_param_len_client);
        pointer_counter += hmac_key_param_len_client;

        // Frees
        free(key_buffer_symmetric_server);
        free(key_buffer_hmac_server);
        free(key_buffer_symmetric_client);
        free(key_buffer_hmac_client);

        return serialized_pte;
    }

    void* serialize_message(int &len)
    {
        int pointer_counter = 0;
        uint8_t* serialized_pkt;
        void* key_buffer_symmetric_server_clear = nullptr;
        void* key_buffer_hmac_server_clear = nullptr;

        if (encrypted_signing == nullptr || encrypted_signing_len == 0 || iv_cbc == nullptr)
        {
            cerr << "ERR: Missing field!" << endl;
            return nullptr;
        }

        // Symm_key
        key_buffer_symmetric_server_clear = serialize_evp_pkey(symmetric_key_param_server_clear, symmetric_key_param_server_clear_len);
        uint32_t certified_symmetric_key_server_clear_len = htonl(symmetric_key_param_server_clear_len);

        // Hmac_key
        key_buffer_hmac_server_clear = serialize_evp_pkey(hmac_key_param_server_clear, hmac_key_param_server_clear_len);
        uint32_t certified_hmac_key_server_clear_len = htonl(hmac_key_param_server_clear_len);

        uint32_t certified_encrypted_signing_len = htonl(encrypted_signing_len);

        //Total len
        len = sizeof(certified_symmetric_key_server_clear_len) + sizeof(certified_hmac_key_server_clear_len) + sizeof(certified_encrypted_signing_len) + IV_LENGTH + symmetric_key_param_server_clear_len + hmac_key_param_server_clear_len + encrypted_signing_len;

        serialized_pkt = (uint8_t *)malloc(len);
        if (!serialized_pkt)
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            return nullptr;
        }

        // Start copying
        memcpy(serialized_pkt, &certified_symmetric_key_server_clear_len, sizeof(certified_symmetric_key_server_clear_len));
        pointer_counter += sizeof(certified_symmetric_key_server_clear_len);
        memcpy(serialized_pkt + pointer_counter, &certified_hmac_key_server_clear_len, sizeof(certified_hmac_key_server_clear_len));
        pointer_counter += sizeof(certified_hmac_key_server_clear_len);
        memcpy(serialized_pkt + pointer_counter, &certified_encrypted_signing_len, sizeof(certified_encrypted_signing_len));
        pointer_counter += sizeof(encrypted_signing_len);
        memcpy(serialized_pkt + pointer_counter, iv_cbc, IV_LENGTH);
        pointer_counter += IV_LENGTH;
        memcpy(serialized_pkt + pointer_counter, key_buffer_symmetric_server_clear, symmetric_key_param_server_clear_len);
        pointer_counter += symmetric_key_param_server_clear_len;
        memcpy(serialized_pkt + pointer_counter, key_buffer_hmac_server_clear, hmac_key_param_server_clear_len);
        pointer_counter += hmac_key_param_server_clear_len;
        memcpy(serialized_pkt + pointer_counter, encrypted_signing, encrypted_signing_len);

        // Frees
        free(key_buffer_symmetric_server_clear);
        free(key_buffer_hmac_server_clear);

        return serialized_pkt;
    }

    void* serialize_message_no_clear_keys(int &len)
    {
        int pointer_counter = 0;
        uint8_t* serialized_pkt;

        if (encrypted_signing == nullptr || encrypted_signing_len == 0 || iv_cbc == nullptr)
        {
            cerr << "ERR: Missing field!" << endl;
            return nullptr;
        }

        uint32_t certified_encrypted_signing_len = htonl(encrypted_signing_len);

        //Total len
        len = sizeof(certified_encrypted_signing_len) + IV_LENGTH + cert_len + encrypted_signing_len;

        serialized_pkt = (uint8_t *)malloc(len);
        if (!serialized_pkt)
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            return nullptr;
        }

        // Start copying
        memcpy(serialized_pkt + pointer_counter, &certified_encrypted_signing_len, sizeof(certified_encrypted_signing_len));
        pointer_counter += sizeof(encrypted_signing_len);
        memcpy(serialized_pkt + pointer_counter, iv_cbc, IV_LENGTH);
        pointer_counter += IV_LENGTH;
        memcpy(serialized_pkt + pointer_counter, encrypted_signing, encrypted_signing_len);

        return serialized_pkt;
    }

    bool deserialize_message(uint8_t* serialized_pkt_received)
    {
        uint64_t pointer_counter = 0;

        if (iv_cbc != nullptr)
        {
            iv_cbc = nullptr;
        }

        // From the serialized_pkt_received we get all the lengths and then the keys
        memcpy(&symmetric_key_param_server_clear_len, serialized_pkt_received + pointer_counter, sizeof(symmetric_key_param_server_clear_len));
        symmetric_key_param_server_clear_len = ntohl(symmetric_key_param_server_clear_len);
        if (pointer_counter > numeric_limits<uint64_t>::max() - sizeof(symmetric_key_param_server_clear_len))
        {
            return false;
        }
        pointer_counter += sizeof(symmetric_key_param_server_clear_len);

        memcpy(&hmac_key_param_server_clear_len, serialized_pkt_received + pointer_counter, sizeof(hmac_key_param_server_clear_len));
        hmac_key_param_server_clear_len = ntohl(hmac_key_param_server_clear_len);
        if (pointer_counter > numeric_limits<uint64_t>::max() - sizeof(hmac_key_param_server_clear_len))
        {
            return false;
        }
        pointer_counter += sizeof(hmac_key_param_server_clear_len);

        memcpy(&encrypted_signing_len, serialized_pkt_received + pointer_counter, sizeof(encrypted_signing_len));
        encrypted_signing_len = ntohl(encrypted_signing_len);
        if (pointer_counter > numeric_limits<uint64_t>::max() - sizeof(encrypted_signing_len))
        {
            return false;
        }
        pointer_counter += sizeof(encrypted_signing_len);

        iv_cbc = (unsigned char *)malloc(IV_LENGTH);
        if (!iv_cbc)
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            return false;
        }
        memcpy(iv_cbc, serialized_pkt_received + pointer_counter, IV_LENGTH);
        if (pointer_counter > numeric_limits<uint64_t>::max() - IV_LENGTH)
        {
            return false;
        }
        pointer_counter += IV_LENGTH;

        symmetric_key_param_server_clear = deserialize_evp_pkey(serialized_pkt_received + pointer_counter, symmetric_key_param_server_clear_len);
        if (pointer_counter > numeric_limits<uint64_t>::max() - symmetric_key_param_server_clear_len)
        {
            return false;
        }
        pointer_counter += symmetric_key_param_server_clear_len;
        if (symmetric_key_param_server_clear == nullptr)
        {
            cerr << "error in deserialization of symmetric key param" << endl;
            return false;
        }

        hmac_key_param_server_clear = deserialize_evp_pkey(serialized_pkt_received + pointer_counter, hmac_key_param_server_clear_len);
        if (pointer_counter > numeric_limits<uint64_t>::max() - hmac_key_param_server_clear_len)
        {
            return false;
        }
        pointer_counter += hmac_key_param_server_clear_len;
        if (hmac_key_param_server_clear == nullptr)
        {
            cerr << "error in deserialization of hmac key param" << endl;
            return false;
        }

        encrypted_signing = (uint8_t *)malloc(encrypted_signing_len);
        if (!encrypted_signing)
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            return false;
        }
        memcpy(encrypted_signing, serialized_pkt_received + pointer_counter, encrypted_signing_len);
        if (pointer_counter > numeric_limits<uint64_t>::max() - encrypted_signing_len)
        {
            return false;
        }
        pointer_counter += encrypted_signing_len;

        return true;
    }

    bool deserialize_message_no_clear_keys(uint8_t *serialized_pkt_received)
    {
        uint64_t pointer_counter = 0;

        if (iv_cbc != nullptr)
        {
            iv_cbc = nullptr;
        }

        memcpy(&encrypted_signing_len, serialized_pkt_received + pointer_counter, sizeof(encrypted_signing_len));
        encrypted_signing_len = ntohl(encrypted_signing_len);
        if (pointer_counter > numeric_limits<uint64_t>::max() - sizeof(encrypted_signing_len))
        {
            return false;
        }
        pointer_counter += sizeof(encrypted_signing_len);

        iv_cbc = (unsigned char *)malloc(IV_LENGTH);
        if (!iv_cbc)
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            return false;
        }
        memcpy(iv_cbc, serialized_pkt_received + pointer_counter, IV_LENGTH);
        if (pointer_counter > numeric_limits<uint64_t>::max() - IV_LENGTH)
        {
            return false;
        }
        pointer_counter += IV_LENGTH;

        encrypted_signing = (uint8_t *)malloc(encrypted_signing_len);
        if (!encrypted_signing)
        {
            cerr << "encrypted signing malloc failed" << endl;
            return false;
        }

        memcpy(encrypted_signing, serialized_pkt_received + pointer_counter, encrypted_signing_len);

        return true;
    }
};

struct client_pkt
{
    // Filled before serialization and after deserialization_decrypted
    uint16_t code;
    string receiverAndAmount;
    uint32_t counter;

    bool deserialize_plaintext(uint8_t *serialized_decrypted_pkt)
    {

        string s = (char *)serialized_decrypted_pkt;
        string delimiter = "/";
        unsigned int pos;

        // Extract the code
        pos = s.find(delimiter);
        if (pos != string::npos)
        {
            string i = s.substr(0, pos);
            code = stoi(i);
            if (code != BALANCE && code != TRANSFER && code != HISTORY && code != LOGOUT)
            {
                cout << code << endl;
                return false;
            }
            s.erase(0, pos + delimiter.length());
        }

        // Extract the receiverAndAmount
        pos = s.find(delimiter);
        string i = s.substr(0, pos);
        receiverAndAmount = i;
        s.erase(0, pos + delimiter.length());

        // Extract the counter
        pos = s.find(delimiter);
        if (pos != string::npos)
        {
            string i = s.substr(0, pos);
            counter = stoul(i);
            s.erase(0, pos + delimiter.length());
        }

        free(serialized_decrypted_pkt);
        return true;
    }

    string serializePacket()
    {
        return to_string(this->code) + "/" + this->receiverAndAmount + "/" + to_string(this->counter) + "/";
    }
};

struct server_pkt
{
    // Filled before serialization and after deserialization_decrypted
    uint16_t code;
    uint32_t response;
    string response_output;
    uint32_t counter;

    bool deserialize_plaintext(uint8_t *serialized_decrypted_pkt)
    {

        string s = (char *)serialized_decrypted_pkt;
        string delimiter = "/";
        unsigned int pos;

        // Extract the code
        pos = s.find(delimiter);
        if (pos != string::npos)
        {
            string i = s.substr(0, pos);
            code = stoi(i);
            if (code != BALANCE && code != TRANSFER && code != HISTORY && code != LOGOUT)
            {
                return false;
            }
            s.erase(0, pos + delimiter.length());
        }

        // Extract the response
        pos = s.find(delimiter);
        if (pos != string::npos)
        {
            string i = s.substr(0, pos);
            response = stoi(i);
            s.erase(0, pos + delimiter.length());
        }

        // Extract the response_output
        pos = s.find(delimiter);
        string i = s.substr(0, pos);
        response_output = i;
        s.erase(0, pos + delimiter.length());

        // Extract the counter
        pos = s.find(delimiter);
        if (pos != string::npos)
        {
            string i = s.substr(0, pos);
            counter = stoul(i);
            s.erase(0, pos + delimiter.length());
        }

        free(serialized_decrypted_pkt);
        return true;
    }

    string serializePacket()
    {
        return to_string(this->code) + "/" + to_string(this->response) + "/" + this->response_output + "/" + to_string(this->counter) + "/";
    }
};

struct generic_message
{
    unsigned char* iv;
    uint32_t cipher_len;
    uint8_t* ciphertext;
    unsigned char* HMAC;

    bool deserialize_message(uint8_t *serialized_pkt)
    {
        int pointer_counter = 0;

        iv = (unsigned char *)malloc(IV_LENGTH);
        if (!iv)
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            free(serialized_pkt);
            return false;
        }
        memset(iv, 0, IV_LENGTH);
        // Copy of the iv
        memcpy(iv, serialized_pkt + pointer_counter, IV_LENGTH);
        pointer_counter += IV_LENGTH;

        // Copy of the ciphertext length
        memcpy(&cipher_len, serialized_pkt + pointer_counter, sizeof(cipher_len));
        cipher_len = ntohl(cipher_len);
        pointer_counter += sizeof(cipher_len);

        // Check for tainted cipherlen
        if (cipher_len >= MAX_PKT_SIZE)
        {
            cerr << "ERR: Possible tainted cipher received!" << endl;
            free(serialized_pkt);
            free(iv);
            return false;
        }

        ciphertext = (uint8_t *)malloc(cipher_len);
        if (!ciphertext)
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            free(serialized_pkt);
            free(iv);
            return false;
        }
        memset(ciphertext, 0, cipher_len);
        memcpy(ciphertext, serialized_pkt + pointer_counter, cipher_len);
        pointer_counter += cipher_len;

        HMAC = (unsigned char *)malloc(HMAC_LENGTH);
        if (!HMAC)
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            free(serialized_pkt);
            free(ciphertext);
            free(iv);
            return false;
        }
        memset(HMAC, 0, HMAC_LENGTH);

        // Copy of the ciphertext
        memcpy(HMAC, serialized_pkt + pointer_counter, HMAC_LENGTH);
        pointer_counter += HMAC_LENGTH;

        free(serialized_pkt);

        return true;
    }

    int deserialize_code(uint8_t *serialized_decrypted_pkt)
    {

        unsigned short code = -1;

        string s = (char *)serialized_decrypted_pkt;
        string delimiter = "/";
        unsigned int pos;

        // Extract the code
        pos = s.find(delimiter);
        if (pos != string::npos)
        {
            string i = s.substr(0, pos);
            code = stoi(i);
        }

        return code;
    }

    void *serialize_message(int &len)
    {
        uint8_t *serialized_pkt = nullptr;
        int pointer_counter = 0;

        len = (sizeof(cipher_len) + cipher_len + IV_LENGTH + HMAC_LENGTH);

        serialized_pkt = (uint8_t *)malloc(len);
        if (!serialized_pkt)
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            return nullptr;
        }

        uint32_t certif_ciph_len = htonl(cipher_len);

        // Adding iv
        uint8_t *cert_iv = (uint8_t *)iv;
        memcpy(serialized_pkt + pointer_counter, cert_iv, IV_LENGTH);
        pointer_counter += IV_LENGTH;

        // Adding ciphertext len
        memcpy(serialized_pkt + pointer_counter, &certif_ciph_len, sizeof(certif_ciph_len));
        pointer_counter += sizeof(certif_ciph_len);

        // Adding ciphertext
        memcpy(serialized_pkt + pointer_counter, ciphertext, cipher_len);
        pointer_counter += cipher_len;

        // Adding Hmac
        uint8_t *cert_hmac = (uint8_t *)HMAC;
        memcpy(serialized_pkt + pointer_counter, cert_hmac, HMAC_LENGTH);
        pointer_counter += HMAC_LENGTH;

        return serialized_pkt;
    }
};