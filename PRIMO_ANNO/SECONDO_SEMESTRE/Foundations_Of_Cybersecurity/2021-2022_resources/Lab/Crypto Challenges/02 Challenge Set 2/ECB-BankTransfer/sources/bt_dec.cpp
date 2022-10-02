#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>
#include <iostream> 
#include <string>
#include <stdlib.h>
#include <stdio.h> // for fopen(), etc.
#include <limits.h> // for INT_MAX
#include <string.h> // for memset()
#include <openssl/pem.h>
#include <openssl/rand.h>
using namespace std;
void handleErrors(void)
{
	ERR_print_errors_fp(stderr);
	abort();
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
unsigned char *iv, unsigned char *plaintext)
{
	EVP_CIPHER_CTX *ctx;

	int len;

	int plaintext_len;

	/* Create and initialise the context */
	ctx = EVP_CIPHER_CTX_new();

	// Decrypt Init
	EVP_DecryptInit(ctx, EVP_aes_128_ecb(), key, iv);

	// Decrypt Update: one call is enough because our mesage is very short.
	if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
	handleErrors();
	plaintext_len = len;

	// Decryption Finalize
	if(1 != EVP_DecryptFinal(ctx, plaintext + len, &len)) handleErrors();
	plaintext_len += len;

	// Clean the context!
	EVP_CIPHER_CTX_free(ctx);

	return plaintext_len;
}



int main (void){

	int ret=0;
	//128 bit key (16 characters * 8 bit)
	unsigned char *key = (unsigned char *)malloc(16);
	string key_s;
	cout << "Please, insert 16 character pwd: ";
	getline(cin, key_s);
	if(!cin) { cerr << "Error during input\n"; exit(1); }
	key=(unsigned char*)key_s.c_str();
	
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
	
	// Allocate buffer ciphertext, plaintext
	int cphr_size = cphr_file_size;
	unsigned char* cphr_buf = (unsigned char*)malloc(cphr_size);
	unsigned char* clear_buf = (unsigned char*)malloc(cphr_size);
	if(!cphr_buf || !clear_buf) { cerr << "Error: malloc returned NULL (file too big?)\n"; exit(1); }

	// read the ciphertext from file:
	ret = fread(cphr_buf, 1, cphr_size, cphr_file);
	if(ret < cphr_size) { cerr << "Error while reading file '" << cphr_file_name << "'\n"; exit(1); }
	fclose(cphr_file);
	
	int clear_size=0;
	// Decrypt the ciphertext
	clear_size = decrypt(cphr_buf, cphr_size, key, NULL, clear_buf);

	// Add a NULL terminator. We are expecting printable text
	clear_buf[clear_size] = '\0';

	// Show the decrypted text 
	printf("Decrypted text is:\n");
	printf("%s\n", clear_buf);  
	return 0;
}
