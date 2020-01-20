//lc_crypt-sequ.c

#include "lc_vars.h"



/*

This file makes these changes from Basic2 in the way the program operates:
   1) When encrypting, the output is made out-of-sequence (mixed) by a sequencer.
     During decoding the input is re-ordered by the same sequencer so that
     the output is in proper sequence.
     If the define EncSequenceMixes is zero, then the output is not mixed.
   2) If UseMultiBytes is defined then the byte selected to xor the character
     is created from multiple bytes in the Rands.
*/


#define EncSequenceMixes 1	//Set to zero to keep naturally ordered output- no mixing.
				//Greater than zero to mix - to disorder the output sequence.
				//0 to 12 suggested. Each mix slows cypher.



//************************* xor byte selection *************************************

 #define UseMultiBytes
//If defined, each byte selected for encryption will be created from multiple randoms.
//They are added together into a uint8_t and wrap with modulo 256 arithmetic.

#define MultiBytes 32
//Must be larger than zero.
//The number of bytes to combine when UseMultiBytes is defined.

#ifdef UseMultiBytes
//Add Rand bytes together into a uint8_t so that they wrap.
//Never returns zero.
uint8_t AddMultiBytes()
{ uint8_t b = 0;
  int i;
  for (i=0; i<MultiBytes; i++) {
    NewPicks();
    b += Rands[LayerPick].byte[BytePick];
  } 
  RefreshRands();
  if (!b)
    b = AddMultiBytes();
  return b;
}
#endif


uint8_t aByte()
{ 
#ifdef UseMultiBytes
  return AddMultiBytes();	//Combine multiple bytes.
#else
  return RandByte();	//Just select a random and use it.
#endif
}




//************************** Mixers ***************************

//Mixers are used to mix-up an array by swapping elemets.
//They are used here by the sequencers to disorder a sequentially ordered array.
//There are 8 and 16 bit versions, with a power-of-two version, and a not version for each.



//MixPw2Array8 only works when size is a power of 2 and is 256 or less.
void MixPw2Array8(uint8_t array[], uint8_t size, uint8_t k, uint8_t mixes)
{ int i, j;
  uint8_t c, l2, mask;
  if (!isPowerOfTwo(size))
    abortprog("MixPw2Array8: size passed was not a power of 2", "");
  if ((size < 2) || (size > 256))
    abortprog("MixPw2Array8: size passed was out of range", "");
  mask = size-1; 
  k &= 3;				//Insure k is 0-3
  for (j=0; j<mixes; j++) 		//mixes loop
    for (i=0; i<size; i++) {		//mix the sequence
      l2 = LayerRandByte(i, k) & mask;	//the layer to swap with
      c = array[i];
      array[i] = array[l2];
      array[l2] = c;
    }	//for i
}

//Does the mixing for GenerateSequence8 and GenerateLayerSequence.
//Do not use for array sizes larger than 256.
//In this function k is normalized to 0-3.
void MixArray8(uint8_t array[], uint8_t size, uint8_t k, uint8_t mixes)
{ int i, j;
  uint8_t c, l2;
  if (size > 256)
    abortprog("MixArray8 size too large", "");
  if (isPowerOfTwo(size))
    MixPw2Array8(array, size, k, mixes); 
  else {  //If not power of 2, this mixes the array
    k = k&3;				//Insure k is 0-3
    for (j=0; j<mixes; j++) 		//mixes loop
      for (i=0; i<size; i++) {		//mix the sequence
        l2 = LayerRandByte(i, k) % size;	//the layer to swap with
        c = array[i];
        array[i] = array[l2];
        array[l2] = c;
      }	//for i 
  }	//else  
}



//MixPw2Array16 only works when size is a power of 2.
//Once debugging is done the abortive test can be removed.
//The difference with this version is that it is able to use '&' in stead of '%'
void MixPw2Array16(uint16_t array[], uint16_t size, uint8_t mixes)
{ int i, j;
  uint16_t c, l2, mask;
  if (!isPowerOfTwo(size))
    abortprogI("MixPw2Array16: size passed was not a power of 2 :", size);
  mask = size-1;  
  for (j=0; j<mixes; j++)  {		//Mixes loop
    Next4Rands();			//Generate a fresh set
    for (i=0; i<size; i++) {		//Mix the array
      l2 =  Rands2[i&0x01ff] & mask;	//The position to swap with. 0x1ff limits it to 0-511
                                        //Rands2 is ptr to Rands as 2 byte elements
					//There are 512 2-byte elements in the 1024 byte array.
      c = array[i];			//Perform the sawp
      array[i] = array[l2];
      array[l2] = c;
    }	//for i
  }	//for mixes
}

//Does the mixing for GenerateSequence16
//For mixing uint16_t arrays
void MixArray16(uint16_t array[], uint16_t size, uint8_t mixes)
{ int i, j;
  uint16_t c, l2;
  if (isPowerOfTwo(size))   
      MixPw2Array16(array, size, mixes);
  else   	//If not power of 2 this mixes the rest  
    for (j=0; j<mixes; j++) {		//mixes loop
      Next4Rands();
      for (i=0; i<size; i++) {		//mix the sequence.
        l2 =  Rands2[i&0x01ff] % size;	//the poistion to swap with
        c = array[i];
        array[i] = array[l2];
        array[l2] = c;
      }	//for i
  }	//for mixes
}

//Swap RNG Layer pointers so that the random production for all the layers 
//is disrupted.  Similar to key-mixing, but key remains unchanged.
//This can be called during SetupRandoms() or even periodically during encryption.
void MixLayerGens()
{ int i, j=0;	//j must be initialized
  RNG_state_t *tmp;		//ptr to a RNG's data
  for (i=0; i<LayersMax; i++) {
    j = Rands[i].byte[j&3];
    tmp = LayerGenState[i];
    LayerGenState[i] = LayerGenState[j];
    LayerGenState[j] = tmp;
  }
}

/* =================== Sequencers =================================

The idea when making a sequence is to 1st create an array of sequential numbers 
and then mix them up. The array can then be used to randomly redirect requests, 
such as for layers to use.
We do not want duplicates, so we fill sequentially and then only do swaps.
Stepping thru the list, the appropriate byte (k) from Rands[i] is used to 
select the next in the sequence, and that position of array is swapped with 
array[i], producing a mixed up array.
The var "mixes" runs through the mixing multiple times.  Zero returns a
naturally ordered numerical set, i.e 0,1,2,3...
   
*/


//Uses 8 bit elements to accommodate sequences <= 256.  
//Zero mixes returns the unaltered sequential array.
void GenerateSequence8(uint8_t *array, uint8_t size, uint8_t k, uint8_t mixes)
{ uint8_t i;
  k = k & 3;
  for (i=0; i<size; i++) 	//Fill array with sequential numbers
    array[i] = i;
  if ((mixes == 0) || (size == 0)) 
    return;
  MixArray8(array, size, k, mixes);  
}	//GenerateSequence8


//Uses 16 bit elements to accommodate sequences greater than 256.  
//Typically used for sequencing 1024 elements such as *Rand1024Byte[] which is 256*4 bytes.
//If less than 256 elements, use the two 8 bit version.
//Zero mixes returns the unaltered sequential array.
void GenerateSequence16(uint16_t *array, uint16_t size, uint8_t mixes)
{ uint16_t i;
  for (i=0; i<size; i++) 	//Fill array with sequential numbers
    array[i] = i;
  if ((mixes == 0) || (size == 0)) 
    return;
  MixArray16(array, size, mixes);  
}	//GenerateSequence16






//************************ encryption *********************************

#define EncBufSize 1024

//These read file into a buffer, then create a sequence used to 
//disorder or reorder the output buffer. The out-buffer is written to file.
//A new sequence is created with each read.
//The disordering of the output requires a separate function for encrpt and decrypt.
//To disable the disorder mode set EncSequenceMixes to zero.
//At completion, both source and target files are closed.

int EncryptFile(FILE *src, FILE *dest)
{int i, bytesread, byteswrite;
  uint8_t *buf_in, *buf_out;
  uint16_t *BufSequ;			//sequence for picking from in-file
  buf_in  = malloc(EncBufSize);
  buf_out = malloc(EncBufSize);
  BufSequ = malloc(EncBufSize * sizeof(BufSequ[0])); 
  while ((bytesread = fread(buf_in, 1, EncBufSize, src))) {
if (!bytesread)
  abortprog("bytes raed = zero", "");
    GenerateSequence16(BufSequ, bytesread, EncSequenceMixes);	//Create a sequence
    for (i=0; i<bytesread; i++)					//Step thru the array
      buf_out[BufSequ[i]] = buf_in[i] ^ aByte();
    byteswrite = fwrite(buf_out, 1, bytesread, dest);
    if (bytesread != byteswrite)
      abortprog("ERROR: Bytes written not equal bytes read.", "");
  }	//while src
  fclose(src);
  fclose(dest);
  free(BufSequ);
  free(buf_out);
  free(buf_in);
  return 1;
}

int DecryptFile(FILE *src, FILE *dest)
{int i, bytesread, byteswrite;
  uint8_t *buf_in, *buf_out;
  uint16_t *BufSequ;				//sequence for reordering out file
  buf_in  = malloc(EncBufSize);
  buf_out = malloc(EncBufSize);
  BufSequ = malloc(EncBufSize * sizeof(BufSequ[0]));
  while ((bytesread = fread(buf_in, 1, EncBufSize, src))) {
    GenerateSequence16(BufSequ, bytesread, EncSequenceMixes);
    for (i=0; i<bytesread; i++)
      buf_out[i] = buf_in[BufSequ[i]] ^ aByte();	
    byteswrite = fwrite(buf_out, 1, bytesread, dest);
    if (bytesread != byteswrite)
      abortprog("ERROR: Bytes written not equal bytes read.", "");
  }	//while src
  fclose(src);
  fclose(dest);
  free(BufSequ);
  free(buf_out);
  free(buf_in);
  return 1;
}
