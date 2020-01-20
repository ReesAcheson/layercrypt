#ifndef LC_CRYPT_SEQ_INCLUDED
#define LC_CRYPT_SEQ_INCLUDED

//MixPw2Array8 only works when size is a power of 2 and is 256 or less.
//Once debugging is done the two abortive tests can be removed.
void MixPw2Array8(uint8_t array[], uint8_t size, uint8_t k, uint8_t mixes);

//Does the mixing for GenerateSequence8 and GenerateLayerSequence.
//Do not use for array sizes larger than 256.
//In this function k is normalized to 0-3.
void MixArray8(uint8_t array[], uint8_t size, uint8_t k, uint8_t mixes);


//MixPw2Array16 only works when size is a power of 2.
//Once debugging is done the abortive test can be removed.
//The difference between this version is that it is able to use '&' in stead of '%'
void MixPw2Array16(uint16_t array[], uint16_t size, uint8_t mixes);

//Does the mixing for GenerateSequence16
//For mixing uint16_t arrays
void MixArray16(uint16_t array[], uint16_t size, uint8_t mixes);

//Uses 8 bit elements to accommodate sequences <= 256.  
//Zero mixes returns the unaltered sequential array.
void GenerateSequence8(uint8_t *array, uint8_t size, uint8_t k, uint8_t mixes);

void MixLayerGens();

int EncryptFile(FILE *src, FILE *dest);

int DecryptFile(FILE *src, FILE *dest);

#endif
