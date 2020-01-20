#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>		//for uint32_t define

#define ProgramName "layercrypt"
#define VersionProg   "00.00"

#define LayersMax 256
	//This should not change. It certainly cannot be greater than 256.
	//System was designed for 256 layers.

#define RandStateSize 256
	//buffer used by random_r(), bytes (a power of 2)
	//random.c says sizes are: 8, 32, 64, 128 or 256

#define RandRefreshRate 8
	//Number of accesses to randoms allowed before making a new set
	//1 to 16 suggested.  Speed effected

//Different ways to access the Rands
typedef union {
  uint32_t rnd4;
  uint16_t rnd2[2];
  uint8_t  byte[4];
} aRand_t;


//The keys
typedef struct KeyStruct_t{
  char ProgName[20];
  char Ver[12];		
  uint32_t layer[LayersMax];	//The keys
} __attribute__((aligned(4),packed)) KeyStruct_t;		


//In linux struct random_data is defined in stdlib.h as: random_data
//To avoid a conditional compile, here it is with a redefined type name.
typedef struct {
    int32_t *fptr;		// Front pointer.  
    int32_t *rptr;		// Rear pointer.  
    int32_t *state;		// Array of state values.  
    int rand_type;		// Type of random number generator.  
    int rand_deg;		// Degree of random number generator.
    int rand_sep;		// Distance between front and rear.  
    int32_t *end_ptr;	// Pointer behind state table.
  } random_data_t;

//RNG RandData and State arrays for each layer
typedef struct  {
  random_data_t RandData;
  char State[RandStateSize];
} RNG_state_t; 


KeyStruct_t *TheKey = NULL;

//Ptr to array[MaxLayers] of uint32_t
//These are where the layer generators deposit their results
aRand_t *Rands = NULL;

//array of ptrs of RNG_state_t structs to be allocated on the heap 
//for the state of each random generator.
//Including the extra layer[256] for general purpose generator
RNG_state_t *LayerGenState[LayersMax+1] = {NULL};  

//These 2 are the layer and byte within the layer to use for picking the 
//next random byte from the set of layered randoms
//PickByte value can be 0-3
uint8_t LayerPick = 0, BytePick = 0;	//these will be replaced with values from Rands

int RandsAccessCount = 0;	//Count to determine when to refresh Rands
int OverwriteTarget = 1;
char KeyFn[120] = "layercrypt.key";		
char SrcFn[120] = "";
char DestFn[120] = "";

void abortprog(char *s1, char *s2)
{ printf("%s ", s1);
  if (*s2)
    printf("%s", s2);
  printf("\n");
  exit(EXIT_FAILURE);
}

//Return 1 if exists
//Mode values of accesss(const char *path,int mode)
//  00 Existence only
//  02 Write permission
//  04 Read permission
//  06 Read and write permission
int fExists(char *fn, int AccessMode)
{ if( (access( fn, AccessMode )) == 0 )
    return 1;
  else
    return 0; 		//Not exist
}


// Copy src to string dst of size siz.  At most siz-1 characters will be copied.
// Returns strlen(dst); if length src >= siz, truncation occurred.
size_t _strlcpy(char *dst, const char *src, size_t siz)	//siz=sizeof()
{ int i;
  for (i=0; i<siz-1 && src[i]; i++)
    dst[i] = src[i];
  dst[i] = 0;
  return i; 
}


void syntax(char *progname)
{ printf("syntax: %s Source Target\n", progname);
  puts  ("This is just a basib demo to illustrate the layer concept. No attempt has");
  puts  ("  been made to provide or insure file name conformity such as file extensions.");
  puts  ("Also, no attempt has been made to differentiate between encoding or ");
  puts  ("  decoding - they are the same.");
  puts  ("Encoding always produces the same random progression from a given key.");
  puts  ("This is not a useful version.");
  exit(EXIT_SUCCESS);
}


//************************ LAYERS ********************************

//New Rands for a single layer, loop times
//Used when refreshing the randoms, and for de-synchronizing the layers
void AdvanceLayer(uint8_t lay, uint16_t loop)
{ int i;
  //load the layer's gen with its data
  local__setstate_r(LayerGenState[lay]->State, &LayerGenState[lay]->RandData);
  for (i=0; i<loop; i++) 
    local__random_r(&LayerGenState[lay]->RandData, &Rands[lay].rnd4);    
}

//New rands for all layers (except LayersMax).
//Layer generators are advanced by 1, each producing the next in their progression
void Next4Rands() 
{ int i;				
  for (i=0; i<LayersMax; i++) 
    AdvanceLayer(i, 1);
}

//Get new LayerPick & BytePick values
//BytePick is normalized to 0 to 3
inline void NewPicks()
{ LayerPick = Rands[LayerPick].byte[BytePick];
  BytePick = Rands[LayerPick].byte[BytePick] &3;	//0 thru 3
}

//Inc the RandsAccessCount and generate new Rands if count 
//  is greater than RandRefreshRate.
inline void RefreshRands()
{ RandsAccessCount++;
  if (RandsAccessCount > RandRefreshRate) {	//Time for new rands?  Rate is #define in lc_vars.h
    Next4Rands();
    RandsAccessCount = 0;
  }	//if
}

//If needed, refresh all the randoms by calling their generators.
//Return byte at layer "lay", and byte position "k"
//k = 0 thru 3 (k is normalized here.)
//Also see Rand1024Byte[], which accesses the 1024 bytes directly, not by layers, or RandByte().
//Most selections from the randoms are made through this function, or RandByte() which calls it.
//This function refreshes the randoms during a byte selection, others may not.
uint8_t LayerRandByte(uint8_t lay, uint8_t k)
{ k = k & 3;
  RefreshRands();
  return Rands[lay].byte[k];
}

//Select a number from a layer and a byte using the randoms for the selection
//If zero, then it tries again. 
//Because zeros should be avoided when encrypting using xor, this function should be used for that.
uint8_t RandByte()
{ uint8_t b;
  NewPicks();
  b = LayerRandByte(LayerPick, BytePick);
  if (!b) 		//If zero, pick another
    b = RandByte();
  return b;
}


//Initialize each of the random generator buffers.  (Excluding extra at LayersMax)
//Use to start or restart the generators with key.
void InitRandLayers(KeyStruct_t *key) 
{ int i;
  uint32_t n;			
  //inspect the seeds of 1st 3 layers of key to see if they are non-zero
  if (key->layer[0] == 0 && key->layer[1] == 0 && key->layer[2] == 0)
     abortprog("It appears that the key has not been loaded or created.", "");  
  for (i=0; i<LayersMax; i++) 	{	//don't do the extra general purpose generator 
    n = key->layer[i];		//the key value for this layer.
    if (local__initstate_r(n, LayerGenState[i]->State, RandStateSize, &LayerGenState[i]->RandData) != 0)
       abortprog("Error in initstate_r", "");
  }
  //Make the 1st set of rands available, but not the extra one
  Next4Rands();
}


//*********************************************************************

int LoadKey(KeyStruct_t *key, char *fn)
{ FILE *f;
  int bytes;
  f = fopen(fn, "r");
  if (!f) return 0;
  bytes = fread(key, 1, sizeof(KeyStruct_t), f);
  fclose(f);
  if (strncmp(key->ProgName, ProgramName, strlen(ProgramName)) != 0)
    return 0;
  return 1;
}	//LoadKey


//Used for encypting or decrypting the source file
int CryptFile(FILE *src, FILE *dest)
{ int i, bytesread, byteswrite;
  uint8_t buf[1024];
  while ((bytesread = fread(&buf, 1, sizeof(buf), src))) {
    for (i=0; i<bytesread; i++)
      buf[i] = buf[i] ^ RandByte();		//RandByte() never returns a zero
    byteswrite = fwrite(&buf, 1, bytesread, dest);
    if (bytesread != byteswrite)
      abortprog("ERROR: Bytes written not equal bytes read.", "");
  }	//while src
  return 1;
}	//CryptFile


//Open both the source and target files.
int OpenFiles(FILE **src, FILE **dest)
{ if (!*SrcFn || !*DestFn)
    return 0;
  if (!OverwriteTarget && fExists(DestFn, 0))
    abortprog("Target file exists.  Please erase", DestFn);
  *src = fopen(SrcFn, "r");
  if (!*src)
    abortprog("Unable to open source:", SrcFn);
  *dest = fopen(DestFn, "w");
  if (!*dest) {
    fclose(*src);
    abortprog("Unable to create target:", DestFn);
  }
  return 1;
}



//************************* init ******************************

void init(int argc, char *argv[])
{ int i;
  if (argc <2) 
    abortprog("Please supply source and target filenames.","");
  _strlcpy(SrcFn, argv[1], sizeof(SrcFn)); 
  _strlcpy(DestFn, argv[2], sizeof(DestFn)); 

  TheKey = calloc(1, sizeof(KeyStruct_t));

//Allocate RAM for each random gen data 
  for (i=0; i<LayersMax; i++)			
    LayerGenState[i] = calloc(1, sizeof(RNG_state_t)); 	
    //initstate must be passed zeroed ram 1st time (calloc)
    
//Then calloc space for generator results - the rands. 
  Rands = calloc(LayersMax, sizeof(uint32_t));
}	//init


//****************************** main ********************************

int main(int argc, char *argv[])
{ FILE *src=NULL, *dest=NULL;
  init(argc, argv);

  if (!LoadKey(TheKey, KeyFn))
    abortprog("Unable to load the key:", KeyFn);

  if (!OpenFiles(&src, &dest))
    abortprog("Unable to open files", "");

  InitRandLayers(TheKey);	//Initialize all the RNGs witheir respective keys.

  CryptFile(src, dest);		//Encrypt or decrypt the file.
  fclose(src);
  fclose(dest);

  return EXIT_SUCCESS;
}
