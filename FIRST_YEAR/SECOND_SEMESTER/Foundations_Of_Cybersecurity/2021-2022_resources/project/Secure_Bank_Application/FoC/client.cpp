#include <stdlib.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fstream>
#include <cstdint>
#include <vector>
#include <sys/stat.h>
#include <sstream>
#include <math.h>
#include "defines.h"
#include "./cPackets.h"

using namespace std;

// Generic vars
string username;
string password;
unsigned long port;
int sessionSocket = -1;
const string serverIp = SERVER_IP;
sockaddr_in serverAddress;
uint32_t counter = 0;

string server_pubK_path = "./server_pubK.pem";

// iv
unsigned char *iv = nullptr;
int iv_size = EVP_CIPHER_iv_length(EVP_aes_128_cbc());

// Keys
EVP_PKEY *private_key = nullptr;
EVP_PKEY *server_pubk = nullptr;
unsigned char *symmetric_key = nullptr;
unsigned char *hmac_key = nullptr;
int symmetric_key_length = EVP_CIPHER_key_length(EVP_aes_256_gcm());
int hmac_key_length = HMAC_KEY_SIZE;

// Receive message from socket
int receive_message(unsigned char *&recv_buffer, uint32_t &len)
{
    ssize_t ret;
    // Receive message length
    ret = recv(sessionSocket, &len, sizeof(uint32_t), 0);
    if (ret == 0)
    {
        cerr << "ERR: server disconnected" << endl
             << endl;
        return -2;
    }
    if (ret < 0 || (unsigned long)ret < sizeof(len))
    {
        cerr << "ERR: message length received is too short" << endl
             << endl;
        return -1;
    }
    try
    {
        // Allocate receive buffer
        len = ntohl(len);
        recv_buffer = (unsigned char *)malloc(len);
        if (!recv_buffer)
        {
            cerr << "ERR: recv_buffer malloc fail" << endl
                 << endl;
            throw 1;
        }
        // receive message
        ret = recv(sessionSocket, recv_buffer, len, 0);
        if (ret == 0)
        {
            cerr << "ERR: Client disconnected" << endl
                 << endl;
            throw 2;
        }
        if (ret < 0 || (unsigned long)ret < sizeof(len))
        {
            cerr << "ERR: Message received is too short" << endl
                 << endl;
            throw 3;
        }
    }
    catch (int error_code)
    {
        free(recv_buffer);
        if (error_code == 2)
        {
            return -2;
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

// Send message through socket
bool send_message(void *msg, const uint32_t len)
{
    ssize_t ret;
    uint32_t actual_len = htonl(len);
    // send message length
    ret = send(sessionSocket, &actual_len, sizeof(actual_len), 0);
    // -1 error, if returns 0 no bytes are sent
    if (ret <= 0)
    {
        cerr << "Error: message length not sent" << endl;
        return false;
    }
    // send message
    ret = send(sessionSocket, msg, len, 0);
    // -1 error, if returns 0 no bytes are sent
    if (ret <= 0)
    {
        cerr << "Error: message not sent" << endl;
        return false;
    }
    return true;
}

// Send first packet to start the comm
void send_wave_pkt(wave_pkt &pkt)
{
    unsigned char *send_buffer;
    int len;
    unsigned char *to_copy;

    pkt.code = WAVE;
    pkt.username = username;

    pkt.symmetric_key_param = generate_dh_key();
    pkt.hmac_key_param = generate_dh_key();

    if (pkt.symmetric_key_param == nullptr || pkt.hmac_key_param == nullptr)
    {
        throw 1;
    }

    to_copy = (unsigned char *)pkt.serialize_message(len);

    if (to_copy == nullptr)
    {
        free(to_copy);
        throw 2;
    }

    send_buffer = (unsigned char *)malloc(len);

    if (!send_buffer)
    {
        free(send_buffer);
        free(to_copy);
        throw 3;
    }

    memcpy(send_buffer, to_copy, len);

    if (!send_message(send_buffer, len))
    {
        free(send_buffer);
        free(to_copy);
        throw 4;
    }

    free(send_buffer);
    free(to_copy);
}

// Receive the server authentication packet
void receive_login_server_authentication(wave_pkt &hello_pkt, login_authentication_pkt &pkt)
{
    int ret;
    unsigned char *receive_buffer;
    uint32_t len;
    unsigned char *symmetric_key_no_hashed;
    unsigned char *hmac_key_no_hashed;
    unsigned char *plaintext;
    uint32_t plainlen;
    unsigned char *signed_text;
    int signed_text_len;

    // Receive message
    if (receive_message(receive_buffer, len) < 0)
    {
        free(receive_buffer);
        throw 1;
    }

    // Deserialize the message to read clearly the message
    if (!pkt.deserialize_message(receive_buffer))
    {
        free(receive_buffer);
        throw 2;
    }

    // Derive symmetric key and hmac key, hash them and take a portion of the hash for the 128 bit key
    symmetric_key_no_hashed = derive_shared_secret(hello_pkt.symmetric_key_param, pkt.symmetric_key_param_server_clear);

    if (!symmetric_key_no_hashed)
    {
        free(receive_buffer);
        throw 3;
    }

    ret = hash_symmetric_key(symmetric_key, symmetric_key_no_hashed);
    if (ret != 0)
    {
        free(receive_buffer);
        throw 4;
    }

    hmac_key_no_hashed = derive_shared_secret(hello_pkt.hmac_key_param, pkt.hmac_key_param_server_clear);
    if (!hmac_key_no_hashed)
    {
        throw 3;
    }

    ret = hash_hmac_key(hmac_key, hmac_key_no_hashed);
    if (ret != 0)
    {
        free(receive_buffer);
        throw 4;
    }

    // Clear the non hashed keys
    free(symmetric_key_no_hashed);
    free(hmac_key_no_hashed);

    // Decrypt using the key and the iv received
    if (iv != nullptr)
    {
        free(iv);
    }

    iv = (unsigned char *)malloc(iv_size);
    if (!iv)
    {
        free(receive_buffer);
        free(iv);
        iv = nullptr;
        throw 5;
    }

    memcpy(iv, pkt.iv_cbc, iv_size);
    free(pkt.iv_cbc);

    ret = cbc_decrypt(pkt.encrypted_signing, pkt.encrypted_signing_len, plaintext, plainlen, symmetric_key, iv);

    if (ret != 0)
    {
        free(receive_buffer);
        free(iv);
        iv = nullptr;
        free(plaintext);
        throw 6;
    }

    // Extract server public key
    FILE *server_pubkey_file = fopen("server_pubK.pem", "r");
    server_pubk = PEM_read_PUBKEY(server_pubkey_file, NULL, NULL, NULL);
    fclose(server_pubkey_file);

    if (server_pubk == nullptr)
    {
        free(receive_buffer);
        free(iv);
        iv = nullptr;
        free(plaintext);
        EVP_PKEY_free(server_pubk);
        throw 7;
    }

    // Save received fields
    pkt.symmetric_key_param_server = pkt.symmetric_key_param_server_clear;
    pkt.symmetric_key_param_len_server = pkt.symmetric_key_param_server_clear_len;
    pkt.hmac_key_param_server = pkt.hmac_key_param_server_clear;
    pkt.hmac_key_param_len_server = pkt.hmac_key_param_server_clear_len;
    pkt.symmetric_key_param_client = hello_pkt.symmetric_key_param;
    pkt.symmetric_key_param_len_client = hello_pkt.symmetric_key_param_len;
    pkt.hmac_key_param_client = hello_pkt.hmac_key_param;
    pkt.hmac_key_param_len_client = hello_pkt.hmac_key_param_len;

    // We serialize before encrypting and sending
    unsigned char *to_copy = (unsigned char *)pkt.serialize_part_to_encrypt(signed_text_len);

    signed_text = (unsigned char *)malloc(signed_text_len);
    if (!signed_text)
    {
        free(receive_buffer);
        free(iv);
        iv = nullptr;
        free(plaintext);
        EVP_PKEY_free(server_pubk);
        free(signed_text);
        throw 5;
    }
    memcpy(signed_text, to_copy, signed_text_len);

    // Verify the signature
    ret = verify_signature(server_pubk, plaintext, plainlen, signed_text, signed_text_len);
    if (ret != 0)
    {
        free(receive_buffer);
        free(iv);
        iv = nullptr;
        free(plaintext);
        EVP_PKEY_free(server_pubk);
        free(signed_text);
        throw 8;
    }

    // Frees
    free(signed_text);
    free(to_copy);
    free(receive_buffer);
    free(plaintext);
}

// Send the client authentication packet encrypted
void send_login_client_authentication(login_authentication_pkt &pkt)
{
    unsigned char *part_to_encrypt;
    int pte_len;
    int final_pkt_len;
    unsigned int signature_len;
    unsigned char *signature;
    unsigned char *ciphertext;
    unsigned char *final_pkt;
    unsigned char *to_copy;
    int cipherlen;
    int ret;

    // Serialize the part to encrypt
    to_copy = (unsigned char *)pkt.serialize_part_to_encrypt(pte_len);
    if (to_copy == nullptr)
        throw 1;

    part_to_encrypt = (unsigned char *)malloc(pte_len);
    if (part_to_encrypt == nullptr)
    {
        free(to_copy);
        throw 2;
    }

    memcpy(part_to_encrypt, to_copy, pte_len);

    // Sign the document
    signature = sign_message(private_key, part_to_encrypt, pte_len, signature_len);
    if (signature == nullptr)
    {
        free(to_copy);
        free(part_to_encrypt);
        throw 3;
    }

    iv = generate_iv(); // THROWS 0

    // Encrypt
    ret = cbc_encrypt(signature, signature_len, ciphertext, cipherlen, symmetric_key, iv);
    if (ret != 0)
    {
        free(to_copy);
        free(part_to_encrypt);
        free(signature);
        throw 3;
    }

    // Assign to packet values
    pkt.iv_cbc = iv;
    pkt.encrypted_signing = ciphertext;
    pkt.encrypted_signing_len = cipherlen;

    // Final serialization
    free(to_copy);
    free(part_to_encrypt);

    to_copy = (unsigned char *)pkt.serialize_message_no_clear_keys(final_pkt_len);

    final_pkt = (unsigned char *)malloc(final_pkt_len);
    if (!final_pkt)
    {
        free(ciphertext);
        free(iv);
        iv = nullptr;
        free(signature);
        free(to_copy);
        free(final_pkt);
        throw 2;
    }

    memcpy(final_pkt, to_copy, final_pkt_len);

    if (!send_message(final_pkt, final_pkt_len))
    {
        free(ciphertext);
        free(iv);
        iv = nullptr;
        free(signature);
        free(to_copy);
        free(final_pkt);
        throw 4;
    }

    // Frees
    free(ciphertext);
    free(iv);
    iv = nullptr;
    free(signature);
    free(to_copy);
    free(final_pkt);
}

// Function to establish a symmetric key
bool init_session()
{
    wave_pkt hello_pkt;
    login_authentication_pkt server_auth_pkt;
    login_authentication_pkt client_auth_pkt;

    cout << "CONNECTING TO SERVER" << endl;

    // Send wave
    try
    {
        send_wave_pkt(hello_pkt);
    }
    catch (int error_code)
    {
        switch (error_code)
        {
        case 1:
        {
            cerr << "ERR: Couldn't generate a session key parameter!" << endl;
            break;
        }
        case 2:
        {
            cerr << "ERR: Couldn't serialize wave packet!" << endl;
            break;
        }
        case 3:
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            break;
        }
        case 4:
        {
            cerr << "ERR: Couldn't send wave packet!" << endl;
            break;
        }
        }
        EVP_PKEY_free(hello_pkt.symmetric_key_param);
        EVP_PKEY_free(hello_pkt.hmac_key_param);
        return false;
    }

    cout << "WAITING FOR SERVER AUTHENTICATION" << endl;

    // Receive login_server_authentication_pkt
    try
    {
        receive_login_server_authentication(hello_pkt, server_auth_pkt);
    }
    catch (int error_code)
    {
        switch (error_code)
        {
        case 1:
        {
            cerr << "ERR: Error in the received login_authentication_pkt" << endl;
            break;
        }
        case 2:
        {
            cerr << "ERR: some error in deserialize pkt" << endl;
            break;
        }
        case 3:
        {
            cerr << "ERR: Couldn't derive symmetric key or hmac key!" << endl;
            break;
        }
        case 4:
        {
            cerr << "ERR: Couldn't hash symmetric key or hmac key!" << endl;
            break;
        }
        case 5:
        {
            cerr << "ERR: Couldn't malloc!" << endl;
            break;
        }
        case 6:
        {
            cerr << "ERR: Couldn't decrypt server authentication packet!" << endl;
            break;
        }
        case 7:
        {
            cerr << "ERR: Couldn't extract server's public key!" << endl;
            break;
        }
        case 8:
        {
            cerr << "ERR: Couldn't verify the signature!" << endl;
            break;
        }
        }
        return false;
    }

    cout << "SERVER CORRECTLY AUTHENTICATED" << endl;

    client_auth_pkt.symmetric_key_param_server = server_auth_pkt.symmetric_key_param_server_clear;
    client_auth_pkt.symmetric_key_param_len_server = server_auth_pkt.symmetric_key_param_server_clear_len;
    client_auth_pkt.hmac_key_param_server = server_auth_pkt.hmac_key_param_server_clear;
    client_auth_pkt.hmac_key_param_len_server = server_auth_pkt.hmac_key_param_server_clear_len;
    client_auth_pkt.symmetric_key_param_client = hello_pkt.symmetric_key_param;
    client_auth_pkt.symmetric_key_param_len_client = hello_pkt.symmetric_key_param_len;
    client_auth_pkt.hmac_key_param_client = hello_pkt.hmac_key_param;
    client_auth_pkt.hmac_key_param_len_client = hello_pkt.hmac_key_param_len;

    // Send login_client_authentication_pkt
    try
    {
        send_login_client_authentication(client_auth_pkt);
    }
    catch (int error_code)
    {
        switch (error_code)
        {
        case 0:
        {
            cerr << "ERR: Couldn't generate iv!" << endl;
            break;
        }
        case 1:
        {
            cerr << "ERR: Couldn't serialize part to encrypt!" << endl;
            break;
        }
        case 2:
        {
            cerr << "ERR: Failed malloc!" << endl;
            break;
        }
        case 3:
        {
            cerr << "ERR: Couldn't generate a valid signature or ciphertext!" << endl;
            break;
        }
        case 4:
        {
            cerr << "ERR: Couldn't send message to the server!" << endl;
            break;
        }
        }
        return false;
    }

    // Frees
    EVP_PKEY_free(hello_pkt.symmetric_key_param);
    EVP_PKEY_free(hello_pkt.hmac_key_param);
    EVP_PKEY_free(server_auth_pkt.symmetric_key_param_server_clear);
    EVP_PKEY_free(server_auth_pkt.hmac_key_param_server_clear);

    return true;
}

bool encrypt_generate_HMAC_and_send(string buffer)
{
    // Generic Packet
    generic_message pkt;
    unsigned char* ciphertext;
    int cipherlen;
    unsigned char* data;
    int data_length;
    uint32_t MAC_len;
    unsigned char* HMAC;
    unsigned char* generated_MAC;

    // Encryption
    if (cbc_encrypt((unsigned char *)buffer.c_str(), buffer.length(), ciphertext, cipherlen, symmetric_key, iv) != 0)
    {
        cerr << "ERR: Couldn't decrypt!" << endl;
        free(ciphertext);
        ciphertext = nullptr;
        return false;
    }

    // Get the HMAC
    generated_MAC = (uint8_t *)malloc(IV_LENGTH + cipherlen + sizeof(cipherlen));
    if (!generated_MAC)
    {
        cerr << "ERR: Couldn't malloc!" << endl;
        return false;
    }

    // Clean allocated space and copy
    memset(generated_MAC, 0, IV_LENGTH + cipherlen + sizeof(cipherlen));
    memcpy(generated_MAC, iv, IV_LENGTH);
    memcpy(generated_MAC + IV_LENGTH, &cipherlen, sizeof(cipherlen));
    memcpy(generated_MAC + IV_LENGTH + sizeof(cipherlen), (void *)ciphertext, cipherlen);

    // Generate the HMAC on the receiving side iv||ciphertext
    generate_SHA256_HMAC(generated_MAC, IV_LENGTH + cipherlen + sizeof(cipherlen), HMAC, MAC_len, hmac_key, MAX_PKT_SIZE);

    // Initialization of the data to serialize
    pkt.ciphertext = (uint8_t *)ciphertext;
    pkt.cipher_len = cipherlen;
    pkt.iv = iv;
    pkt.HMAC = HMAC;

    data = (unsigned char *)pkt.serialize_message(data_length);

    //If we couldn't serialize the message!
    if (data == nullptr)
    {
        cerr << "ERR: Couldn't serialize!" << endl;
        free(generated_MAC);
        generated_MAC = nullptr;
        free(ciphertext);
        ciphertext = nullptr;
        free(pkt.HMAC);
        pkt.HMAC = nullptr;
        return false;
    }

    // Send the message
    if (!send_message((void *)data, data_length))
    {
        cerr << "ERR: Couldn't send message!" << endl;
        free(generated_MAC);
        generated_MAC = nullptr;
        free(ciphertext);
        ciphertext = nullptr;
        free(pkt.HMAC);
        pkt.HMAC = nullptr;
        free(data);
        data = nullptr;
        return false;
    }

    // Frees
    free(generated_MAC);
    generated_MAC = nullptr;
    free(ciphertext);
    ciphertext = nullptr;
    free(pkt.HMAC);
    pkt.HMAC = nullptr;
    free(data);
    data = nullptr;
    return true;
}

unsigned char* receive_decrypt_and_verify_HMAC()
{
    unsigned char* data;
    generic_message rcvd_pkt;
    uint32_t length_rec;
    unsigned char* plaintxt;
    uint32_t ptlen;
    uint32_t MAC_len;
    uint8_t* generated_MAC;
    uint8_t* HMAC;

    // Receive the serialized data
    int ret = receive_message(data, length_rec);
    if (ret != 0)
    {
        cerr << "ERR: some error in receiving MSG, received error: " << ret << endl;
        free(data);
        data = nullptr;
        return nullptr;
    }

    // Deserialize message
    if (!rcvd_pkt.deserialize_message(data))
    {
        cerr << "Error during deserialization of the data" << endl;
        free(data);
        data = nullptr;
        return nullptr;
    }

    free(iv);
    iv = nullptr;
    iv = rcvd_pkt.iv;

    generated_MAC = (uint8_t *)malloc(IV_LENGTH + rcvd_pkt.cipher_len + sizeof(rcvd_pkt.cipher_len));
    if (!generated_MAC)
    {
        cerr << "Error during malloc of generated_MAC" << endl;
        return nullptr;
    }

    // Clean allocated space and copy
    memset(generated_MAC, 0, IV_LENGTH + rcvd_pkt.cipher_len + sizeof(rcvd_pkt.cipher_len));
    memcpy(generated_MAC, rcvd_pkt.iv, IV_LENGTH);
    memcpy(generated_MAC + IV_LENGTH, &rcvd_pkt.cipher_len, sizeof(rcvd_pkt.cipher_len));
    memcpy(generated_MAC + IV_LENGTH + sizeof(rcvd_pkt.cipher_len), (void *)rcvd_pkt.ciphertext, rcvd_pkt.cipher_len);

    // Generate the HMAC to verify the correctness of the received message
    generate_SHA256_HMAC(generated_MAC, IV_LENGTH + rcvd_pkt.cipher_len + sizeof(rcvd_pkt.cipher_len), HMAC, MAC_len, hmac_key, MAX_PKT_SIZE);

    // Verify HMAC
    if (!verify_SHA256(HMAC, rcvd_pkt.HMAC))
    {
        cerr << "ERR: Couldn't verify HMAC, try again" << endl;
        free(generated_MAC);
        generated_MAC = nullptr;
        free(rcvd_pkt.HMAC);
        rcvd_pkt.HMAC = nullptr;
        return nullptr;
    }

    // Decrypt the ciphertext and obtain the plaintext
    if (cbc_decrypt((unsigned char *)rcvd_pkt.ciphertext, rcvd_pkt.cipher_len, plaintxt, ptlen, symmetric_key, iv) != 0)
    {
        cerr << "ERR: Couldn't encrypt!" << endl;
        free(generated_MAC);
        generated_MAC = nullptr;
        free(rcvd_pkt.HMAC);
        rcvd_pkt.HMAC = nullptr;
        return nullptr;
    }

    // Frees
    free(generated_MAC);
    generated_MAC = nullptr;
    free(HMAC);
    HMAC = nullptr;
    free(rcvd_pkt.HMAC);
    rcvd_pkt.HMAC = nullptr;
    return plaintxt;
}

string send_packet(int operation)
{
    client_pkt pkt;
    pkt.code = operation;
    pkt.receiverAndAmount = "empty";
    pkt.counter = counter;
    string buffer;

    if (operation == 2)
    {
        string receiverName;
        string prov;
        int amount;
        cout << "BANK: Transfering money to user:" << endl;
        cout << "ME: ";
        cin >> receiverName;
        if(!cin)
        {
            throw 6;
        }
        if (receiverName.find_first_not_of(USERNAME_WHITELIST_CHARS) != std::string::npos)
            throw 6;
        cout << "BANK: Amount to transfer:" << endl;
        cout << "ME: ";
        cin >> prov;
        if(!cin)
        {
            throw 6;
        }
        if(prov.find_first_not_of(TRANSFER_WHITELIST_NUMS) != std::string::npos)
            prov = "0";
        amount = stoi(prov);
        if (amount >= 999 || amount <= 0)
            throw 6;
        pkt.receiverAndAmount = receiverName + "|" + to_string(amount) + "|";
    }

    buffer = pkt.serializePacket();

    iv = generate_iv(); // THROWS 0

    if (!encrypt_generate_HMAC_and_send(buffer))
    {
        free(iv);
        iv = nullptr;
        throw 1;
    }

    counter++;

    // Receive the message, check the HMAC validity and decrypt the ciphertext
    unsigned char *plaintxt = receive_decrypt_and_verify_HMAC();

    if (plaintxt == nullptr)
    {
        free(iv);
        iv = nullptr;
        throw 2;
    }

    // Expected packet type
    server_pkt rcvd_pkt;

    // Deserialize & extracts plaintext (NOTE: Plaintext is freed in the function)
    if (!rcvd_pkt.deserialize_plaintext(plaintxt))
    {
        free(iv);
        iv = nullptr;
        throw 3;
    }

    // Check on rcvd packets values
    if (rcvd_pkt.counter != counter)
    {
        free(iv);
        iv = nullptr;
        throw 4;
    }

    // Check the response of the server
    if (rcvd_pkt.response != 1)
    {
        free(iv);
        iv = nullptr;
        throw 5;
    }

    free(iv);
    iv = nullptr;

    return rcvd_pkt.response_output;
}

int main(int argc, char **argv)
{
    // Check if port has been specified
    if (argc < 2)
    {
        cerr << "ERR: Port parameter is not present" << endl;
        return -1;
    }

    port = stoul(argv[1]);

    //------------ Collecting user data ------------//

    // Input username and check for the length
    cout << "Insert your username" << endl;
    cin >> username;
    if (!cin)
    {
        cerr << "ERR: Couldn't insert username" << endl;
        return -1;
    }
    if (username.length() > MAX_USERNAME_LENGTH || username.length() <= MIN_USERNAME_LENGTH)
    {
        cerr << "ERR: Username length not respected" << endl;
        return -1;
    }
    if (username.find_first_not_of(USERNAME_WHITELIST_CHARS) != std::string::npos)
    {
        cerr << "ERR: Username has been poorly formatted" << endl;
        return -1;
    }

    // Input password
    cout << "Insert your password" << endl;
    cin >> password;
    if (!cin)
    {
        cerr << "ERR: Couldn't insert password" << endl;
        return -1;
    }
    if (password.find_first_not_of(USERNAME_WHITELIST_CHARS) != std::string::npos)
    {
        cerr << "ERR: Password has been poorly formatted" << endl;
        return -1;
    }
    cout << endl;

    //----------------------------------------------//

    //-------------- Checking account --------------//

    // Searches the private key file
    string dir = "./" + username + "_privK.pem";
    FILE *file = fopen(dir.c_str(), "r");

    if (!file)
    {
        cerr << "ERR: Username or password are wrong" << endl;
        return -1;
    }

    // Tries to convalidate the password protected key
    EVP_PKEY *privk = PEM_read_PrivateKey(file, NULL, NULL, (void *)password.c_str());

    fclose(file);

    if (privk == nullptr)
    {
        cerr << "ERR: Username or password are wrong" << endl;
        return -1;
    }

    // Saves locally the private key
    private_key = privk;

    //----------------------------------------------//

    //------ Setting up the client connection ------//

    sessionSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (sessionSocket < 0)
    {
        cerr << "ERR: Socket creation failed" << endl;
        return -1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIp.c_str());
    serverAddress.sin_port = htons(port);

    // Connect to server
    if (connect(sessionSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        cerr << "ERR: Connection to server failed" << endl;
        return -1;
    }

    // cout << "Socket id: " << sessionSocket << " connected with the server" << endl;

    // Establish session and HMAC key
    if (!init_session())
    {
        cerr << "ERR: Session keys establishment failed" << endl;
        return -1;
    }

    cout << "SESSION KEYS HAVE BEEN ESTABLISHED CORRECTLY" << endl
         << endl;

    //----------------------------------------------//

    //--- Communication betwwen client and server ---//

    bool connected = true;

    cout << "---------------WELCOME TO THE BANK!---------------" << endl;

    while (connected)
    {
        string prov;
        int operation;

        if (counter == numeric_limits<uint32_t>::max() - 1) // Avoid overflow sending as last a logout packet!
        {
            cout << "--------------------------------------------------" << endl
             << "BANK: You have reached the maximum number of requests, you need to reconnect to the bank!" << endl;
            operation = 4;
        } else {
            cout << "--------------------------------------------------" << endl
             << "BANK: Insert the operation you want to perform:" << endl
             << "1: Balance(): Returns your bankId and balance" << endl
             << "2: Transfer(User, amount): Sends to the user the amount of money specified" << endl
             << "3: History(): Returns the list of transfers" << endl
             << "4: Logout(): Disconnects from the bank" << endl;
            cout << "ME: ";
            cin >> prov;
            if(!cin)
            {
                cerr << "ERR: Couldn't insert operation!" << endl;
                return -1;
            }
            if(prov.find_first_not_of(OPERATION_WHITELIST_NUMS) != std::string::npos)
                prov = "0";
            operation = stoi(prov);
            if (operation >= 5 || operation <= 0)
                operation = 0;
        }

        try
        {
            switch (operation)
            {
            case 1:
            {
                string result = send_packet(1);
                cout << "BANK: Your Id and balance are: " << result << endl;
                break;
            }
            case 2:
            {
                string result = send_packet(2);
                cout << "BANK: Operation performed!" << endl;
                break;
            }
            case 3:
            {
                string result = send_packet(3);
                cout << "BANK: --OPERATIONS--" << endl
                     << result << endl;
                break;
            }
            case 4:
            {
                connected = false;
                string result = send_packet(4);
                cout << "BANK: Goodbye!" << endl;
                break;
            }
            default:
            {
                cout << "BANK: Operation doesn't exist!" << endl;
                break;
            }
            }
        }
        catch (int error_code)
        {
            switch (error_code)
            {
                case 0:
                {
                    cerr << "ERR: Couldn't generate iv!" << endl;
                    break;
                }
                case 1:
                {
                    cerr << "ERR: Couldn't encrypt and generate HMAC!" << endl;
                    break;
                }
                case 2:
                {
                    cerr << "ERR: Could't verify the HMAC of the received message!" << endl;
                    break;
                }
                case 3:
                {
                    cerr << "ERR: Could't deserialized the received message!" << endl;
                    break;
                }
                case 4:
                {
                    cerr << "ERR: Counter of the the received message is not correct!" << endl;
                    break;
                }
                case 5:
                {
                    cerr << "ERR: Operation was not possible!" << endl;
                    break;
                }
                case 6:
                {
                    cerr << "ERR: Reformat the request!" << endl;
                    break;
                }
            }
        }
    }
    return 0;
}