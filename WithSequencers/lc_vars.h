#ifndef LC_VARS_INCLUDED
#define LC_VARS_INCLUDED

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>		//for uint32_t
#include <inttypes.h>		//for printing uint64_t

#include "lc_random.h"

#define ProgramName "layercrypt"
#define VersionProg   "00.02"

#define LayersMax 256
//	This should not change. It certainly cannot be greater than 256.
//	System was designed for 256 layers, using less has not lately been 
//	investigated.


//#define RandBytesMax LayersMax * 4
	//Number of total bytes in Rands[].
	//4 bytes per rand

#define BootSeedSize 32
	//Number of bytes for boot seed in encrypted file header
	//This an even integer and at least 2.
	//Have tried up to 254.
	//Think 12 too small? Then increase it.
	//Besure to adjust Options_size_check (-b=1 -v shows size)

#define RandStateSize 256
	//buffer used by random_r(), bytes (a power of 2)
	//random.c says sizes are: 8, 32, 64, 128 or 256

#define RandRefreshRate 8
	//Number of accesses to randoms allowed before making a new set
	//1 to 16 suggested.  Speed effected

#define PasswdSize 64

#define FnSize 120

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
  uint8_t Pass8[PasswdSize];
  uint8_t BootSeeds[BootSeedSize];
  uint64_t FileSize;
} __attribute__((aligned(4),packed))  options_t;


//The keys
typedef struct KeyStruct_t{
  char ProgName[20];
  char Ver[12];		
  uint32_t keys[LayersMax];	//The keys
} __attribute__((aligned(4),packed)) KeyStruct_t;		



//random_data_t defined in lc_random.h

//RNG RandData and State arrays for each layer
typedef struct  {
  random_data_t RandData;
  char State[RandStateSize];
} RNG_state_t; 

enum debug_t {dg_none, dg_general, dg_RandsCheck, dg_ShowKey} debug_t;
enum OpMode_t {mode_none, mode_newkey, mode_encrypt, mode_decrypt} OpMode_t;

extern KeyStruct_t *TheKey;

//1 byte Ptr to array[MaxLayers] of uint32_t
//These are where the layer generators deposit their results
extern aRand_t *Rands;

//Access Rands as single 1024 byte array.
extern uint8_t *Rand1024Byte;	//Will point to Rands[0].

//2 byte Ptr to array[MaxLayers] of uint32_t
extern uint16_t *Rands2;

extern uint64_t *Rands8;

//array of ptrs of RNG_state_t structs to be allocated on the heap 
//for the state of each random generator.
//Including the extra layer[256] for general purpose generator
extern RNG_state_t *LayerGenState[LayersMax+1];  
			     

//These 2 are the layer and byte within the layer to use for picking the 
//next random byte from the set of layered randoms
//PickByte value can be 0-3
extern uint8_t LayerPick, BytePick;	//these will be replaced with values from rands

//A struct to store items to be included as a header in the encrypted file.
extern options_t options;

extern int RandsAccessCount;	//Count to determine when to refresh randoms
extern int OverwriteTarget;
extern char KeyFn[FnSize];		
extern char SrcFn[FnSize];
extern char DestFn[FnSize];
extern char password[PasswdSize];

//extern uint8_t Pass8[PasswdSize];

extern uint64_t FileSize;

extern  FILE *src;
extern  FILE *dest;

extern enum debug_t debug;
extern enum OpMode_t OpMode;

extern int verbose;

extern int Unreasonables;
extern int ReasonablesChecked;
extern int ReasonableRange;
extern int ReasonableMax;



#endif
