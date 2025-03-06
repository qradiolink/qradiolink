#ifndef RC4_H
#define RC4_H
void swap(unsigned char *a, unsigned char *b);
void KSA(unsigned char *key, unsigned char *S, unsigned int key_length);
void PRGA(unsigned char *S, unsigned char *in, unsigned char *keystream, int text_length);
void arc4_get_challenge_response(unsigned char *key, unsigned int key_length, unsigned int &challenge, unsigned int &response);
#endif // RC4_H
