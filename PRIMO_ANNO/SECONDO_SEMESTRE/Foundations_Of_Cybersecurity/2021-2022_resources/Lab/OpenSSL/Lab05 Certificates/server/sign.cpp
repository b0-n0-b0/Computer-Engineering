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

   // read the file to sign from keyboard:
   string clear_file_name;
   cout << "Please, type the file to sign: ";
   getline(cin, clear_file_name);
   if(!cin) { cerr << "Error during input\n"; exit(1); }

   // open the file to sign:
   FILE* clear_file = fopen(clear_file_name.c_str(), "rb");
   if(!clear_file) { cerr << "Error: cannot open file '" << clear_file_name << "' (file does not exist?)\n"; exit(1); }

   // get the file size: 
   // (assuming no failures in fseek() and ftell())
   fseek(clear_file, 0, SEEK_END);
   long int clear_size = ftell(clear_file);
   fseek(clear_file, 0, SEEK_SET);

   // read the plaintext from file:
   unsigned char* clear_buf = (unsigned char*)malloc(clear_size);
   if(!clear_buf) { cerr << "Error: malloc returned NULL (file too big?)\n"; exit(1); }
   ret = fread(clear_buf, 1, clear_size, clear_file);
   if(ret < clear_size) { cerr << "Error while reading file '" << clear_file_name << "'\n"; exit(1); }
   fclose(clear_file);

   // declare some useful variables:
   const EVP_MD* md = EVP_sha256();

   // create the signature context:
   EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
   if(!md_ctx){ cerr << "Error: EVP_MD_CTX_new returned NULL\n"; exit(1); }

   // allocate buffer for signature:
   unsigned char* sgnt_buf = (unsigned char*)malloc(EVP_PKEY_size(prvkey));
   if(!sgnt_buf) { cerr << "Error: malloc returned NULL (signature too big?)\n"; exit(1); }

   // sign the plaintext:
   // (perform a single update on the whole plaintext, 
   // assuming that the plaintext is not huge)
   ret = EVP_SignInit(md_ctx, md);
   if(ret == 0){ cerr << "Error: EVP_SignInit returned " << ret << "\n"; exit(1); }
   ret = EVP_SignUpdate(md_ctx, clear_buf, clear_size);
   if(ret == 0){ cerr << "Error: EVP_SignUpdate returned " << ret << "\n"; exit(1); }
   unsigned int sgnt_size;
   ret = EVP_SignFinal(md_ctx, sgnt_buf, &sgnt_size, prvkey);
   if(ret == 0){ cerr << "Error: EVP_SignFinal returned " << ret << "\n"; exit(1); }

   // delete the digest and the private key from memory:
   EVP_MD_CTX_free(md_ctx);
   EVP_PKEY_free(prvkey);

   // write the signature into a '.sgn' file:
   string sgnt_file_name = clear_file_name + ".sgn";
   FILE* sgnt_file = fopen(sgnt_file_name.c_str(), "wb");
   if(!sgnt_file) { cerr << "Error: cannot open file '" << sgnt_file_name << "' (no permissions?)\n"; exit(1); }
   ret = fwrite(sgnt_buf, 1, sgnt_size, sgnt_file);
   if(ret < sgnt_size) { cerr << "Error while writing the file '" << sgnt_file_name << "'\n"; exit(1); }
   fclose(sgnt_file);

   cout << "File '"<< clear_file_name << "' signed into file '" << sgnt_file_name << "'\n";

   // deallocate buffers:
   free(clear_buf);
   free(sgnt_buf);

   return 0;
}
