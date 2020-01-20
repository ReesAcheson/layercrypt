
#include "lc_vars.h"

KeyStruct_t *TheKey = NULL;

//Ptr to array[MaxLayers] of uint32_t
//These are where the layer generators deposit their results
aRand_t *Rands = NULL;

uint16_t *Rands2 = NULL;

uint64_t *Rands8 = NULL;

//Access Rands as single 1024 byte array.
uint8_t *Rand1024Byte = NULL;	//Will point to Rands[0].


//array of ptrs of RNG_state_t structs to be allocated on the heap 
//for the state of each random generator.
//Including the extra layer[256] for general purpose generator
RNG_state_t *LayerGenState[LayersMax+1] = {NULL};  
			     

//These 2 are the layer and byte within the layer to use for picking the 
//next random byte from the set of layered randoms
//PickByte value can be 0-3
uint8_t LayerPick = 0, BytePick = 0;	//these will be replaced with values from rands

//A struct to store items to be included as a header in the encrypted file.
options_t options;

int RandsAccessCount = 0;	
//Count to determine when to refresh randoms. Randoms will be selected from same
//layer set until RandRefreshRate is reached.

int OverwriteTarget = 1;
char KeyFn[120] = "layercrypt.key";		
char SrcFn[120] = "";
char DestFn[120] = "";

char password[PasswdSize] = "";		//String for the optional password
//If supplied, password is only as an array of seeds to desyncronize the random generators.

//uint8_t Pass8[PasswdSize] ={0};		
//Array to hold a hash to check password and correct program operation

uint64_t FileSize = 0;

FILE *src=NULL, *dest=NULL;

enum debug_t debug = dg_none;

enum OpMode_t OpMode = mode_none;

int verbose = 0;

//Some debug variables
int Unreasonables = 0;
int ReasonablesChecked = 0;
int ReasonableRange = 10;
int ReasonableMax = 14;


