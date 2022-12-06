The University of Pisa is in great debt.
Luckily Alice Feiht is making a payment through her bank account.
However she's trying to fool us. She's paying 100.000$ when se owe us MUCH MORE!
Are you able to find a way to make her pay?

You have at your disposal:
	bt_enc: the executable used by bank users (as Alice) to encrypt transactions.
	bt_dec: the executable that the bank uses to decrypt transactions.
	captured/transaction.enc: is the encrypted transaction that we captured from Alice.

Info:
	The used encryption cipher is AES-128 in ECB mode.
	Each user has his own password, and the bank knows them.
	Alice Feiht nickname is 'Alice'
	The nickname of our University is 'PisaUniversity'

Your objective is to create somehow a ciphertext that fools the "verifier" executable.
When you think you have solved the exercise, execute './verifier' and when it asks for a file type the name of your forged ciphertext.

Good luck!

P.s. hint: try to encrypt/decrypt something by yourself using bt_enc/bt_dec... It may give you further information!
