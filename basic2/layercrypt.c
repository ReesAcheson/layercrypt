#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>		//for uint32_t define

#define ProgramName "layercrypt"
#define VersionProg   "00.01"

#define LayersMax 256
	//This should not change. It certainly cannot be greater than 256.
	//System was designed for 256 layers.

#define BootSeedSize 32
	//Number of bytes for boot seed in encrypted file header
	//This a power of 2 integer.

#define RandStateSize 256
	//buffer used by random_r(), bytes (a power of 2)
	//random.c says sizes are: 8, 32, 64, 128 or 256

#define RandRefreshRate 8
	//Number of accesses to randoms allowed before making a new set.
	//1 to 16 suggested.  Speed effected

//Different ways to access the Rands
typedef union {
  uint32_t rnd4;
  uint16_t rnd2[2];
  uint8_t  byte[4];
} aRand_t;

//Option Info to be stored in the encrypted document file.
//BootSeeds are exchanged with values obtained from a set of randoms
//before being used to decync the layers.
typedef struct options_t{
  char ProgName[20];
  char Ver[12];
  uint8_t BootSeeds[BootSeedSize];
} __attribute__((aligned(4),packed))  options_t;


//The keys
typedef struct KeyStruct_t{
  char ProgName[20];
  char Ver[12];		
  uint32_t keys[LayersMax];	//The keys for all 256 RNGs
} __attribute__((aligned(4),packed)) KeyStruct_t;		


//In linux struct random_data is defined in stdlib.h as: random_data
//To avoid an OS conditional compile, here it is with a redefined type name.
typedef struct {
    int32_t *fptr;		// Front pointer.  
    int32_t *rptr;		// Rear pointer.  
    int32_t *state;		// Array of state values.  
    int rand_type;		// Type of random number generator.  
    int rand_deg;		// Degree of random number generator.
    int rand_sep;		// Distance between front and rear.  
    int32_t *end_ptr;		// Pointer behind state table.
  } random_data_t;

//RandData and State arrays for each layer's RNG
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
//PickByte value should be 0-3
uint8_t LayerPick = 0, BytePick = 0;	//these will be replaced with values from rands

//A struct to store items to be included as a header in the encrypted file.
options_t options;

int RandsAccessCount = 0;	//Count to determine when to refresh randoms
int OverwriteTarget = 1;
char KeyFn[120] = "layercrypt.key";		
char SrcFn[120] = "";
char DestFn[120] = "";
char password[64] = "";

enum debug {dg_none, dg_general, dg_RandsCheck, dg_ShowKey} debug = dg_none;

typedef enum OpMode_t {mode_none, mode_newkey, mode_encrypt, mode_decrypt} OpMode_t;
OpMode_t OpMode = mode_none;




//Processor Time Stamp Counter
//Returns the number of cycles since processor reset. 
uint64_t rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

void abortprog(char *s1, char *s2)
{ printf("%s ", s1);
  if (*s2)
    printf("%s", s2);
  printf("\n");
  if (src)
    fclose(src);
  if (dest)
    fclose(dest);
  exit(EXIT_FAILURE);
}

void abortprogI(char *s1, int n)
{ printf("%s %i\n", s1, n);
  if (src)
    fclose(src);
  if (dest)
    fclose(dest);
  exit(EXIT_FAILURE);
}

int isPowerOfTwo(uint32_t x)
{ return ((x != 0) && !(x & (x - 1))); }

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

//Looks for word in choices table and returns 1 based word position (1st word, 2nd word, etc)
//Choices is a char array that is space delimited (extra spaces ignored).
//Does not alter either string.
//Returns zero if not found in table
//example: strWordPos(s, "help usage debug")
size_t strWordPos(char *word, char *choices)
{ size_t len;	//length of 'word'
  size_t WordCount = 0;
  char *p = choices;
  if (word == NULL || choices == NULL) return 0;
  if (*word == 0 || *choices == 0) return 0;
  len = strlen(word);
  while (*p) {
    WordCount++;
    if (strncmp(word, p, len) == 0)
      return WordCount;
    for (; *p && *p>' '; p++);	//skip to next space
    for (; *p && *p<=' '; p++);	//skip to next non-space
  }	//while
  return 0;
}


void syntax(char *progname)
{ printf("syntax: %s [-e | -d | --newkey] [-k KeyFn] [-o] [--password pass] Source Target\n", progname);
  puts  ("  -e Encrypt mode.");
  puts  ("  -d Decrypt mode.");
  puts  ("  --newkey [keyname]  Create a new key.");
  puts  ("  -o Overwrite target file if one already exists.");
  puts  ("  --password pass  Use a password to alter the encryption. Optional.");
  puts  ("      If used, must also be supplied for decrypt.");
  puts  ("This is just a demo to illustrate the layer concept. No attempt has");
  puts  ("  been made to provide or insure file name conformity such as file extensions.");
  puts  ("  Likewise, the password is not checked and the encoding proceeds, ");
  puts  ("  incorrectly if passwords do not match.");
  exit(EXIT_SUCCESS);
}


//Debug, show the contents of the keys in the key 
void ShowKey(KeyStruct_t *key)
{ int i, j=0;
  for (i=0; i<LayersMax; i++) {
    if (j>=12) {
      puts(""); j=0;}
    printf("%08x ", key->keys[i]);
    j++;
  }
  puts("");
}

//for debugging
void DisplayOptSeeds()
{ int i;
  puts("Option BootSeeds:");
  for (i=0; i<BootSeedSize; i++)
    printf("%4i", options.BootSeeds[i]);
  puts("");
}



//************************ LAYERS ********************************

//New randoms for a single layer, loop times
//Used when refreshing the randoms, and for de-synchronizing the layers
void AdvanceLayer(uint8_t lay, uint16_t loop)
{ int i;
  //load the gen with this layer's data
  local__setstate_r(LayerGenState[lay]->State, &LayerGenState[lay]->RandData);
  for (i=0; i<loop; i++) {
    local__random_r(&LayerGenState[lay]->RandData, &Rands[lay].rnd4);
  }    
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
  LayerPick = Rands[LayerPick].byte[BytePick];
}

//Inc the RandsAccessCount and generate new rands if count 
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
//Mmost selections from the randoms are made through this function, or RandByte() which calls it.
//This function is refreshes the randoms during a byte selection, others may not.
uint8_t LayerRandByte(uint8_t lay, uint8_t k)
{ k = k & 3;
  RefreshRands();
  return Rands[lay].byte[k];
}

//Select a number from a layer and byte using the randoms for the selection.
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
  if (key->keys[0] == 0 && key->keys[1] == 0 && key->keys[2] == 0)
     abortprog("It appears that the key has not been loaded or created.", "");  
  for (i=0; i<LayersMax; i++) 	{	//don't do the extra general purpose generator 
    n = key->keys[i];		//the key value for this layer.
    //initstate must be passed zeroed ram 1st time.
    memset(LayerGenState[i], 0, sizeof(RNG_state_t));
    if (local__initstate_r(n, LayerGenState[i]->State, RandStateSize, &LayerGenState[i]->RandData) != 0)
       abortprog("Error in initstate_r", "");
  }
  //Make the 1st set of rands available, but not the extra one
  Next4Rands();
}

//Initialize the general purpose RNG, layer 256, using rdtsc for seed.
//Call once before 1st use of ARandom.
void InitARandom()
{ local__initstate_r(rdtsc(), LayerGenState[LayersMax]->State, RandStateSize, &LayerGenState[LayersMax]->RandData);
}

//A general purpose random number producer.
//ARandom gets initialized in init(). It is independent of layer operations.
//Use for generating randoms not connected with the layers and the key - they
//  will be different each time the program is run.
//jmp is for advancing into production.
uint32_t ARandom(uint16_t jmp)
{ uint32_t n;
  int i;
  if (!jmp) jmp++;	//zero would not return a random
  local__setstate_r(LayerGenState[LayersMax]->State, &LayerGenState[LayersMax]->RandData);
  for (i=0; i<jmp; i++)
    local__random_r(&LayerGenState[LayersMax]->RandData, &n);
  return n;
}

//Copy the current set of Rands to the key part of KeyStruct_t
//to make a sub-key from Rands.
//Key is not installed.
void KeyFromRands(KeyStruct_t *key)
{ memcpy(key->keys, Rands, sizeof(key->keys)); }

//************************* Decync ********************************

//In a loop, get next layer randoms and then desynchronize layers.
//Scale is the degree of desyncing, each scale increment is a doubling of decyncing.
//Scale 0-7 (will be normalized).  Must be 16-bit due to shift.
void DesyncLayers(uint8_t loops, uint16_t scale)
{ int i, j;
  scale = (scale&7)+1;	        //normalize to 1-8
  scale = (1 << scale)-1;	//1, 3, 7, 15, 31, 63, 127, 255
  for (j=0; j<loops; j++) {
    Next4Rands();
    for (i=0; i<LayersMax; i++)
      AdvanceLayer(i, RandByte()*scale);	//advance layer i, by RandByte*scale interations
  }    
}

//DesyncWithSeeds uses a seeds array to perform desyncronizing of layer generators.
//Desyncs all layers.
//Advances each layer generator's production by an amount selected from rands by a seed.
//SeedsUsed must be a power of two.
//The IsPowerOfTwo can be removed after debugging.
// #define ScaleLimit 0x3f
#define ScaleLimit 0x0f		//a PowerOf2-1
void DesyncWithSeeds(uint8_t SeedArray[], uint8_t SeedsUsed, uint8_t loops)
{ int i, j, k=0;
  uint8_t seed, scale;
  if (!isPowerOfTwo(SeedsUsed)) 
    abortprogI("DecyncWithSeeds requires a power-of-two SeedsUsed. Value was", SeedsUsed);
  SeedsUsed--;	//make zero based for '&' mask
  for (j=0; j<loops; j++)
    for (i=0; i<LayersMax; i++) {
      k = LayerRandByte(i, k) & 3;
      seed = SeedArray[i & SeedsUsed];
      scale = RandByte() & ScaleLimit;
      AdvanceLayer(i, LayerRandByte(seed, k)*scale);
    }	//for i
}


//Exchange the bytes in BootSeeds array with the randoms that they point to.
void ExchangeSeeds(options_t *opt)
{ int i;
  for (i=0; i<BootSeedSize; i++)
    opt->BootSeeds[i] = LayerRandByte(opt->BootSeeds[i], (RandByte()&3));
}


//If a password is used, step through each char and use them to further decync the layers
void ProcessPassword(char *pass)
{ char *p;
  for (p=pass; *p; p++)
    AdvanceLayer((uint8_t) *p, RandByte());
  puts("Also using a password to desync the layers.");
}


//Cycle the generators to unevenly advance their progressions.
void SetupRandoms()
{ if (debug) DisplayOptSeeds();
  ExchangeSeeds(&options);	//Replace the seeds with randoms based on the seeds
  if (debug) DisplayOptSeeds();
  DesyncWithSeeds(options.BootSeeds, BootSeedSize, RandByte());

  if (*password)
    ProcessPassword(password);

//Up until now the generators have been running under the initialization with the original key.
//Now make a new key (a sub-key) and reinitialize the generators with it.
  KeyFromRands(TheKey);		//make a sub-key
  InitRandLayers(TheKey);	//Restart the generators with the new sub-key

  DesyncLayers(RandByte()&1, RandByte());
//Increasing the above &1 greatly increases the desyncing, and also slows the setup.

//The generators have been restarted using a key that is unique to each encrypted file,
// and they are now each in different stages of their number progression.
//We are ready to begin using the generators for encryption.
}


//***************************** File Operations ************************************


//Create a new key file. 
//Does not use layers and so layer generators need not have been started.
int FileCreateKey(KeyStruct_t *key, char *fn)
{ FILE *f;
  int bytes, i;
  if (!key) abortprog("TheKey has not been allocated RAM.","");
  for (i=0; i<LayersMax; i++) //Fill the keys with randoms from the independant generator
    key->keys[i] = ARandom(1);
  f = fopen(fn, "w");
  if (!f)
    abortprog("Unable to create key file", fn);
  _strlcpy(key->ProgName, ProgramName, sizeof(key->ProgName));
  _strlcpy(key->Ver, VersionProg, sizeof(TheKey->Ver));
  bytes = fwrite(key, 1, sizeof(KeyStruct_t), f);
  printf("%i bytes wrttten to %s\n", bytes, fn);
  fclose(f);
  if (bytes != sizeof(KeyStruct_t))
    abortprog("SIZE ERROR writing key to file", "");
  if (debug == dg_ShowKey)
    ShowKey(key);
  return 1;
}	//FileCreateKey


int FileLoadKey(KeyStruct_t *key, char *fn)
{ FILE *f;
  int bytes;
  f = fopen(fn, "r");
  if (!f) return 0;
  bytes = fread(key, 1, sizeof(KeyStruct_t), f);
  fclose(f);
  if (debug == dg_ShowKey)
    ShowKey(key);
  if (strncmp(key->ProgName, ProgramName, strlen(ProgramName)) != 0)
    return 0;
  if (bytes != sizeof(KeyStruct_t))
    abortprogI("Size of key read in is incorrect:", bytes);
  return 1;
}	//FileLoadKey


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
  fclose(src);
  fclose(dest);
  return 1;
}	//CryptFile


//Write the options to the open file
int WriteOptions(options_t *op, FILE *f)
{ int bytes; 
  if (!f) return 0;
  bytes = fwrite(op, 1, sizeof(options), f);
  if (bytes != sizeof(options)) 
    return 0;	//error
  return 1;
}

//Read the options from the open file
int ReadOptions(options_t *op, FILE *f)
{ int bytes;
  if (!f) return 0;
  bytes = fread(op, 1, sizeof(options), f);
  if (bytes != sizeof(options)) return 0;
  if (strncmp(op->ProgName, ProgramName, strlen(ProgramName)) != 0)
    return 0;	//error
  return 1;
}

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

//This version does not use '=' embedded in param, but uses ffmpeg style params: the value is next param.
//Handling the long params with the short.
//One hypen with Multiple short options allowed.
void ParseCmdln3(int argc, char *argv[])
{ int i, j;
  int params = 0;
  char s[120], *arg;
  if (argc == 0)
  syntax(argv[0]);
  for (i=1; i<argc; i++) {
    _strlcpy(s, argv[i], sizeof(s));
    arg = NULL;		//argument to parameter
    if (i<argc-1) {
      arg = argv[i+1];
      if (arg && *arg=='-') arg=NULL;	//point to next arg only if not another option
    }
    if (s[0] == '-') {
      if (s[1] == '-')	//we have 2 hyphens.
        //skip over 2 hyphens, find option, set s[1] to short option, set [s2] to '='
        //to stop multiple option mode.  If no short equivelent, set to hi or low chars.
        //123-127, & 0-32 available. Only 7 bit seems ,to be allowed.
        switch (strWordPos(s+2, "help key newkey password debug")) {
          case 0: abortprog(s, " Long Option not found"); break;
          case 1: syntax(argv[0]);  break;
          case 2: s[1] = 'k';  s[2] = '='; break;	//key name
          case 3: s[1] = 3;    s[2] = '='; break;	//newkey
          case 4: s[1] = 'p';  s[2] = '='; break;	//password
          case 5: s[1] = 5;    s[2] = '='; break;	//debug
          default: abortprog(s, " Long Option not allowed for"); break;	
                 //this should never happen
        }	//switch

      for (j=1; s[j] && s[j]!='='; j++)
        switch (s[j]) {
          case 'h': syntax(argv[0]); 
                    break;
          case 'e': OpMode = mode_encrypt;
                    break;
          case 'd': OpMode = mode_decrypt;
                    break;
          case 'k': if (arg) {
                      i++;
                      _strlcpy(KeyFn, arg, sizeof(KeyFn));
                    }
                    break;
          case 'o': OverwriteTarget = 1;
                    break;
          case 'p': if (arg) {	//password
                      i++;
                      _strlcpy(password, arg, sizeof(password));
                    }
                    break;
          case 3: if (arg) {	//newkey
                      i++;
                      _strlcpy(KeyFn, arg, sizeof(KeyFn));
                    }
                    OpMode = mode_newkey;
                    break;
           case 5: debug = 1; 
                   if (arg) {	//debug number
                      i++;
                      debug = atoi(arg);
                    }
                   break;
          default : abortprog ("Unrecognized option.", s);
        }	//switch

    }	//not a hyphen
    else {	// not a hyphen
      params++;
      switch (params) {
        case 1: _strlcpy(SrcFn, s, sizeof(SrcFn)); break;
        case 2: _strlcpy(DestFn, s, sizeof(DestFn)); break;
        default: printf("Parameter %i ignored\n", params);
      }	//switch
    }
  }	//for i
}	//ParseCmdln3()

void init(int argc, char *argv[])
{ int i;
  ParseCmdln3(argc, argv);
  memset(&options, 0, sizeof(options_t));		//zero the options
  TheKey = calloc(1, sizeof(KeyStruct_t));

//Allocate RAM for each random gen data 
//Also accomodate the extra 257th general purpose generator (<=)
  for (i=0; i<=LayersMax; i++)			
    LayerGenState[i] = calloc(1, sizeof(RNG_state_t)); 	
    //initstate must be passed zeroed ram 1st time (calloc)
    
//Then calloc space for generator results - the rands. 
//Note that there is none for extra gen purpose gen (256)
  Rands = calloc(LayersMax, sizeof(uint32_t));
  
//Initialize the general purpose RNG, layer 256
  InitARandom();
}	//init


//Fill the options struct with name, version and BootSeeds.
//The BootSeeds are selected using a separate random generator that has been initialized
//using the rdtsc.  Therefore the seeds are unique to each encrypted file.
//There is no connection to the layers of randoms, or to the key used to make them.
void FillOpHeader(options_t *op)
{ int i;
  _strlcpy(op->ProgName, ProgramName, sizeof(op->ProgName));
  _strlcpy(op->Ver, VersionProg, sizeof(op->Ver));
  for (i=0; i<BootSeedSize; i++)
    op->BootSeeds[i] = (uint8_t)ARandom(1);	//Don't use layers for these randoms.
}	//FillOpHeader


//****************************** main ********************************

int main(int argc, char *argv[])
{ FILE *src=NULL, *dest=NULL;
  init(argc, argv);

  if (OpMode == mode_none)
    abortprog("Nothing to do.","");

  if (OpMode == mode_newkey) {
    if (FileCreateKey(TheKey, KeyFn))
      printf("Key Created: %s\n", KeyFn);
    return EXIT_SUCCESS;
  }

  if (!FileLoadKey(TheKey, KeyFn))
    abortprog("Unable to load the key:", KeyFn);

  if (!OpenFiles(&src, &dest))
    abortprog("Unable to open files", "");

//Start up the layer RNGs
  InitRandLayers(TheKey);

  switch (OpMode) {
    case mode_encrypt: FillOpHeader(&options);
                       if (WriteOptions(&options, dest)) {
                         SetupRandoms();
                         CryptFile(src, dest); 
                       } else
                         abortprog("Writing options failed.", "");
                       break;
    case mode_decrypt: if (ReadOptions(&options, src)) {
                         SetupRandoms();
                         CryptFile(src, dest); 
                       }  else
                         abortprog("unable to retrieve options from", SrcFn);
                       break;
  }	//switch OpMode
  return EXIT_SUCCESS;
}
