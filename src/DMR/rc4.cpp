/**
  Implementation of MS authentication challenge-response algorithm
    6.4.8.0
    Authentication Procedures - Introduction
    ETSI TS 102 361-4

    Input1: 24 bit unsigned int random number between 0 and FFFCDF which will be concatenated with key
    Input2: 16 byte (128 bit) MS auth key
    Output1: 24 bit unsigned int random number challenge sent to MS
    Output2: 24 bit unsigned int from the last 3 bytes of the keystream which is the challenge response from MS

    Written by Adrian Musceac YO8RZZ (c)2024 based on code from:
    https://en.wikipedia.org/wiki/RC4
    https://crypto.stackexchange.com/questions/80596/how-to-generate-the-keystream-from-ivkey-in-rc4
**/
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "rc4.h"
const int perm = 256;
void swap(unsigned char *a, unsigned char *b) {
    unsigned char temp = *a;
    *a = *b;
    *b = temp;
}
void KSA(unsigned char *key, unsigned char *S, unsigned int key_length) {
    for (int i = 0; i < perm; i++) {
        S[i] = i;
    }
    int j = 0;
    for (int i = 0; i < perm; i++) {
        j = (j + S[i] + key[i % key_length]) % perm;
        swap(&S[i], &S[j]);
    }
}
void PRGA(unsigned char *S, unsigned char *in, unsigned char *keystream, int text_length) {
    int i = 0, j = 0;
    for (int k = 0; k < text_length; k++) {
        i = (i + 1) % perm;
        j = (j + S[i]) % perm;
        swap(&S[i], &S[j]);
        int r = S[(S[i] + S[j]) % perm];
        keystream[k] = in[k] ^ r;
    }
}
void arc4_get_challenge_response(unsigned char *key, unsigned int key_length, unsigned int &challenge, unsigned int &response) {
    srand(time(NULL));
    challenge = (unsigned int)(rand() & 0xFFFFFF);
    if(challenge > 0xFFFCDF)
        challenge = 0xFFFCDF; // clamp to spec
    unsigned int text_length = 259;
    unsigned char text[text_length];
    memset(text, 0, text_length);
    unsigned int concatenated_length = key_length + 3;
    unsigned char concatenated[concatenated_length];
    memset(concatenated, 0, concatenated_length);
    memcpy(concatenated + 3, key, key_length);
    concatenated[0] = (challenge >> 16) & 0xFF;
    concatenated[1] = (challenge >> 8) & 0xFF;
    concatenated[2] = (challenge) & 0xFF;
    unsigned char S[perm];
    KSA(concatenated, S, concatenated_length);
    unsigned char keystream[text_length];
    PRGA(S, text, keystream, text_length);
    response = 0;
    response |= keystream[256] << 16;
    response |= keystream[257] << 8;
    response |= keystream[258];
}
