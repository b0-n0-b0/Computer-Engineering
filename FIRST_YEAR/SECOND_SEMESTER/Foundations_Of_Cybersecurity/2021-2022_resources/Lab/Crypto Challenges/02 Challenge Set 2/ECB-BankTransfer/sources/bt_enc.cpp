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

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
unsigned char *iv, unsigned char *ciphertext)
{
	EVP_CIPHER_CTX *ctx;

	int len;
	int ciphertext_len;

	/* Create and initialise the context */
	ctx = EVP_CIPHER_CTX_new();

	// Encrypt init
	EVP_EncryptInit(ctx, EVP_aes_128_ecb(), key, iv);

	// Encrypt Update: one call is enough because our mesage is very short.
	if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
	handleErrors();
	ciphertext_len = len;

	//Encrypt Final. Finalize the encryption and adds the padding
	if (1 != EVP_EncryptFinal(ctx, ciphertext + len, &len))
	handleErrors();
	ciphertext_len += len;

	// MUST ALWAYS BE CALLED!!!!!!!!!!
	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;
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
   
   // read the payer:
   string payer;
   cout << "Please, type the nickname of the payer: ";
   getline(cin, payer);
   if(!cin) { cerr << "Error during input\n"; exit(1); }
   
   // read the beneficiary:
   string beneficiary;
   cout << "Please, type the nickname of the beneficiary: ";
   getline(cin, beneficiary);
   if(!cin) { cerr << "Error during input\n"; exit(1); }
   
	// read the amount:
	int aux;
	char buf [17];
	char amount [17];
	cout << "Please, type the amount(max 16 characters, integer!): ";
	cin>>aux;
	int a;
	a=sprintf (buf, "%d", aux);
    
    for (int i=0; i<=16;i++){
	   if(i>=16-a)
			amount[i]=buf[i-(16-a)];
	   else amount[i]='0';
	}
	
	// generate transaction number
	srand (time(NULL));
	char buffer [17];
	char t_num [17];
	int n;
	srand (time(NULL));
	n=sprintf (buffer, "%d",rand());
	for (int i=0; i<=16;i++){
	   if(i>=16-n)
			t_num[i]=buffer[i-(16-n)];
	   else t_num[i]='0';
	}
	printf ("[%s]\n",t_num);
	string transfer = payer+ " transfers " + amount + "$ to: " + beneficiary + ". Transfer# " + t_num;	
	cout<<transfer<<endl;
 
	// get the transaction size: 
	long int clear_size = transfer.length()+1;

	// put the transaction in a buffer
	unsigned char* clear_buf = (unsigned char*)malloc(clear_size);
	if(!clear_buf) { cerr << "Error: malloc returned NULL (file too big?)\n"; exit(1); }
	clear_buf=(unsigned char*)transfer.c_str();
	
	// Buffer for ciphertext. Ensure the buffer is long enough for the
	//ciphertext which may be longer than the plaintext, depending on the
	//algorithm and mode
	unsigned char* ciphertext = (unsigned char *) malloc(clear_size+16);

	int decryptedtext_len, ciphertext_len;
	// Encrypt utility function
	ciphertext_len = encrypt (clear_buf, clear_size, key, NULL, ciphertext);

	// Redirect our ciphertext to the terminal
	printf("Ciphertext is:\n");
	BIO_dump_fp (stdout, (const char *)ciphertext, ciphertext_len);

	// write the ciphertext into 'transaction.enc' file:
	string cphr_file_name = "transaction.enc";
	FILE* cphr_file = fopen(cphr_file_name.c_str(), "wb");
	if(!cphr_file) { cerr << "Error: cannot open file '" << cphr_file_name << "' (no permissions?)\n"; exit(1); }

	ret = fwrite(ciphertext, 1, ciphertext_len, cphr_file);
	if(ret < ciphertext_len) { cerr << "Error while writing the file '" << cphr_file_name << "'\n"; exit(1); }

	fclose(cphr_file);
  
	return 0;
}
