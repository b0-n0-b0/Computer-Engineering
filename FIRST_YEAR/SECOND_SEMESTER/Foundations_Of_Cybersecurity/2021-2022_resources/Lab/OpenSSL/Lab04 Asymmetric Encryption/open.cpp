#include <iostream> 
#include <string>
#include <stdio.h> // for fopen(), etc.
#include <limits.h> // for INT_MAX
#include <string.h> // for memset()
#include <openssl/evp.h>
#include <openssl/pem.h>

using namespace std;

int main() {
   int ret; // used for return values

   // read my private key file from keyboard:
   // if the file is protected, a prompt shows up automatically
   string prvkey_file_name;
   cout << "Please, type the PEM file containing my private key: ";
   getline(cin, prvkey_file_name);
   if(!cin) { cerr << "Error during input\n"; exit(1); }

   // load my private key:
   FILE* prvkey_file = fopen(prvkey_file_name.c_str(), "r");
   if(!prvkey_file){ cerr << "Error: cannot open file '" << prvkey_file_name << "' (missing?)\n"; exit(1); }
   EVP_PKEY* prvkey = PEM_read_PrivateKey(prvkey_file, NULL, NULL, NULL);
   fclose(prvkey_file);
   if(!prvkey){ cerr << "Error: PEM_read_PrivateKey returned NULL\n"; exit(1); }

   // read the file to decrypt from keyboard:
   string cphr_file_name;
   cout << "Please, type the file to decrypt: ";
   getline(cin, cphr_file_name);
   if(!cin) { cerr << "Error during input\n"; exit(1); }

   // open the file to decrypt:
   FILE* cphr_file = fopen(cphr_file_name.c_str(), "rb");
   if(!cphr_file) { cerr << "Error: cannot open file '" << cphr_file_name << "' (file does not exist?)\n"; exit(1); }

   // get the file size: 
   // (assuming no failures in fseek() and ftell())
   fseek(cphr_file, 0, SEEK_END);
   long int cphr_file_size = ftell(cphr_file);
   fseek(cphr_file, 0, SEEK_SET);

   // declare some useful variables:
   const EVP_CIPHER* cipher = EVP_aes_128_cbc();
   int encrypted_key_len = EVP_PKEY_size(prvkey);
   int iv_len = EVP_CIPHER_iv_length(cipher);

   // check for possible integer overflow in (encrypted_key_len + iv_len)
   // (theoretically possible if the encrypted key is too big):
   if(encrypted_key_len > INT_MAX - iv_len) { cerr << "Error: integer overflow (encrypted key too big?)\n"; exit(1); }
   // check for correct format of the encrypted file
   // (size must be >= encrypted key size + IV + 1 block):
   if(cphr_file_size < encrypted_key_len + iv_len) { cerr << "Error: encrypted file with wrong format\n"; exit(1); }

   // allocate buffers for encrypted key, IV, ciphertext, and plaintext:
   unsigned char* encrypted_key = (unsigned char*)malloc(encrypted_key_len);
   unsigned char* iv = (unsigned char*)malloc(iv_len);
   int cphr_size = cphr_file_size - encrypted_key_len - iv_len;
   unsigned char* cphr_buf = (unsigned char*)malloc(cphr_size);
   unsigned char* clear_buf = (unsigned char*)malloc(cphr_size);
   if(!encrypted_key || !iv || !cphr_buf || !clear_buf) { cerr << "Error: malloc returned NULL (file too big?)\n"; exit(1); }

   // read the encrypted key, the IV, and the ciphertext from file:
   ret = fread(encrypted_key, 1, encrypted_key_len, cphr_file);
   if(ret < encrypted_key_len) { cerr << "Error while reading file '" << cphr_file_name << "'\n"; exit(1); }
   ret = fread(iv, 1, iv_len, cphr_file);
   if(ret < iv_len) { cerr << "Error while reading file '" << cphr_file_name << "'\n"; exit(1); }
   ret = fread(cphr_buf, 1, cphr_size, cphr_file);
   if(ret < cphr_size) { cerr << "Error while reading file '" << cphr_file_name << "'\n"; exit(1); }
   fclose(cphr_file);

   // create the envelope context:
   EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
   if(!ctx){ cerr << "Error: EVP_CIPHER_CTX_new returned NULL\n"; exit(1); }

   // decrypt the ciphertext:
   // (perform a single update on the whole ciphertext, 
   // assuming that the ciphertext is not huge)
   ret = EVP_OpenInit(ctx, cipher, encrypted_key, encrypted_key_len, iv, prvkey);
   if(ret == 0){ cerr << "Error: EVP_OpenInit returned " << ret << "\n"; exit(1); }
   int nd = 0; // bytes decrypted at each chunk
   int ndtot = 0; // total decrypted bytes
   ret = EVP_OpenUpdate(ctx, clear_buf, &nd, cphr_buf, cphr_size);
   if(ret == 0){ cerr << "Error: EVP_OpenUpdate returned " << ret << "\n"; exit(1); }
   ndtot += nd;
   ret = EVP_OpenFinal(ctx, clear_buf + ndtot, &nd);
   if(ret == 0){ cout << "Error: EVP_OpenFinal returned " << ret << " (corrupted file?)\n"; exit(1); }
   ndtot += nd;
   int clear_size = ndtot;

   // delete the symmetric key and the private key from memory:
   EVP_CIPHER_CTX_free(ctx);
   EVP_PKEY_free(prvkey);

   // write the plaintext into a '.dec' file:
   string clear_file_name = cphr_file_name + ".dec";
   FILE* clear_file = fopen(clear_file_name.c_str(), "wb");
   if(!clear_file) { cerr << "Error: cannot open file '" << clear_file_name << "' (no permissions?)\n"; exit(1); }
   ret = fwrite(clear_buf, 1, clear_size, clear_file);
   if(ret < clear_size) { cerr << "Error while writing the file '" << clear_file_name << "'\n"; exit(1); }
   fclose(clear_file);

   // delete the plaintext from memory:
   memset(clear_buf, 0, clear_size);
   free(clear_buf);

   cout << "File '"<< cphr_file_name << "' decrypted into file '" << clear_file_name << "', clear size is " << clear_size << " bytes\n";

   // deallocate buffers:
   free(encrypted_key);
   free(iv);
   free(cphr_buf);

   return 0;
}
