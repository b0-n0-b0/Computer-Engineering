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
    /*generate parameters from command line and paste the output function here*/
}

int handleErrors(){
	printf("An error occourred.\n");
	exit(1);
}
		
int main() {

/*CHECKPOINT 1*/
/*GENERATING MY EPHEMERAL KEY*/
/* Use built-in parameters */
printf("Generating ephemeral DH KeyPair\n");

/*CHECKPOINT 2*/
/*write my public key into a file, so the other client can read it*/
cout << "Please, type the PEM file that will contain your DH public key: ";

/*CHECKPOINT 2.1*/
cout << "Please, type the PEM file that contains the peer's DH public key: ";



/*CHECKPOINT 3*/
printf("Deriving a shared secret\n");


//////////////////////////////////////////////////
//												//
//	USING SHA-256 TO EXTRACT A SAFE KEY!		//
//												//
//////////////////////////////////////////////////
	
printf("Digest is:\n");
for(n=0;digest[n]!= '\0'; n++) //REMEMBER TO CHANGE THE VARIABLE NAME, IF YOU WANT THE CODE TO COMPILE!
	printf("%02x", (unsigned char) digest[n]);
printf("\n");

   


/*CHECKPOINT 4*/

cout << "Please, type the file to encrypt: ";


/*CHECKPOINT 5*/
// read the file to decrypt from keyboard:
cout << "Please, type the file to decrypt: ";


return 0;
}
