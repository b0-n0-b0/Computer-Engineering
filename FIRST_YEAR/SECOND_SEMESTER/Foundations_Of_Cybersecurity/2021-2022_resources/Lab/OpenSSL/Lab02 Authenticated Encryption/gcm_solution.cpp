#include <iostream> 
#include <string>
#include <stdio.h> // for fopen(), etc.
#include <limits.h> // for INT_MAX
#include <string.h> // for memset()
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
using namespace std;
int handleErrors(){
	printf("An error occourred.\n");
	exit(1);
}

int gcm_encrypt(unsigned char *plaintext, int plaintext_len,
                unsigned char *aad, int aad_len,
                unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *ciphertext,
                unsigned char *tag)
{
    EVP_CIPHER_CTX *ctx;
    int len=0;
    int ciphertext_len=0;
    // Create and initialise the context
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();
    // Initialise the encryption operation.
    if(1 != EVP_EncryptInit(ctx, EVP_aes_128_gcm(), key, iv))
        handleErrors();

    //Provide any AAD data. This can be called zero or more times as required
    if(1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len))
        handleErrors();

    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;
	//Finalize Encryption
    if(1 != EVP_EncryptFinal(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;
    /* Get the tag */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag))
        handleErrors();
    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

int gcm_decrypt(unsigned char *ciphertext, int ciphertext_len,
                unsigned char *aad, int aad_len,
                unsigned char *tag,
                unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    int ret;
    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();
    if(!EVP_DecryptInit(ctx, EVP_aes_128_gcm(), key, iv))
        handleErrors();
	//Provide any AAD data.
    if(!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len))
        handleErrors();
	//Provide the message to be decrypted, and obtain the plaintext output.
    if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;
    /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, tag))
        handleErrors();
    /*
     * Finalise the decryption. A positive return value indicates success,
     * anything else is a failure - the plaintext is not trustworthy.
     */
    ret = EVP_DecryptFinal(ctx, plaintext + len, &len);

    /* Clean up */
    EVP_CIPHER_CTX_cleanup(ctx);

    if(ret > 0) {
        /* Success */
        plaintext_len += len;
        return plaintext_len;
    } else {
        /* Verify failed */
        return -1;
    }
}
int main (void)
{
	unsigned char msg[] = "Short message";
	//create key
	unsigned char key_gcm[]="1234567890123456";
	unsigned char iv_gcm[]= "123456780912";
	unsigned char *cphr_buf;
	unsigned char *tag_buf;
	int cphr_len;
	int tag_len;
	int pt_len = sizeof(msg);
	cphr_buf=(unsigned char*)malloc(pt_len+16);
	tag_buf=(unsigned char*)malloc(16);
	gcm_encrypt(msg, pt_len, iv_gcm, 12, key_gcm, iv_gcm, 12, cphr_buf, tag_buf);
	cout<<"CT:"<<endl;
	BIO_dump_fp (stdout, (const char *)cphr_buf, pt_len);
	cout<<"Tag:"<<endl;
	BIO_dump_fp (stdout, (const char *)tag_buf, 16);
	unsigned char *dec_buf;
	dec_buf=(unsigned char*)malloc(pt_len);
	gcm_decrypt(cphr_buf, pt_len, iv_gcm, 12, tag_buf, key_gcm, iv_gcm, 12, dec_buf);
	cout<<"PT:"<<endl;
	BIO_dump_fp (stdout, (const char *)dec_buf, pt_len);
	return 0;
}
