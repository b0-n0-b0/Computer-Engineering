#include <iostream> 
#include <string>
#include <stdio.h> // for fopen(), etc.
#include <limits.h> // for INT_MAX
#include <string.h> // for memset()
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/conf.h>


using namespace std;

static DH *get_dh2048_auto(void)
{
    static unsigned char dhp_2048[] = {
        0xF9, 0xEA, 0x2A, 0x73, 0x80, 0x26, 0x19, 0xE4, 0x9F, 0x4B,
        0x88, 0xCB, 0xBF, 0x49, 0x08, 0x60, 0xC5, 0xBE, 0x41, 0x42,
        0x59, 0xDB, 0xEC, 0xCA, 0x1A, 0xC9, 0x90, 0x9E, 0xCC, 0xF8,
        0x6A, 0x3B, 0x60, 0x5C, 0x14, 0x86, 0x19, 0x09, 0x36, 0x29,
        0x39, 0x36, 0x21, 0xF7, 0x55, 0x06, 0x1D, 0xA3, 0xED, 0x6A,
        0x16, 0xAB, 0xAA, 0x18, 0x2B, 0x29, 0xE9, 0x64, 0x48, 0x67,
        0x88, 0xB4, 0x80, 0x46, 0xFD, 0xBF, 0x47, 0x17, 0x91, 0x4A,
        0x9C, 0x06, 0x0A, 0x58, 0x23, 0x2B, 0x6D, 0xF9, 0xDD, 0x1D,
        0x93, 0x95, 0x8F, 0x76, 0x70, 0xC1, 0x80, 0x10, 0x4B, 0x3D,
        0xAC, 0x08, 0x33, 0x7D, 0xDE, 0x38, 0xAB, 0x48, 0x7F, 0x38,
        0xC4, 0xA6, 0xD3, 0x96, 0x4B, 0x5F, 0xF9, 0x4A, 0xD7, 0x4D,
        0xAE, 0x10, 0x2A, 0xD9, 0xD3, 0x4A, 0xF0, 0x85, 0x68, 0x6B,
        0xDE, 0x23, 0x9A, 0x64, 0x02, 0x2C, 0x3D, 0xBC, 0x2F, 0x09,
        0xB3, 0x9E, 0xF1, 0x39, 0xF6, 0xA0, 0x4D, 0x79, 0xCA, 0xBB,
        0x41, 0x81, 0x02, 0xDD, 0x30, 0x36, 0xE5, 0x3C, 0xB8, 0x64,
        0xEE, 0x46, 0x46, 0x5C, 0x87, 0x13, 0x89, 0x85, 0x7D, 0x98,
        0x0F, 0x3C, 0x62, 0x93, 0x83, 0xA0, 0x2F, 0x03, 0xA7, 0x07,
        0xF8, 0xD1, 0x2B, 0x12, 0x8A, 0xBF, 0xE3, 0x08, 0x12, 0x5F,
        0xF8, 0xAE, 0xF8, 0xCA, 0x0D, 0x52, 0xBC, 0x37, 0x97, 0xF0,
        0xF5, 0xA7, 0xC3, 0xBB, 0xC0, 0xE0, 0x54, 0x7E, 0x99, 0x6A,
        0x75, 0x69, 0x17, 0x2D, 0x89, 0x1E, 0x64, 0xE5, 0xB6, 0x99,
        0xCE, 0x84, 0x08, 0x1D, 0x89, 0xFE, 0xBC, 0x80, 0x1D, 0xA1,
        0x14, 0x1C, 0x66, 0x22, 0xDA, 0x35, 0x1D, 0x6D, 0x53, 0x98,
        0xA8, 0xDD, 0xD7, 0x5D, 0x99, 0x13, 0x19, 0x3F, 0x58, 0x8C,
        0x4F, 0x56, 0x5B, 0x16, 0xE8, 0x59, 0x79, 0x81, 0x90, 0x7D,
        0x7C, 0x75, 0x55, 0xB8, 0x50, 0x63
    };
    static unsigned char dhg_2048[] = {
        0x02
    };
    DH *dh = DH_new();
    BIGNUM *p, *g;

    if (dh == NULL)
        return NULL;
    p = BN_bin2bn(dhp_2048, sizeof(dhp_2048), NULL);
    g = BN_bin2bn(dhg_2048, sizeof(dhg_2048), NULL);
    if (p == NULL || g == NULL
            || !DH_set0_pqg(dh, p, NULL, g)) {
        DH_free(dh);
        BN_free(p);
        BN_free(g);
        return NULL;
    }
    return dh;
}

int handleErrors(){
	printf("An error occourred.\n");
	exit(1);
}
		
int main() {
/*GENERATING MY EPHEMERAL KEY*/
/* Use built-in parameters */
printf("Start: loading standard DH parameters\n");
EVP_PKEY *params;
if(NULL == (params = EVP_PKEY_new())) handleErrors();
DH* temp = get_dh2048_auto();
if(1 != EVP_PKEY_set1_DH(params,temp)) handleErrors();
DH_free(temp);
printf("\n");
printf("Generating ephemeral DH KeyPair\n");
/* Create context for the key generation */
EVP_PKEY_CTX *DHctx;
if(!(DHctx = EVP_PKEY_CTX_new(params, NULL))) handleErrors();
/* Generate a new key */
EVP_PKEY *my_dhkey = NULL;
if(1 != EVP_PKEY_keygen_init(DHctx)) handleErrors();
if(1 != EVP_PKEY_keygen(DHctx, &my_dhkey)) handleErrors();

/*write my public key into a file, so the other client can read it*/
string my_pubkey_file_name;
cout << "Please, type the PEM file that will contain your DH public key: ";
getline(cin, my_pubkey_file_name);
if(!cin) { cerr << "Error during input\n"; exit(1); }
FILE* p1w = fopen(my_pubkey_file_name.c_str(), "w");
if(!p1w){ cerr << "Error: cannot open file '"<< my_pubkey_file_name << "' (missing?)\n"; exit(1); }
PEM_write_PUBKEY(p1w, my_dhkey);
fclose(p1w);
string peer_pubkey_file_name;

cout << "Please, type the PEM file that contains the peer's DH public key: ";
getline(cin, peer_pubkey_file_name);
if(!cin) { cerr << "Error during input\n"; exit(1); }
/*Load peer public key from a file*/
FILE* p2r = fopen(peer_pubkey_file_name.c_str(), "r");
if(!p2r){ cerr << "Error: cannot open file '"<< peer_pubkey_file_name <<"' (missing?)\n"; exit(1); }
EVP_PKEY* peer_pubkey = PEM_read_PUBKEY(p2r, NULL, NULL, NULL);
fclose(p2r);
if(!peer_pubkey){ cerr << "Error: PEM_read_PUBKEY returned NULL\n"; exit(1); }

printf("Deriving a shared secret\n");
/*creating a context, the buffer for the shared key and an int for its length*/
EVP_PKEY_CTX *derive_ctx;
unsigned char *skey;
size_t skeylen;
derive_ctx = EVP_PKEY_CTX_new(my_dhkey,NULL);
if (!derive_ctx) handleErrors();
if (EVP_PKEY_derive_init(derive_ctx) <= 0) handleErrors();
/*Setting the peer with its pubkey*/
if (EVP_PKEY_derive_set_peer(derive_ctx, peer_pubkey) <= 0) handleErrors();
/* Determine buffer length, by performing a derivation but writing the result nowhere */
EVP_PKEY_derive(derive_ctx, NULL, &skeylen);
/*allocate buffer for the shared secret*/
skey = (unsigned char*)(malloc(int(skeylen)));
if (!skey) handleErrors();
/*Perform again the derivation and store it in skey buffer*/
if (EVP_PKEY_derive(derive_ctx, skey, &skeylen) <= 0) handleErrors();
printf("Here it is the shared secret: \n");
BIO_dump_fp (stdout, (const char *)skey, skeylen);
/*WARNING! YOU SHOULD NOT USE THE DERIVED SECRET AS A SESSION KEY!
 * IS COMMON PRACTICE TO HASH THE DERIVED SHARED SECRET TO OBTAIN A SESSION KEY.
 */
//FREE EVERYTHING INVOLVED WITH THE EXCHANGE (not the shared secret tho)
EVP_PKEY_CTX_free(derive_ctx);
EVP_PKEY_free(peer_pubkey);
EVP_PKEY_free(my_dhkey);
EVP_PKEY_CTX_free(DHctx);
EVP_PKEY_free(params);


//////////////////////////////////////////////////
//												//
//	USING SHA-256 TO EXTRACT A SAFE KEY!		//
//												//
//////////////////////////////////////////////////
	

// Hashing the shared secret to obtain a key.
//create digest pointer and length variable
unsigned char* digest;
unsigned int digestlen;	
// Create and init context
EVP_MD_CTX *Hctx;
Hctx = EVP_MD_CTX_new();	
//allocate memory for digest
digest = (unsigned char*) malloc(EVP_MD_size(EVP_sha256()));	
//init, Update (only once) and finalize digest
EVP_DigestInit(Hctx, EVP_sha256());
EVP_DigestUpdate(Hctx, (unsigned char*)skey, skeylen);
EVP_DigestFinal(Hctx, digest, &digestlen);
//REMEMBER TO FREE CONTEXT!!!!!!
EVP_MD_CTX_free(Hctx);
//Print digest to screen in hexadecimal
int n;
printf("Digest is:\n");
for(n=0;digest[n]!= '\0'; n++)
	printf("%02x", (unsigned char) digest[n]);
printf("\n");

   


//////////////////////////////////////////////////
//												//
//	THIRD PART: ENCRYPTION WITH A SAFE KEY!		//
//												//
//////////////////////////////////////////////////
int ret; // used for return values   
// read the file to encrypt from keyboard:
string clear_file_name;
cout << "Please, type the file to encrypt: ";
getline(cin, clear_file_name);
if(!cin) { cerr << "Error during input\n"; exit(1); }
// open the file to encrypt:
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
const EVP_CIPHER* cipher = EVP_aes_128_cbc();
int iv_len = EVP_CIPHER_iv_length(cipher);
int block_size = EVP_CIPHER_block_size(cipher);

int key_len = EVP_CIPHER_key_length(cipher);
unsigned char *key = (unsigned char*)malloc(key_len);
memcpy(key, digest, key_len);
// Free the shared secret buffer!
#pragma optimize("", off)
   memset(digest, 0, digestlen);
#pragma optimize("", on)
   free(digest);
BIO_dump_fp (stdout, (const char *)key, key_len);
// Allocate memory for and randomly generate IV:
unsigned char* iv = (unsigned char*)malloc(iv_len);
// Seed OpenSSL PRNG
RAND_poll();
// Generate 16 bytes at random. That is my IV
RAND_bytes((unsigned char*)&iv[0],iv_len);
   
// check for possible integer overflow in (clear_size + block_size) --> PADDING!
// (possible if the plaintext is too big, assume non-negative clear_size and block_size):
if(clear_size > INT_MAX - block_size) { cerr <<"Error: integer overflow (file too big?)\n"; exit(1); }
// allocate a buffer for the ciphertext:
int enc_buffer_size = clear_size + block_size;
unsigned char* cphr_buf = (unsigned char*)malloc(enc_buffer_size);
if(!cphr_buf) { cerr << "Error: malloc returned NULL (file too big?)\n"; exit(1); }
   
//Create and initialise the context with used cipher, key and iv
EVP_CIPHER_CTX *ctx;
ctx = EVP_CIPHER_CTX_new();
if(!ctx){ cerr << "Error: EVP_CIPHER_CTX_new returned NULL\n"; exit(1); }
ret = EVP_EncryptInit(ctx, cipher, key, iv);
if(ret != 1){
	cerr <<"Error: EncryptInit Failed\n";
	exit(1);
}
int update_len = 0; // bytes encrypted at each chunk
int total_len = 0; // total encrypted bytes
  
// Encrypt Update: one call is enough because our file is small.
ret = EVP_EncryptUpdate(ctx, cphr_buf, &update_len, clear_buf, clear_size);
if(ret != 1){
    cerr <<"Error: EncryptUpdate Failed\n";
    exit(1);
}
total_len += update_len;
 
//Encrypt Final. Finalize the encryption and adds the padding
ret = EVP_EncryptFinal(ctx, cphr_buf + total_len, &update_len);
if(ret != 1){
    cerr <<"Error: EncryptFinal Failed\n";
    exit(1);
}
total_len += update_len;
int cphr_size = total_len;

// delete the context and the plaintext from memory:
EVP_CIPHER_CTX_free(ctx);
// Telling the compiler it MUST NOT optimize the following instruction. 
// With optimization the memset would be skipped, because of the next free instruction.
#pragma optimize("", off)
   memset(clear_buf, 0, clear_size);
#pragma optimize("", on)
   free(clear_buf);
   
// write the IV and the ciphertext into a '.enc' file:
string cphr_file_name = clear_file_name + ".enc";
FILE* cphr_file = fopen(cphr_file_name.c_str(), "wb");
if(!cphr_file) { cerr << "Error: cannot open file '" << cphr_file_name << "' (no permissions?)\n"; exit(1); }
   
ret = fwrite(iv, 1, EVP_CIPHER_iv_length(cipher), cphr_file);
if(ret < EVP_CIPHER_iv_length(cipher)) { cerr << "Error while writing the file '" << cphr_file_name << "'\n"; exit(1); }
  
ret = fwrite(cphr_buf, 1, cphr_size, cphr_file);
if(ret < cphr_size) { cerr << "Error while writing the file '" << cphr_file_name << "'\n"; exit(1); }
   
fclose(cphr_file);
cout << "File '"<< clear_file_name << "' encrypted into file '" << cphr_file_name << "'\n";

// deallocate buffers:
free(cphr_buf);
free(iv);


//////////////////////////////////////////////////
//												//
//	FOURTH PART: DECRYPTION OF RECEIVED MSG		//
//												//
//////////////////////////////////////////////////

// read the file to decrypt from keyboard:
string peer_file_name;
cout << "Please, type the file to decrypt: ";
getline(cin, peer_file_name);
if(!cin) { cerr << "Error during input\n"; exit(1); }

// open the file to decrypt:
FILE* peer_file = fopen(peer_file_name.c_str(), "rb");
if(!peer_file) { cerr << "Error: cannot open file '" << peer_file_name << "' (file does not exist?)\n"; exit(1); }

// get the file size: 
// (assuming no failures in fseek() and ftell())
fseek(peer_file, 0, SEEK_END);
long int peer_file_size = ftell(peer_file);
fseek(peer_file, 0, SEEK_SET);
 
// Allocate buffer for IV, ciphertext, plaintext
unsigned char* peer_iv = (unsigned char*)malloc(iv_len);
int peer_cphr_size = peer_file_size - iv_len;
unsigned char* peer_msg_buf = (unsigned char*)malloc(peer_cphr_size);
unsigned char* peer_clear_buf = (unsigned char*)malloc(peer_cphr_size);
if(!peer_iv || !peer_msg_buf || !peer_clear_buf) { cerr << "Error: malloc returned NULL (file too big?)\n"; exit(1); }

// read the IV and the ciphertext from file:
ret = fread(peer_iv, 1, iv_len, peer_file);
if(ret < iv_len) { cerr << "Error while reading file '" << peer_file_name << "'\n"; exit(1); }
ret = fread(peer_msg_buf, 1, peer_cphr_size, peer_file);
if(ret < peer_cphr_size) { cerr << "Error while reading file '" << peer_file_name << "'\n"; exit(1); }
fclose(peer_file);

//Create and initialise the context
EVP_CIPHER_CTX *peer_ctx;
peer_ctx = EVP_CIPHER_CTX_new();
if(!peer_ctx){ cerr << "Error: EVP_CIPHER_CTX_new returned NULL\n"; exit(1); }
ret = EVP_DecryptInit(peer_ctx, cipher, key, peer_iv);
if(ret != 1){
  cerr <<"Error: DecryptInit Failed\n";
  exit(1);
}

int peer_update_len = 0; // bytes decrypted at each chunk
int peer_total_len = 0; // total decrypted bytes

// Decrypt Update: one call is enough because our ciphertext is small.
ret = EVP_DecryptUpdate(peer_ctx, peer_clear_buf, &peer_update_len, peer_msg_buf, peer_cphr_size);
if(ret != 1){
  cerr <<"Error: DecryptUpdate Failed\n";
  exit(1);
}
peer_total_len += peer_update_len;

//Decrypt Final. Finalize the Decryption and adds the padding
ret = EVP_DecryptFinal(peer_ctx, peer_clear_buf + peer_total_len, &peer_update_len);
if(ret != 1){
  cerr <<"Error: DecryptFinal Failed ret="<<ret<<endl;
  exit(1);
}
peer_total_len += peer_update_len;
int peer_clear_size = peer_total_len;

// delete the context from memory:
EVP_CIPHER_CTX_free(peer_ctx);


// write the plaintext into a '.dec' file:
string peer_clear_file_name = peer_file_name + ".dec";
FILE* peer_clear_file = fopen(peer_clear_file_name.c_str(), "wb");
if(!peer_clear_file) { cerr << "Error: cannot open file '" << peer_clear_file_name << "' (no permissions?)\n"; exit(1); }
ret = fwrite(peer_clear_buf, 1, peer_clear_size, peer_clear_file);
if(ret < peer_clear_size) { cerr << "Error while writing the file '" << peer_clear_file_name << "'\n"; exit(1); }
fclose(peer_clear_file);

// Just out of curiosity, print on stdout the used IV retrieved from file.
cout<<"Used IV:"<<endl;
BIO_dump_fp (stdout, (const char *)peer_iv, iv_len);

// delete the plaintext from memory:
// Telling the compiler it MUST NOT optimize the following instruction. 
// With optimization the memset would be skipped, because of the next free instruction.
#pragma optimize("", off)
memset(peer_clear_buf, 0, peer_clear_size);
#pragma optimize("", on)
free(peer_clear_buf);

cout << "File '"<< peer_file_name << "' decrypted into file '" << peer_clear_file_name << "', clear size is " << peer_clear_size << " bytes\n";

// deallocate buffers:
free(iv);
free(peer_msg_buf);
return 0;
}
